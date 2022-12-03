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

#ifndef HACKQUAD_BLINKCODES_H
#define HACKQUAD_BLINKCODES_H

#include "freertos/FreeRTOS.h"
#include "hackquad/lint_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLINK_GPIO 9

#define BLCR_BASE(_x) (_x / portTICK_PERIOD_MS)

#define BLCR_SLOW       BLCR_BASE(1000)
#define BLCR_NORMAL     BLCR_BASE(500)
#define BLCR_FAST       BLCR_BASE(200)
#define BLCR_VERY_FAST  BLCR_BASE(100)

/*
 * Sets the current blink rate
 */
void blc_setrate(u32 rate);

/**
 * Initializes the blink led.
 *
 * Originally I intended to make a utility for blinking out morse code messages
 * for error codes. However, while cool, practically speaking there are a lot
 * better methods for debugging.
 */
void blc_init();

#ifdef __cplusplus
}
#endif

#endif /* HACKQUAD_BLINKCODES_H */
