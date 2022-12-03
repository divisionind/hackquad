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

#ifndef HACKQUAD_WIFI_H
#define HACKQUAD_WIFI_H

#include "hackquad/lint_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* REGISTRY FIELDS */
extern u8 wifi_mode;              // wifi_mode_t
extern u8 wifi_ap_authmode;       // wifi_auth_mode_t
extern u8 wifi_ap_max_connection;
extern u8 wifi_ap_channel;
extern u8 wifi_st_authmode;       // wifi_auth_mode_t
extern u8 wifi_st_max_retrys;

extern char wifi_ap_ssid[32];
extern char wifi_ap_pass[64];
extern char wifi_st_ssid[32];
extern char wifi_st_pass[64];

void wifi_init();
s8 wifi_get_rssi();

#ifdef __cplusplus
}
#endif

#endif /* HACKQUAD_WIFI_H */
