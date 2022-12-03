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

#ifndef HACKQUAD_MPU_H
#define HACKQUAD_MPU_H

#include "hackquad/i2c.h"
#include "hackquad/lint_defs.h"
#include "esp_attr.h"
#include "flightmath.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MPU_BUS  0
#define MPU_ADDR 0x68
#define MPU_INT  38

#define MPU_CALIBRATION_ITERATIONS 4269
/*
#define MPU_CAL_ERROR_BASE  0x1000
#define MPU_CAL_ERROR_
*/

struct mpu_data {
    madgwick_ahrs_t ahrs;
    vec3f_t angle, rate;
    vec3f_t raw_acc, raw_gyr;
};

extern u8 mpu_has_calibration;
extern float mpu_gyroffset_x;
extern float mpu_gyroffset_y;
extern float mpu_gyroffset_z;
extern float mpu_accoffset_x;
extern float mpu_accoffset_y;
extern float mpu_accoffset_z;

extern struct mpu_data mpu_latest;

int mpu_init();
void mpu_read(float dt);
void mpu_calibrate(); // DONT USE

#ifdef __cplusplus
}
#endif

#endif
