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

#ifndef HACKQUAD_MSG_H
#define HACKQUAD_MSG_H

#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

/* main flight controller update bits */
#define HQMSG_MPU_UPDATE  (1)
#define HQMSG_CTRL_UPDATE (1 << 1)

extern TaskHandle_t task_hackquad_main;

#ifdef __cplusplus
}
#endif

#endif /* HACKQUAD_MSG_H */
