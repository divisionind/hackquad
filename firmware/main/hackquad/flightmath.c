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

#include "flightmath.h"
#include "math.h"

/* fast inverse sqrt from Quake III Arena source code */
static inline float Q_rsqrt(float number) {
    long i;
    float x2, y;
    const float threehalfs = 1.5F;

    x2 = number * 0.5F;
    y = number;
    i = *(long *) &y;                       // evil floating point bit level hacking
    i = 0x5f3759df - (i >> 1);              // what the fuck?
    y = *(float *) &i;
    y = y * (threehalfs - (x2 * y * y));    // 1st iteration
//	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

    return y;
}

void madgwick_init(madgwick_ahrs_t *ahrs, float gerr) {
    ahrs->q.w = 1.f;
    ahrs->q.x = 0.f;
    ahrs->q.y = 0.f;
    ahrs->q.z = 0.f;
    ahrs->beta = sqrtf(3.f/4.f) * (gerr * DEG_TO_RAD);
}

void madgwick_update(madgwick_ahrs_t *q, float dt, float ax, float ay, float az, float gx, float gy, float gz) {
    float q0 = q->q.w, q1 = q->q.x, q2 = q->q.y, q3 = q->q.z;

    float norm;
    float SEqDot_omega_1, SEqDot_omega_2, SEqDot_omega_3, SEqDot_omega_4;   // quaternion delta from gyroscopes elements
    float f_1, f_2, f_3;                                                    // objective function elements
    float J_11or24, J_12or23, J_13or22, J_14or21, J_32, J_33;               // objective function Jacobian elements
    float SEqHatDot_1, SEqHatDot_2, SEqHatDot_3, SEqHatDot_4;               // estimated direction of the gyroscope error

    // Auxiliary variables to avoid repeated calculations
    float halfq0 = 0.5f * q0;
    float halfq1 = 0.5f * q1;
    float halfq2 = 0.5f * q2;
    float halfq3 = 0.5f * q3;
    float x2q0 = 2.0f * q0;
    float x2q1 = 2.0f * q1;
    float x2q2 = 2.0f * q2;

    // Normalise the accelerometer measurement
    norm = Q_rsqrt(ax * ax + ay * ay + az * az);
    ax *= norm;
    ay *= norm;
    az *= norm;
    // Compute the objective function and Jacobian
    f_1 = x2q1 * q3 - x2q0 * q2 - ax;
    f_2 = x2q0 * q1 + x2q2 * q3 - ay;
    f_3 = 1.0f - x2q1 * q1 - x2q2 * q2 - az;
    J_11or24 = x2q2; // J_11 negated in matrix multiplication
    J_12or23 = 2.0f * q3;
    J_13or22 = x2q0; // J_12 negated in matrix multiplication
    J_14or21 = x2q1;
    J_32 = 2.0f * J_14or21; // negated in matrix multiplication
    J_33 = 2.0f * J_11or24; // negated in matrix multiplication
    // Compute the gradient (matrix multiplication)
    SEqHatDot_1 = J_14or21 * f_2 - J_11or24 * f_1;
    SEqHatDot_2 = J_12or23 * f_1 + J_13or22 * f_2 - J_32 * f_3;
    SEqHatDot_3 = J_12or23 * f_2 - J_33 * f_3 - J_13or22 * f_1;
    SEqHatDot_4 = J_14or21 * f_1 + J_11or24 * f_2;
    // Normalise the gradient
    norm = Q_rsqrt(SEqHatDot_1 * SEqHatDot_1 + SEqHatDot_2 * SEqHatDot_2 + SEqHatDot_3 * SEqHatDot_3 +
                   SEqHatDot_4 * SEqHatDot_4);
    SEqHatDot_1 *= norm;
    SEqHatDot_2 *= norm;
    SEqHatDot_3 *= norm;
    SEqHatDot_4 *= norm;
    // Compute the quaternion delta measured by gyroscopes
    SEqDot_omega_1 = -halfq1 * gx - halfq2 * gy - halfq3 * gz;
    SEqDot_omega_2 = halfq0 * gx + halfq2 * gz - halfq3 * gy;
    SEqDot_omega_3 = halfq0 * gy - halfq1 * gz + halfq3 * gx;
    SEqDot_omega_4 = halfq0 * gz + halfq1 * gy - halfq2 * gx;
    // Compute then integrate the estimated quaternion delta
    q0 += (SEqDot_omega_1 - (q->beta * SEqHatDot_1)) * dt;
    q1 += (SEqDot_omega_2 - (q->beta * SEqHatDot_2)) * dt;
    q2 += (SEqDot_omega_3 - (q->beta * SEqHatDot_3)) * dt;
    q3 += (SEqDot_omega_4 - (q->beta * SEqHatDot_4)) * dt;
    // Normalise quaternion
    norm = Q_rsqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 *= norm;
    q1 *= norm;
    q2 *= norm;
    q3 *= norm;

    q->q.w = q0;
    q->q.x = q1;
    q->q.y = q2;
    q->q.z = q3;
}

/*
 * Quaternions:
 *  - https://www.cprogramming.com/tutorial/3d/quaternions.html
 *  - http://www.euclideanspace.com/maths/algebra/realNormedAlgebra/quaternions/transforms/index.htm
 */

// ypr gravity
void quaternion_get_gravity(quaternion_t *q, vec3f_t *g) {
    g->x = 2.f * (q->x * q->z - q->w * q->y);
    g->y = 2.f * (q->w * q->x + q->y * q->z);
    g->z = q->w * q->w - q->x * q->x - q->y * q->y + q->z * q->z;
}

void quaternion_euler(quaternion_t *q, vec3f_t *g, vec3f_t *rpy) {
    rpy->x = atan2f(g->y, g->z);
    rpy->y = -atan2f(g->x, (1.f / Q_rsqrt(g->y * g->y + g->z * g->z))); // negative to match ref frame of old euler code
    rpy->z = atan2f(2.f * q->x * q->y - 2.f * q->w * q->z, 2.f * q->w * q->w + 2.f * q->x * q->x - 1.f);

    if (g->z < 0) {
        if (rpy->y > 0) {
            rpy->y = (float) M_PI - rpy->y;
        } else {
            rpy->y = (float) -M_PI - rpy->y;
        }
    }

    rpy->x *= RAD_TO_DEG;
    rpy->y *= RAD_TO_DEG;
    rpy->z *= RAD_TO_DEG;
}

/*void quaternion_raw_euler(quaternion_t *q, vec3f_t *rpy) {
    rpy->x = atan2f(2.f * q->x * q->y - 2.f * q->w * q->z, 2.f * q->w * q->w + 2.f * q->x * q->x - 1.f);
    rpy->y = -asinf(2.f * q->x * q->z + 2.f * q->w * q->y);
    rpy->z = atan2f(2.f * q->y * q->z - 2.f * q->w * q->x, 2.f * q->w * q->w + 2.f * q->z * q->z - 1.f);
}*/

void quaternion_rotate_radians(quaternion_t *q, vec3f_t *r) {
    quaternion_t rotq;
    rotq.w = cosf(r->x/2.f);
    //rotq.x =
}

void quaternion_multiply(quaternion_t *qin, quaternion_t *by, quaternion_t *ret) {
    //ret->w =
}

// yea no this isn't going to work even for small angles the rotation order is all wrong
// ang in radians
void vec3f_trig_rotate(vec3f_t *in, vec3f_t *out, vec3f_t *ang) {
    out->x = in->x * (cosf(ang->theta) * cosf(ang->psi)) + in->y * (cosf(ang->theta) * sinf(ang->psi)) - in->z * sinf(ang->theta);
    out->y = in->x * (sinf(ang->phi) * sinf(ang->theta) * cosf(ang->psi) - sinf(ang->psi) * cosf(ang->theta)) +
             in->y * (sinf(ang->psi) * sinf(ang->phi) * sinf(ang->theta) + cosf(ang->psi) * cosf(ang->phi)) +
             in->z * sinf(ang->phi) * cosf(ang->theta);
    out->z = in->x * (cosf(ang->phi) * sinf(ang->theta) * cosf(ang->psi) + sinf(ang->psi) * sinf(ang->phi)) +
             in->y * (cosf(ang->phi) * sinf(ang->theta) * sinf(ang->psi) - sinf(ang->phi) * cosf(ang->psi)) +
             in->z * (cosf(ang->phi) * cosf(ang->theta));
}