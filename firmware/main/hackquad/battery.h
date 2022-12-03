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

#ifndef HACKQUAD_BATTERY_H
#define HACKQUAD_BATTERY_H

#include "hackquad/lint_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize ADC and battery reading functionality.
 */
void battery_init();

/**
 * Reads the current battery voltage.
 */
float battery_read();

#ifdef __cplusplus
}
#endif

#endif
