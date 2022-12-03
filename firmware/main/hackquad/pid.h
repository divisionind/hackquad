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

#ifndef HACKQUAD_PID_H
#define HACKQUAD_PID_H

#ifdef __cplusplus
extern "C" {
#endif

struct pid_ctx {
    struct pid_kon *kons;

    /* private use */
    float integral, previous_error;
};

struct pid_kon {
    /* all constant values */
    float kp, ki, kd, epsilon;
};

float pid_update(struct pid_ctx *ctx, float set, float actual, float dt);

#ifdef __cplusplus
}
#endif

#endif /* HACKQUAD_PID_H */
