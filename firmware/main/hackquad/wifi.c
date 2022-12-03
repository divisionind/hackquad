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

#include <string.h>

#include "hackquad/wifi.h"
#include "esp_wifi_types.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"

#define WIFI_FAIL_FLAG    (1 << 1)
#define WIFI_SUCCESS_FLAG 1

struct hq_wifi_event_args {
    EventGroupHandle_t wifi_event_group;
    int retry_num;
};


/* REGISTRY FIELDS */
u8 wifi_mode              = WIFI_MODE_AP;
u8 wifi_ap_authmode       = WIFI_AUTH_WPA2_PSK;
u8 wifi_ap_max_connection = 4;
u8 wifi_ap_channel        = 1;
u8 wifi_st_authmode       = WIFI_AUTH_WPA2_PSK;
u8 wifi_st_max_retrys     = 3; //st_macaddr, st_channel, etc for each struct var

char wifi_ap_ssid[32] = "HackQuad" /* means default value */;
char wifi_ap_pass[64] = "division";
char wifi_st_ssid[32] = "HackQuad";
char wifi_st_pass[64] = "division";

static void sta_wifi_event_handler(struct hq_wifi_event_args *arg, esp_event_base_t event_base, s32 event_id, void *event_data) {
    ip_event_got_ip_t *event;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (arg->retry_num < wifi_st_max_retrys) {
            esp_wifi_connect();
            arg->retry_num++;
        } else {
            xEventGroupSetBits(arg->wifi_event_group, WIFI_FAIL_FLAG);
        }

        ESP_LOGI(TAG, "connect to the AP fail, retry = %i", arg->retry_num);
    } else
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        event = (ip_event_got_ip_t *) event_data;

        ESP_LOGI(TAG, "got ip = " IPSTR, IP2STR(&event->ip_info.ip));
        arg->retry_num = 0;
        xEventGroupSetBits(arg->wifi_event_group, WIFI_SUCCESS_FLAG);
    }
}

static int init_wifi_sta() {
    struct hq_wifi_event_args wifi_event_arg;
    wifi_config_t conf;
    EventBits_t status;
    esp_event_handler_instance_t any_event_handler;
    esp_event_handler_instance_t got_ip_handler;
    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();

    wifi_event_arg.wifi_event_group = xEventGroupCreate();
    wifi_event_arg.retry_num = 0;

    esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        (esp_event_handler_t) &sta_wifi_event_handler,
                                                        &wifi_event_arg,
                                                        &any_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        (esp_event_handler_t) &sta_wifi_event_handler,
                                                        &wifi_event_arg,
                                                        &got_ip_handler));

    memset(&conf, 0, sizeof(conf));
    strcpy((char *) conf.sta.ssid, wifi_st_ssid);
    strcpy((char *) conf.sta.password, wifi_st_pass);
    conf.sta.threshold.authmode = wifi_st_authmode;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));  // be sure to call this to prevent that stupid flash caching crap
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &conf));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "wifi sta mode started");

    ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(78));  // 20dBm
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));  // lowest latency

    // wait for wifi to attempt connection all x-times
    status = xEventGroupWaitBits(wifi_event_arg.wifi_event_group, WIFI_FAIL_FLAG | WIFI_SUCCESS_FLAG,
                                 pdFALSE, pdFALSE, portMAX_DELAY);

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, any_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, got_ip_handler));
    vEventGroupDelete(wifi_event_arg.wifi_event_group);

    if (status & WIFI_SUCCESS_FLAG) {
        ESP_LOGI(TAG, "connected to wifi ssid = \"%s\"", wifi_st_ssid);
        return 0;
    } else {
        ESP_LOGE(TAG, "FAILED to connect to wifi ssid = \"%s\"", wifi_st_ssid);
        return 1;
    }
}

static void ap_wifi_event_handler(void *arg, esp_event_base_t event_base, s32 event_id, void *event_data) {
    (void) arg;
    (void) event_base;

    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
    } else
    if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
    }
}

static void init_wifi_ap() {
    wifi_config_t conf;
    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();

    esp_netif_create_default_wifi_ap();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));

    // a little logging never hurt
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &ap_wifi_event_handler,
                                                        NULL,
                                                        NULL));

    memset(&conf, 0, sizeof(conf));
    memcpy(&conf.ap.ssid, wifi_ap_ssid, sizeof(conf.ap.ssid));
    memcpy(&conf.ap.password, wifi_ap_pass, sizeof(conf.ap.password));
    conf.ap.ssid_len = strlen(wifi_ap_ssid);
    conf.ap.authmode = wifi_ap_authmode;
    conf.ap.max_connection = wifi_ap_max_connection;
    conf.ap.channel = wifi_ap_channel;  // 1-13

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &conf));

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "wifi ap mode started");

    ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(78));  // 20dBm
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));  // lowest latency
}

void wifi_init() {
    // setup default stuffz
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    if (wifi_mode == WIFI_MODE_AP) {
        goto start_ap;
    }

    // start sta point
    if (!init_wifi_sta() && wifi_mode != WIFI_MODE_APSTA)
        return;

    // sta start failed, continue to start ap
    start_ap:
    init_wifi_ap();
    return;
}

/**
 * Gets the RSSI of the client connection (whether it be AP or
 * STA).
 *
 * A little crude but works fine as a proof-of-concept.
 *
 * @return rssi of some connection
 */
s8 wifi_get_rssi() {
    wifi_ap_record_t aprec;
    wifi_sta_list_t stalist;

    // return rssi to AP (only in STA mode)
    if (!esp_wifi_sta_get_ap_info(&aprec)) {
        return aprec.rssi;
    }

    // return rssi of first softAP client
    if (!esp_wifi_ap_get_sta_list(&stalist)) {
        if (stalist.num > 0) {
            return stalist.sta[0].rssi;
        }
    }

    return 0;
}
