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

#include "driver/ledc.h"
#include "esp_log.h"

#include "hackquad/motor.h"

void motor_init() {
    int i;

    // setup timer
    ledc_timer_config_t timer_conf = {
            .speed_mode      = LEDC_HIGH_SPEED_MODE,
            .duty_resolution = LEDC_TIMER_10_BIT,
            .timer_num       = MOTOR_TIMER,
            .freq_hz         = M_FREQ,
            .clk_cfg         = LEDC_USE_APB_CLK //LEDC_AUTO_CLK
    };

    // setup motors - with duty zero
    ledc_channel_config_t motor_conf[4] = {
            {
                    .gpio_num   = M0_PIN,
                    .channel    = LEDC_CHANNEL_0,
                    .speed_mode = LEDC_HIGH_SPEED_MODE,
                    .intr_type  = LEDC_INTR_DISABLE,
                    .timer_sel  = MOTOR_TIMER,
                    .duty       = 0,
                    .hpoint     = 0,
            },
            {
                    .gpio_num   = M1_PIN,
                    .channel    = LEDC_CHANNEL_1,
                    .speed_mode = LEDC_HIGH_SPEED_MODE,
                    .intr_type  = LEDC_INTR_DISABLE,
                    .timer_sel  = MOTOR_TIMER,
                    .duty       = 0,
                    .hpoint     = 0,
            },
            {
                    .gpio_num   = M2_PIN,
                    .channel    = LEDC_CHANNEL_2,
                    .speed_mode = LEDC_HIGH_SPEED_MODE,
                    .intr_type  = LEDC_INTR_DISABLE,
                    .timer_sel  = MOTOR_TIMER,
                    .duty       = 0,
                    .hpoint     = 0,
            },
            {
                    .gpio_num   = M3_PIN,
                    .channel    = LEDC_CHANNEL_3,
                    .speed_mode = LEDC_HIGH_SPEED_MODE,
                    .intr_type  = LEDC_INTR_DISABLE,
                    .timer_sel  = MOTOR_TIMER,
                    .duty       = 0,
                    .hpoint     = 0,
            },
    };

    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    for (i = 0; i < 4; i++)
        ledc_channel_config(&motor_conf[i]);

    ESP_ERROR_CHECK(ledc_fade_func_install(0));
    ESP_LOGI(TAG, "motors configured with %i-bit resolution @ %i Hz", timer_conf.duty_resolution, M_FREQ);
}

void motor_throttle(motor_index_t motor, u32 throttle) {
    // ensure normal
    //if (throttle > 1023)
    //    throttle = 1023;

    ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, motor, throttle, 0);
}

u32 motor_throttle_get(motor_index_t motor) {
    return ledc_get_duty(LEDC_HIGH_SPEED_MODE, motor);
}

