/*
 * HackQuad - an open-source firmware+hardware quadcopter
 * Copyright (C) 2020, Andrew Howard, <divisionind.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "mdns.h"
#include "esp_log.h"
#include "hackquad/motor.h"
#include "hackquad/registry.h"
#include "hackquad/battery.h"
#include "hackquad/wifi.h"
#include "hackquad/mpu.h"
#include "hackquad/hackquad_msg.h"
#include "hackquad/udpserver.h"
#include "hackquad/pid.h"
#include "hackquad/httpserver.h"
#include "hackquad/blinkcodes.h"
#include "pthread.h"

#define POWER_SEL_IO        33
#define HACKQUAD_MDNS_EN    1   /* whether or not to init mdns */
#define HACKQUAD_TEST_LOG   0   /* whether or not to spawn logging task */

#define FLAG_PANIC_MODE     (1 << 31)
#define FC_PANIC_MODE_ACT   50  /* max degrees of rotation before fc enters panic-mode */

#define STATUS_UPDATE_RATE  100  /* delay in ms between sending status updates */
#define FC_UPDATE_TIMEOUT   50   /* delay in ms between recv-ing updates before fc times-out */
#define NO_CTRL_TIMEOUT     3000 /* delay in ms to enter panic mode after not recving ctrl update */

/* copied strait from arduino cause im lazy */
#define constrain(amt, low, high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

struct control_data {
    float throttle, x, y, z;
    int flag_clear_panicmode;
};

TaskHandle_t task_hackquad_main;
float hq_avg_fcloop;

static struct control_data control;
static struct udp_context udp_ctx;
static struct pid_kon pid_angle_consts;
static struct pid_kon pid_rate_consts;
static struct pid_kon pid_yaw_rate_consts;
static pthread_mutex_t ctrl_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct pid_ctx pid_angle[2] = {
        {.kons = &pid_angle_consts},
        {.kons = &pid_angle_consts}
};

static struct pid_ctx pid_rate[3] = {
        {.kons = &pid_rate_consts},
        {.kons = &pid_rate_consts},

        /* yaw, this needs different tuning parameters */
        {.kons = &pid_yaw_rate_consts}
};

static void hackquad_main(void *args) {
    (void) args;

    u32 msg;
    u64 curr_time, last_mpu_update = 0, last_fc_update = 0;
    float dt;
    struct control_data ctrl;
    float output[3];
    float x_set_point_adj;
    float y_set_point_adj;
    int fc_panicmode = 0;

    memset(&ctrl, 0, sizeof(ctrl));

    ESP_ERROR_CHECK(iic_init(0, I2C_BUS0_SDA, I2C_BUS0_SCL, I2C_BUS0_FRQ));
    mpu_init();

    /*if (!mpu_has_calibration) {
        ESP_LOGW(TAG, "MPU HAS NOT BEEN CALIBRATED! Flight will likely be unstable.");
    }*/

    // started, change blink-rate
    blc_setrate(BLCR_NORMAL);

    for (;;) {
        // on mpu or user-input data change, we re-do the flight calculations / update the motors
        if (xTaskNotifyWait(0, 0xFFFFFFFF, &msg, FC_UPDATE_TIMEOUT / portTICK_PERIOD_MS)) {

            // if new mpu data rdy, read data
            if (msg & HQMSG_MPU_UPDATE) {
                curr_time = esp_timer_get_time();
                mpu_read((float) (curr_time - last_mpu_update) * 1e-6f);
                last_mpu_update = curr_time;
            }

            // new control data rdy, read data
            if (msg & HQMSG_CTRL_UPDATE) {
                pthread_mutex_lock(&ctrl_mutex);
                memcpy(&ctrl, &control, sizeof(ctrl));
                pthread_mutex_unlock(&ctrl_mutex);
            }

            // use updated data to redo the flight calculations
            // calculate dt sense last fc update
            curr_time = esp_timer_get_time();
            dt = (float) (curr_time - last_fc_update) * 1e-6f;
            last_fc_update = curr_time;

            // keep track of avg flight controller refresh rate
            hq_avg_fcloop = hq_avg_fcloop * 0.995f + dt * 0.005f;

            /*
             * --=== FLIGHT CONTROLLER ===--
             */
            if (ctrl.throttle > 0) {
                // trigger panic mode if angle becomes too skewed
                if (fabsf(mpu_latest.angle.x) > FC_PANIC_MODE_ACT ||
                    fabsf(mpu_latest.angle.y) > FC_PANIC_MODE_ACT) {
                    fc_panicmode = 1;
                }

                // ensure motors shut off if in panic-mode
                if (ctrl.flag_clear_panicmode) {
                    fc_panicmode = 0;
                    blc_setrate(BLCR_NORMAL);
                } else
                if (fc_panicmode) {
                    blc_setrate(BLCR_VERY_FAST);
                    goto panic_mode;
                }

                // Rz(yaw)*in_setpoint
                // makes controls relative to world yaw==0, to make it easier to control while spinning (till i get yaw tuned)
                // likely suffers from angle roll-over issue
                //float yaw_rad = -mpu_latest.angle.z * DEG_TO_RAD;
                //float asx = ctrl.x * cosf(yaw_rad) + ctrl.y * sinf(yaw_rad);
                //float asy = -ctrl.x * sinf(yaw_rad) + ctrl.y * cosf(yaw_rad);

                x_set_point_adj = pid_update(&pid_angle[0], ctrl.x, mpu_latest.angle.x, dt);
                output[0] = pid_update(&pid_rate[0], -x_set_point_adj, mpu_latest.rate.x, dt);

                y_set_point_adj = pid_update(&pid_angle[1], ctrl.y, mpu_latest.angle.y, dt);
                output[1] = pid_update(&pid_rate[1], -y_set_point_adj, mpu_latest.rate.y, dt);

                // yaw always rate/gyro controlled
                output[2] = pid_update(&pid_rate[2], ctrl.z, mpu_latest.rate.z, dt);

                // TODO extend pid chain with linear acceleration control
                // TODO add multiplier for battery percentage adjustment
                // combine pid motor matrix
                motor_throttle(M0, constrain(ctrl.throttle - output[0] - output[1] - output[2], 0, 1023));
                motor_throttle(M1, constrain(ctrl.throttle - output[0] + output[1] + output[2], 0, 1023));
                motor_throttle(M2, constrain(ctrl.throttle + output[0] + output[1] - output[2], 0, 1023));
                motor_throttle(M3, constrain(ctrl.throttle + output[0] - output[1] + output[2], 0, 1023));
            } else {
                panic_mode:
                for (int i = 0; i < 4; i++)
                    motor_throttle(i, 0);
            }
        } else
            goto panic_mode; // timed-out (prob MPU issue), ensure motors remain off
    }
}

void udp_recv_handler(int id, u8 *data, size_t len) {
    switch (id) {
        default:
            ESP_LOGE(TAG, "packet of unknown id recv-ed, id = %i", id);
            break;

        /* UPDATE_CONTROL */
        case 69:
            if (len < 16)
                return;

            // pseudo-mutex created here by xTaskNotify() as we can be fairly confident that once notified, the flight
            // controller loop will process this WAY before our next control data update
            // could modify the data and cause problems
            pthread_mutex_lock(&ctrl_mutex);
            control.throttle = ((float *) data)[0] * MOTOR_MAX_THROTTLE;
            control.x        = ((float *) data)[1];
            control.y        = ((float *) data)[2];
            control.z        = ((float *) data)[3];

            // TODO create u8 flags var which stores clear panic mode and en stabilization info
            //      or keep as is and maintain stabilization for some period after zero throttle
            //      maybe use accel values and maintain stabilization until down-accel is zero

            // use sign bit to indicate that we wish to reset panic-mode detection
            control.flag_clear_panicmode = *((u32 *) &control.throttle) & FLAG_PANIC_MODE;
            *((u32 *) &control.throttle) &= ~FLAG_PANIC_MODE;
            pthread_mutex_unlock(&ctrl_mutex);

            // notify new data
            xTaskNotify(task_hackquad_main, HQMSG_CTRL_UPDATE, eSetBits);
            portYIELD();
            break;
    }
}

/**
 * Dont really need a separate task here if sockets support polling. Could just
 * recv and handle in the hackquad_main task.
 *
 * It would save on a lot of unnecessary mov's.
 */
static void udp_server_task(void *arg) {
    (void) arg;

    udp_ctx.recv_handler = udp_recv_handler;
    udp_create(&udp_ctx, IPADDR_ANY, UDPSERVER_PORT);

    for (;;)
        udp_yield(&udp_ctx);
}

#if HACKQUAD_TEST_LOG
static void test_log_task(void *arg) {
    (void) arg;

    for (;;) {
        //printf("fc = %.2f ms, \tx = %.2f, \ty = %.2f, \trx = %.2f, \try = %.2f\n", hq_avg_fcloop * 1000.0f, mpu_latest.angle[0], mpu_latest.angle[1], mpu_latest.rate[0], mpu_latest.rate[1]);
        printf("ax = %.5f, \tay = %.5f, \taz = %.5f, \tpitch = %.2f, \troll = %.2f\n", mpu_latest.raw_acc[0], mpu_latest.raw_acc[1], mpu_latest.raw_acc[2], mpu_latest.angle[0], mpu_latest.angle[1]);
        vTaskDelay(75 / portTICK_PERIOD_MS);
    }
}
#endif

static void hq_mdns_init() {
    // pretty slow
    /* without mdns in AP mode
        Ping statistics for 192.168.4.1:
            Packets: Sent = 50, Received = 50, Lost = 0 (0% loss),
        Approximate round trip times in milli-seconds:
            Minimum = 1ms, Maximum = 25ms, Average = 2ms
     */

    /* with mdns in AP mode
        Ping statistics for 192.168.4.1:
            Packets: Sent = 50, Received = 49, Lost = 1 (2% loss),
        Approximate round trip times in milli-seconds:
            Minimum = 1ms, Maximum = 39ms, Average = 3ms
     */
    mdns_txt_item_t mdns_txts[] = {
            {"board", "esp32"},
            {"path",  "/"}
    };

    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set("hackquad"));
    ESP_ERROR_CHECK(mdns_instance_name_set("hackquad"));
    ESP_ERROR_CHECK(mdns_service_add("_webserver", "_http", "_tcp", 80, mdns_txts,
                                     sizeof(mdns_txts) / sizeof(mdns_txt_item_t)));
}

static void status_update_task(void *arg) {
    (void) arg;

    struct __attribute__((packed)) {
        u8 id;
        float battery;
        s8 rssi;
        float fc_loop_time;
        float x, y, z;
    } status_update;

    status_update.id = 20;

    for (;;) {
        status_update.battery = battery_read();
        status_update.rssi = wifi_get_rssi();
        status_update.fc_loop_time = hq_avg_fcloop;
        status_update.x = mpu_latest.angle.x;
        status_update.y = mpu_latest.angle.y;
        status_update.z = mpu_latest.angle.z;

        udp_sendp(&udp_ctx, (u8 * ) & status_update, sizeof(status_update));
        vTaskDelay(STATUS_UPDATE_RATE / portTICK_PERIOD_MS);
    }
}

void app_main() {
    motor_init();
    blc_init();

    // enable high-power mode (also turns on camera circuitry)
    //gpio_reset_pin(POWER_SEL_IO);
    //gpio_set_level(POWER_SEL_IO, 1);

    reg_init();
    battery_init();
    wifi_init();
#if HACKQUAD_MDNS_EN
    hq_mdns_init();
#endif
    http_init();

    xTaskCreate(hackquad_main, "hackquad_main", 4096, NULL, configMAX_PRIORITIES - 1, &task_hackquad_main);
    xTaskCreate(udp_server_task, "udp_server", 2048, NULL, configMAX_PRIORITIES - 2, NULL);
    xTaskCreate(status_update_task, "status_task", 2048, NULL, configMAX_PRIORITIES - 3, NULL);
#if HACKQUAD_TEST_LOG
    xTaskCreate(test_log_task, "log_task", 2048, NULL, 0, NULL);
#endif
}

DEFINE_REGISTRY({
    {"WIFI_AP_SSID",       REG_STR, &wifi_ap_ssid, 0, {0}},
    {"WIFI_AP_PASS",       REG_STR, &wifi_ap_pass, 0, {0}},
    {"WIFI_AP_AUTHMODE",   REG_8B,  &wifi_ap_authmode, 0, {0}},
    {"WIFI_AP_MAX_CONN",   REG_8B,  &wifi_ap_max_connection, 0, {0}},
    {"WIFI_AP_CHANNEL",    REG_8B,  &wifi_ap_channel, 0, {0}},
    {"WIFI_ST_SSID",       REG_STR, &wifi_st_ssid, 0, {0}},
    {"WIFI_ST_PASS",       REG_STR, &wifi_st_pass, 0, {0}},
    {"WIFI_ST_AUTHMODE",   REG_8B,  &wifi_st_authmode, 0, {0}},
    {"WIFI_ST_MAX_RETRYS", REG_8B,  &wifi_st_max_retrys, 0, {0}},
    {"WIFI_MODE",          REG_8B,  &wifi_mode, 0, {0}},

    {"MPU_HAS_CALIBRATION", REG_8B,  &mpu_has_calibration, 0, {0}},
    {"MPU_GYROFFSET_X",     REG_FLT, &mpu_gyroffset_x, 0, {0}},
    {"MPU_GYROFFSET_Y",     REG_FLT, &mpu_gyroffset_y, 0, {0}},
    {"MPU_GYROFFSET_Z",     REG_FLT, &mpu_gyroffset_z, 0, {0}},
    {"MPU_ACCOFFSET_X",     REG_FLT, &mpu_accoffset_x, 0, {0}},
    {"MPU_ACCOFFSET_Y",     REG_FLT, &mpu_accoffset_y, 0, {0}},
    {"MPU_ACCOFFSET_Z",     REG_FLT, &mpu_accoffset_z, 0, {0}},

    {"PID_ANGLE_KP",      REG_FLT, &pid_angle_consts.kp, 0, {0}},
    {"PID_ANGLE_KI",      REG_FLT, &pid_angle_consts.ki, 0, {0}},
    {"PID_ANGLE_KD",      REG_FLT, &pid_angle_consts.kd, 0, {0}},
    {"PID_ANGLE_EPSILON", REG_FLT, &pid_angle_consts.epsilon, 0, {0}},
    {"PID_RATE_KP",       REG_FLT, &pid_rate_consts.kp, 0, {0}},
    {"PID_RATE_KI",       REG_FLT, &pid_rate_consts.ki, 0, {0}},
    {"PID_RATE_KD",       REG_FLT, &pid_rate_consts.kd, 0, {0}},
    {"PID_RATE_EPSILON",  REG_FLT, &pid_rate_consts.epsilon, 0, {0}},
    {"PID_YAWRATE_KP",    REG_FLT, &pid_yaw_rate_consts.kp, 0, {0}},
});
