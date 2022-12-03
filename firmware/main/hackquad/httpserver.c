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

#include <stdbool.h>

#include "hackquad/httpserver.h"
#include "hackquad/lint_defs.h"
#include "hackquad/registry.h"
#include "hackquad/mpu.h"
#include "hackquad/blinkcodes.h"
#include "esp_log.h"
#include "assert.h"
#include "esp_http_server.h"
#include "cJSON.h"

/*
 * Only reason this isn't just a TCP socket is I originally planned on making the UI as a webapp
 * running on the HackQuad.
 */

static char *heap;
static httpd_handle_t server;

static void reg_addvalue_tojson(struct reg_entry *ent, cJSON *json) {
    double num_val;

    switch (ent->type) {
        case REG_8B:
            num_val = (double) *((u8 *) ent->location);
            goto add_num;
        case REG_16B:
            num_val = (double) *((u16 *) ent->location);
            goto add_num;
        case REG_32B:
            num_val = (double) *((u32 *) ent->location);
            goto add_num;
        case REG_64B:
            num_val = (double) *((u64 *) ent->location);
            goto add_num;
        case REG_STR:
            break;
        case REG_FLT:
            num_val = (double) *((float *) ent->location);
            goto add_num;
        default:
            return;
    }

    cJSON_AddStringToObject(json, "value", (char *) ent->location);
    return;

    add_num:
    cJSON_AddNumberToObject(json, "value", num_val);
}

static void http_recv(httpd_req_t *req) {
    size_t read, offset;

    if (req->content_len > HTTPSERVER_HEAP_SIZE) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content length too long");
        return;
    }

    offset = 0;
    do {
        read = httpd_req_recv(req, heap + offset, req->content_len);

        if (read <= 0) {
            // 500 invalid read
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "failed to read data from request");
            return;
        }

        offset += read;
    } while (offset < req->content_len);

    heap[req->content_len] = '\0';
}

/* curl --header "Content-Type: application/json" --request GET --data {\"key\":\"WIFI_ST_SSID\"} http://hackquad.local/reg/get */
static int handler_reg_get(httpd_req_t *req) {
    cJSON *root, *out, *key;
    struct reg_entry *reg;
    int ret;

    http_recv(req);
    root = cJSON_Parse(heap);

    key = cJSON_GetObjectItemCaseSensitive(root, "key");
    if (!cJSON_IsString(key)) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "no supplied key");
        ret = -1;
        goto error;
    }

    reg = reg_lookup(key->valuestring);
    if (!reg) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "could not find key by supplied name");
        ret = -1;
        goto error;
    }

    out = cJSON_CreateObject();
    cJSON_AddNumberToObject(out, "type", reg->type);
    reg_addvalue_tojson(reg, out);

    cJSON_PrintPreallocated(out, heap, HTTPSERVER_HEAP_SIZE, false);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, (char *) heap);

    cJSON_Delete(out);
    ret = 0;
    error:
    cJSON_Delete(root);
    return ret;
}

/* curl --header "Content-Type: application/json" --request POST --data {\"key\":\"PID_ANGLE_KP\",\"value\":2.06969} http://hackquad.local/reg/set */
static int handler_reg_set(httpd_req_t *req) {
    cJSON *reqjson, *value, *key;
    struct reg_entry *reg;
    int ret;

    http_recv(req);
    reqjson = cJSON_Parse(heap);

    key = cJSON_GetObjectItemCaseSensitive(reqjson, "key");
    value = cJSON_GetObjectItemCaseSensitive(reqjson, "value");
    if (!cJSON_IsString(key) || value == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "key or value not supplied");
        ret = -1;
        goto error;
    }

    reg = reg_lookup(key->valuestring);
    if (!reg) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "could not find key by supplied name");
        ret = -1;
        goto error;
    }

    switch (reg->type) {
        default:
            break;
        case REG_8B:
            *((u8 *) reg->location) = (u8) value->valueint;
            break;
        case REG_16B:
            *((u16 *) reg->location) = (u16) value->valueint;
            break;
        case REG_32B:
        case REG_64B: // too much effort for 64-bit data types
            *((u32 *) reg->location) = value->valueint;
            break;
        case REG_STR:
            strcpy(reg->location, value->valuestring);
            break;
        case REG_FLT:
            *((float *) reg->location) = (float) value->valuedouble;
    }

    // ensure changes saved
    reg_write(key->valuestring);

    httpd_resp_sendstr(req, "ok");
    ret = 0;
    error:
    cJSON_Delete(reqjson);
    return ret;
}

static int handler_reg_list(httpd_req_t *req) {
    cJSON *out, *ent;
    size_t i;

    out = cJSON_CreateArray();
    for (i = 0; i < registry_len; i++) {
        ent = cJSON_CreateObject();
        cJSON_AddItemToArray(out, ent);

        cJSON_AddStringToObject(ent, "key", registry[i].key);
        cJSON_AddNumberToObject(ent, "type", registry[i].type);
        //cJSON_AddStringToObject(ent, "type", reg_type_tostr(registry[i].type));
        reg_addvalue_tojson(&registry[i], ent);
    }

    cJSON_PrintPreallocated(out, heap, HTTPSERVER_HEAP_SIZE, false);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, (char *) heap);

    cJSON_Delete(out);
    return 0;
}

static void _mpu_calibrate_task(void *arg) {
    (void) arg;

    blc_setrate(BLCR_VERY_FAST);
    mpu_calibrate();
    blc_setrate(BLCR_NORMAL);

    // ensure calibration is saved
    if (mpu_has_calibration) {
        reg_write("MPU_HAS_CALIBRATION");
        reg_write("MPU_GYROFFSET_X");
        reg_write("MPU_GYROFFSET_Y");
        reg_write("MPU_GYROFFSET_Z");
        reg_write("MPU_ACCOFFSET_X");
        reg_write("MPU_ACCOFFSET_Y");
        reg_write("MPU_ACCOFFSET_Z");

        ESP_LOGI(TAG, "saved mpu calibration to registry");
    }

    vTaskDelete(NULL);
}

/* curl --request POST http://hackquad.local/mpu/calibrate */
static int handler_mpu_calibrate(httpd_req_t *req) {
    httpd_resp_sendstr(req, "ok");
    xTaskCreate(_mpu_calibrate_task, "mpu_calibrate", 2048, NULL, 0, NULL);
    return 0;
}

static int handler_index(httpd_req_t *req) {
    httpd_resp_sendstr(req, "HackQuad running");
    return 0;
}

static inline int http_add(const char *uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *r)) {
    httpd_uri_t entry;

    entry.uri = uri;
    entry.method = method;
    entry.handler = handler;
    entry.user_ctx = NULL;

    return httpd_register_uri_handler(server, &entry);
}

int http_init() {
    esp_err_t ret;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    assert(heap == NULL);
    //config.uri_match_fn = httpd_uri_match_wildcard;

    // allocate mem for http recvs
    heap = malloc(HTTPSERVER_HEAP_SIZE);
    if (!heap) {
        ESP_LOGE(TAG, "out of memory");
        return ESP_FAIL;
    }

    if ((ret = httpd_start(&server, &config))) {
        return ret;
    }

    // init handlers
    http_add("/reg/get", HTTP_GET, handler_reg_get);
    http_add("/reg/set", HTTP_POST, handler_reg_set);
    http_add("/reg/list", HTTP_GET, handler_reg_list);
    http_add("/mpu/calibrate", HTTP_POST, handler_mpu_calibrate);
    http_add("/", HTTP_GET, handler_index);

    return ESP_OK;
}
