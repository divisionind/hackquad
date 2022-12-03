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

#include "driver/i2c.h"
#include <string.h>

#include "hackquad/i2c.h"

int iic_init(int port, int sda_pin, int scl_pin, u32 freq) {
    i2c_config_t conf;

    memset(&conf, 0, sizeof(conf));
    conf.mode = I2C_MODE_MASTER;
    conf.sda_pullup_en = 1;
    conf.scl_pullup_en = 1;
    conf.sda_io_num = sda_pin;
    conf.scl_io_num = scl_pin;
    conf.master.clk_speed = freq;

    esp_err_t ret = i2c_param_config(port, &conf);
    if (ret)
        return ret;

    return i2c_driver_install(port, I2C_MODE_MASTER, 0, 0, 0);
}

int iic_readl(int port, u8 address, u8 reg, u8 *buffer, size_t len) {
    i2c_cmd_handle_t ctx;
    int ret;

    /*
     * -----------------------------------------------------
     * | S | slave_address + W:0 | ACK | reg_address | ACK |
     * -----------------------------------------------------
     */
    ctx = i2c_cmd_link_create();
    i2c_master_start(ctx);
    i2c_master_write_byte(ctx, (address << 1) | I2C_MASTER_WRITE, 1);
    i2c_master_write_byte(ctx, reg, 1);

    /*
     * -------------------------------------------------------------------------
     * | S | slave_address + R:1 | ACK || READ0 | ACK ||...|| READN | NACK | P |
     * -------------------------------------------------------------------------
     */
    i2c_master_start(ctx);
    i2c_master_write_byte(ctx, (address << 1) | I2C_MASTER_READ, 1);
    i2c_master_read(ctx, buffer, len, I2C_MASTER_ACK);
    i2c_master_stop(ctx);

    // send i2c cmd queue
    ret = i2c_master_cmd_begin(port, ctx, I2C_MAX_WAIT);
    i2c_cmd_link_delete(ctx);

    return ret;
}

int iic_writel(int port, u8 address, u8 reg, u8 data) {
    i2c_cmd_handle_t ctx;
    int ret;

    ctx = i2c_cmd_link_create();
    i2c_master_start(ctx);
    i2c_master_write_byte(ctx, (address << 1) | I2C_MASTER_WRITE, 1);
    i2c_master_write_byte(ctx, reg, 1);

    // NOTE: could loop over bytes you wish to write and queue them up here (if your slave has support for this)
    i2c_master_write_byte(ctx, data, 1);
    i2c_master_stop(ctx);

    // send i2c cmd queue
    ret = i2c_master_cmd_begin(port, ctx, I2C_MAX_WAIT);
    i2c_cmd_link_delete(ctx);

    return ret;
}

int iic_write(iic_device_t *ctx, u8 reg, u8 data) {
    return iic_writel(ctx->port, ctx->address, reg, data);
}

int iic_read(iic_device_t *ctx, u8 reg, u8 *buffer, size_t len) {
    return iic_readl(ctx->port, ctx->address, reg, buffer, len);
}
