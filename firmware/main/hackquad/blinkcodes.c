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

#include "hackquad/blinkcodes.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static u32 rate = BLCR_FAST;

static void blc_process_task(void *arg) {
    (void) arg;

    u32 counter = 0;

    // setup built in LED
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    for (;;) {
        gpio_set_level(BLINK_GPIO, counter++ & 1);
        vTaskDelay(rate);
    }
}

void blc_init() {
    xTaskCreate(blc_process_task, "blink_task", 2304, NULL, 0, NULL);
}

void blc_setrate(u32 ratel) {
    rate = ratel;
}
