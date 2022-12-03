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

#ifndef HACKQUAD_FLIGHTMATH_H
#define HACKQUAD_FLIGHTMATH_H

#ifdef __cplusplus
extern "C" {
#endif

#define RAD_TO_DEG (180.f / (float) M_PI)
#define DEG_TO_RAD ((float) M_PI / 180.f)

typedef struct {
    union {
        float q[4];
        struct {
            float w, x, y, z;
        };
    };
} quaternion_t;

typedef struct {
    quaternion_t q;
    float beta, rbeta;
    int rest:1;
} madgwick_ahrs_t;

typedef struct {
    union {
        float v[3];
        struct {
            float x, y, z;
        };
        struct {
            float phi, theta, psi;
        };
    };
} vec3f_t;

void vec3f_trig_rotate(vec3f_t *in, vec3f_t *out, vec3f_t *ang);

/**
 *
 * @param ahrs
 * @param gerr estimated deg/s gyroscope error
 */
void madgwick_init(madgwick_ahrs_t *ahrs, float gerr);

/**
 * Based off: https://x-io.co.uk/downloads/madgwick_internal_report.pdf
 * @param q
 * @param dt sense last update
 * @param ax unitless acceleration
 * @param ay
 * @param az
 * @param gx rad/s gyro rate
 * @param gy
 * @param gz
 */
void madgwick_update(madgwick_ahrs_t *q, float dt, float ax, float ay, float az, float gx, float gy, float gz);

void quaternion_get_gravity(quaternion_t *q, vec3f_t *g);
void quaternion_euler(quaternion_t *q, vec3f_t *g, vec3f_t *rpy);

#ifdef __cplusplus
}
#endif

#endif /*HACKQUAD_FLIGHTMATH_H*/
