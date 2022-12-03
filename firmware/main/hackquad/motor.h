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

#ifndef HACKQUAD_MOTOR_H
#define HACKQUAD_MOTOR_H

#include "lint_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M0_PIN 2
#define M1_PIN 12
#define M2_PIN 32
#define M3_PIN 13

// ~= -10% throttle reserved for stab
#define MOTOR_MAX_THROTTLE  930.f
#define MOTOR_TIMER        LEDC_TIMER_1

/*
 * try to use >5kHz to reduce the impedance of the utilized bypass caps to motor noise
 *   (best would be ~1Mhz) MAX: 78125 Hz @ 10-bits e.g. 80MHz/2**bits
 * however, the fets have a hard time switching at anything much higher than 5kHz so this is where we are
 * i reduced the gate-cap drain resistor to improve switching but i have not tested it
 */
#define M_FREQ 5000

typedef enum _motor_index {
    M0 = 0,
    M1,
    M2,
    M3
} motor_index_t;

void motor_init();

void motor_throttle(motor_index_t motor, u32 throttle);
u32 motor_throttle_get(motor_index_t motor);

#ifdef __cplusplus
}
#endif

#endif /* HACKQUAD_MOTOR_H */
