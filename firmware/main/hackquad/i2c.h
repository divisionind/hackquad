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

#ifndef HACKQUAD_I2C_H
#define HACKQUAD_I2C_H

#include "hackquad/lint_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_MAX_WAIT (100 / portTICK_RATE_MS)
#define I2C_BUS0_FRQ 400000                   /* 400kHz - fast mode */
#define I2C_BUS0_SDA 14
#define I2C_BUS0_SCL 15

typedef struct {
    int port;
    u8 address;
} iic_device_t;

int iic_init(int port, int sda_pin, int scl_pin, u32 freq);

int iic_readl(int port, u8 address, u8 reg, u8 *buffer, size_t len);
int iic_writel(int port, u8 address, u8 reg, u8 data);
int iic_write(iic_device_t *ctx, u8 reg, u8 data);
int iic_read(iic_device_t *ctx, u8 reg, u8 *buffer, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* HACKQUAD_I2C_H */
