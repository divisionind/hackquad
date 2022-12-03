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

#include "hackquad/pid.h"

float pid_update(struct pid_ctx *ctx, float set, float actual, float dt) {
    float error = actual - set;
    float output;

    // only integrate if significant error
    if (fabsf(error) > ctx->kons->epsilon)
        ctx->integral += error * dt;

    output = (ctx->kons->kp * error) +                               // P
             (ctx->kons->ki * ctx->integral) +                       // I
             (ctx->kons->kd * ((error - ctx->previous_error) / dt)); // D
    ctx->previous_error = error;

    return output;
}
