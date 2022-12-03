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
#include <string.h>
#include <stdbool.h>

#include "hackquad/mpu.h"
#include "hackquad/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "hackquad/hackquad_msg.h"

/* 0=madgwick / 1=adaptive_comp_filter */
#define ANGLE_MODE 0

/* r/w macros for i2c */
#define iicw(reg, data)      iic_write(&mpu, reg, data)
#define iicr(reg, buff, len) iic_read(&mpu, reg, buff, len)

#define MPU_GYR_COMPFILTER0 0.7f
#define MPU_GYR_COMPFILTER1 0.3f

/*
 * ACC: 1 = +/-4g (8192 LSB/g) | 2 = +/-8g (4096 LSB/g) | 3 = +/-16g (2048 LSB/g)
 * GYR: 1 = +/-500 dps (65.5 LSB/dps) | 2 = +/-1000 dps (32.8 LSB/dps) | 3 = +/-2000 dps (16.4 LSB/dps)
 */
#define ACC_RANGE_SEL 2
#define ACC_LSB       4096.0f
#define GYR_RANGE_SEL 1
#define GYR_LSB       65.5f

/* gravitational acceleration in m/s^2 */
#define G_ACCL 9.80665f
#define ACC_TO_MSS ((1.0f / ACC_LSB) * G_ACCL)

#define MADGWICK_GYRO_ERR 5.f

/* REGISTRY */
u8 mpu_has_calibration = 0;
float mpu_gyroffset_x  = 0.0f;
float mpu_gyroffset_y  = 0.0f;
float mpu_gyroffset_z  = 0.0f;
float mpu_accoffset_x  = 0.0f;
float mpu_accoffset_y  = 0.0f;
float mpu_accoffset_z  = 0.0f;

struct mpu_data mpu_latest;

static iic_device_t mpu = {
    .address = MPU_ADDR,
    .port    = MPU_BUS
};

static inline void _mpu_read_raw() {
    u8  buff[14];
    s16 raw_acc16[3];
    s16 raw_gyr16[3];

    iicr(0x3B /* ACCEL_OUT */, buff, sizeof(buff)); // p29

    raw_acc16[0] = (buff[0] << 8) | buff[1];
    raw_acc16[1] = (buff[2] << 8) | buff[3];
    raw_acc16[2] = (buff[4] << 8) | buff[5];

    raw_gyr16[0] = (buff[8]  << 8) | buff[9];
    raw_gyr16[1] = (buff[10] << 8) | buff[11];
    raw_gyr16[2] = (buff[12] << 8) | buff[13];

    // dont need to divide raw_acc by LSB if only using values for angle calculations
    // this is because these calculations are interested in the distribution
    // of gravitational acceleration amongst the axes (e.g. they are unitless)
    mpu_latest.raw_acc.x = (float) raw_acc16[0] * ACC_TO_MSS;
    mpu_latest.raw_acc.y = (float) raw_acc16[1] * ACC_TO_MSS;
    mpu_latest.raw_acc.z = (float) raw_acc16[2] * ACC_TO_MSS;

    mpu_latest.raw_gyr.x = (float) raw_gyr16[0] / GYR_LSB;
    mpu_latest.raw_gyr.y = (float) raw_gyr16[1] / GYR_LSB;
    mpu_latest.raw_gyr.z = (float) raw_gyr16[2] / GYR_LSB;
}

#if ANGLE_MODE == 0
void mpu_read(float dt) {
    _mpu_read_raw();

    // update quaternion from rotation during elapsed time
    // TODO converges very slowly after crash, inc beta during rest to inc conv
    madgwick_update(&mpu_latest.ahrs, dt, mpu_latest.raw_acc.x, mpu_latest.raw_acc.y, mpu_latest.raw_acc.z,
                    mpu_latest.raw_gyr.x * DEG_TO_RAD, mpu_latest.raw_gyr.y * DEG_TO_RAD, mpu_latest.raw_gyr.z * DEG_TO_RAD);

    // take 70% of current accepted gyr and add it to 30% of new-raw gyr
    mpu_latest.rate.x = mpu_latest.rate.x * MPU_GYR_COMPFILTER0 + mpu_latest.raw_gyr.x * MPU_GYR_COMPFILTER1;
    mpu_latest.rate.y = mpu_latest.rate.y * MPU_GYR_COMPFILTER0 + mpu_latest.raw_gyr.y * MPU_GYR_COMPFILTER1;
    mpu_latest.rate.z = mpu_latest.rate.z * MPU_GYR_COMPFILTER0 + mpu_latest.raw_gyr.z * MPU_GYR_COMPFILTER1;

    // rpy angle
    vec3f_t gravity;
    quaternion_get_gravity(&mpu_latest.ahrs.q, &gravity);
    quaternion_euler(&mpu_latest.ahrs.q, &gravity, &mpu_latest.angle);
}
#elif ANGLE_MODE == 1
void mpu_read(float dt) {
    _mpu_read_raw();

    float comp_gyr, comp_acc = 0.006f, acc_err;
    const float comp_gain = 4.f;

    // error from g-vector
    acc_err = (sqrtf(mpu_latest.raw_acc.x*mpu_latest.raw_acc.x + mpu_latest.raw_acc.y*mpu_latest.raw_acc.y + mpu_latest.raw_acc.z*mpu_latest.raw_acc.z) / G_ACCL) - 1.f;
    acc_err = sqrtf(acc_err*acc_err);
    comp_acc *= 1.f / expf(acc_err*comp_gain);
    comp_gyr = 1.f - comp_acc;

    mpu_latest.angle.x = (mpu_latest.angle.x + mpu_latest.raw_gyr.x * dt) * comp_gyr +
                         ((atan2f(mpu_latest.raw_acc.y, mpu_latest.raw_acc.z) * RAD_TO_DEG) * comp_acc);
    mpu_latest.angle.y = (mpu_latest.angle.y + mpu_latest.raw_gyr.y * dt) * comp_gyr +
                         ((atanf(-mpu_latest.raw_acc.x / sqrtf(mpu_latest.raw_acc.y * mpu_latest.raw_acc.y + mpu_latest.raw_acc.z * mpu_latest.raw_acc.z)) * RAD_TO_DEG) * comp_acc);

    // filter gyro rates
    mpu_latest.rate.x = mpu_latest.rate.x * MPU_GYR_COMPFILTER0 + mpu_latest.raw_gyr.x * MPU_GYR_COMPFILTER1;
    mpu_latest.rate.y = mpu_latest.rate.y * MPU_GYR_COMPFILTER0 + mpu_latest.raw_gyr.y * MPU_GYR_COMPFILTER1;
    mpu_latest.rate.z = mpu_latest.rate.z * MPU_GYR_COMPFILTER0 + mpu_latest.raw_gyr.z * MPU_GYR_COMPFILTER1;
}
#endif

static void IRAM_ATTR _mpu_isr_handler(void *arg) {
    (void) arg;

    BaseType_t higher_priority_taskwoken /*= pdFALSE*/;

    //prvClearInterrupt(); // auto-clr

    // notify task of mpu update
    xTaskNotifyFromISR(task_hackquad_main, HQMSG_MPU_UPDATE, eSetBits, &higher_priority_taskwoken);

    // request ctx switch from kernel to notified task. for speeeeddd
    // assuming higher_priority_taskwoken always true, because it should be
    portYIELD_FROM_ISR();
}

int mpu_init() {
    u8 whoami;
    gpio_config_t isrconf;

    ESP_LOGI(TAG, "initializing mpu...");

    madgwick_init(&mpu_latest.ahrs, MADGWICK_GYRO_ERR);

    iicw(0x6B /* PWR_MGMT_1 */, 1 << 7 /* RESET */);
    vTaskDelay(120 / portTICK_PERIOD_MS);

    iicw(0x6B /* PWR_MGMT_1 */, 1 /* CLKSEL PLL gyro_x */);
    vTaskDelay(15 / portTICK_PERIOD_MS);
    iicw(0x1B /* GYRO_CONFIG */, (GYR_RANGE_SEL << 3) /* 1 = FS_SEL 500dps */);
    iicw(0x1C /* ACCEL_CONFIG */, (ACC_RANGE_SEL << 3) /* 2 = AFS_SEL 8g | 1 = AFS_SEL 4g */);
    iicw(0x1A /* CONFIG */, 3 /* DLPF_44_42 */);

    iicw(0x37 /* INT_PIN_CFG */, 1 << 4 /* clear INT_STATUS by any read */);
    iicw(0x38 /* INT_ENABLE */, 1 /* DATA_RDY_EN */);

    iicr(0x75 /* WHO_AM_I */, &whoami, 1);

    if (whoami == 0x68) {
        memset(&isrconf, 0, sizeof(isrconf));
        isrconf.pull_down_en = 1;
        isrconf.mode = GPIO_MODE_INPUT;
        isrconf.intr_type = GPIO_INTR_POSEDGE;
        isrconf.pin_bit_mask = 1ull << MPU_INT;

        ESP_ERROR_CHECK(gpio_reset_pin(MPU_INT));
        ESP_ERROR_CHECK(gpio_config(&isrconf));

        ESP_ERROR_CHECK(gpio_install_isr_service(0));
        ESP_ERROR_CHECK(gpio_isr_handler_add(MPU_INT, _mpu_isr_handler, NULL));

        return 0;
    } else
        return whoami;
}

static void IRAM_ATTR _mpu_calibrate_isr_handler(EventGroupHandle_t event) {
    BaseType_t higher_priority_taskwoken /*= pdFALSE*/;

    xEventGroupSetBitsFromISR(event, 1, &higher_priority_taskwoken);
    portYIELD_FROM_ISR();
}

// TODO skew correction calibration routine, i made this one for leveling relative to a surface (which isn't needed w/ the current angle methods)
void mpu_calibrate() {
    EventGroupHandle_t mpu_event;
    EventBits_t flags;
    unsigned int i;

    // remove old isr while calibrating to prevent mpu updates to main loop
    if (gpio_isr_handler_remove(MPU_INT))
        return;

    mpu_event = xEventGroupCreate();
    if (!mpu_event)
        goto cleanup;

    // add temp mpu isr
    if (gpio_isr_handler_add(MPU_INT, (gpio_isr_t) _mpu_calibrate_isr_handler, mpu_event))
        goto cleanup;

    // for each update of mpu, read data and calculate offsets
    for (i = 0; i < MPU_CALIBRATION_ITERATIONS; i++) {
        flags = xEventGroupWaitBits(mpu_event, 1, true, true, 100);

        if (flags & 1) {
            // mpu updated, read data
            _mpu_read_raw();

            mpu_gyroffset_x += mpu_latest.raw_gyr.x;
            mpu_gyroffset_y += mpu_latest.raw_gyr.y;
            mpu_gyroffset_z += mpu_latest.raw_gyr.z;

            mpu_accoffset_x += mpu_latest.raw_acc.x;
            mpu_accoffset_y += mpu_latest.raw_acc.y;
            mpu_accoffset_z += mpu_latest.raw_acc.z;
        } else
            goto cleanup; // timeout
    }

    // calculate final offset
    mpu_gyroffset_x /= (float) MPU_CALIBRATION_ITERATIONS;
    mpu_gyroffset_y /= (float) MPU_CALIBRATION_ITERATIONS;
    mpu_gyroffset_z /= (float) MPU_CALIBRATION_ITERATIONS;

    mpu_accoffset_x /= (float) MPU_CALIBRATION_ITERATIONS;
    mpu_accoffset_y /= (float) MPU_CALIBRATION_ITERATIONS;
    mpu_accoffset_z = (mpu_accoffset_z / (float) MPU_CALIBRATION_ITERATIONS) + G_ACCL; // should be 1g

    mpu_has_calibration = true;
    ESP_LOGI(TAG, "mpu calibrated! gyr0 = %.2f, gyr1 = %.2f, gyr2 = %.2f, acc0 = %.2f, acc1 = %.2f, acc2 = %.2f", mpu_gyroffset_x,
             mpu_gyroffset_y, mpu_gyroffset_z, mpu_accoffset_x, mpu_accoffset_y, mpu_accoffset_z);

    cleanup:
    vEventGroupDelete(mpu_event);
    gpio_isr_handler_remove(MPU_INT);
    ESP_ERROR_CHECK(gpio_isr_handler_add(MPU_INT, _mpu_isr_handler, NULL));
}
