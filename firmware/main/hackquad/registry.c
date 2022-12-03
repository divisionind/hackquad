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

#include "registry.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"

#define NVS_NAMESPACE "hq_registry"

/* method taken from Java src */
int reg_calc_hashcode(const char *str) {
    int hash = 0;
    size_t len = strlen(str);

    for (int i = 0; i < len; i++)
        hash = 31 * hash + str[i];

    return hash;
}

/* method taken from https://johnnylee-sde.github.io/Fast-unsigned-integer-to-hex-string/ */
void reg_hashcode_tostr(u32 hash, char *buffer) {
    uint64_t x = hash;

    x = ((x & 0xFFFF) << 32) | ((x & 0xFFFF0000) >> 16);
    x = ((x & 0x0000FF000000FF00) >> 8) | (x & 0x000000FF000000FF) << 16;
    x = ((x & 0x00F000F000F000F0) >> 4) | (x & 0x000F000F000F000F) << 8;

    uint64_t mask = ((x + 0x0606060606060606) >> 4) & 0x0101010101010101;

    x |= 0x3030303030303030;
    x += 0x27 * mask;

    *(uint64_t *) buffer = x;
    buffer[8] = 0;
}

struct reg_entry *reg_lookup(const char *key) {
    u32 curr_hash;
    struct reg_entry *entry = NULL;
    int i;

    // lookup reg_entry
    curr_hash = reg_calc_hashcode(key);
    for (i = 0; i < registry_len; i++) {
        if (registry[i].key_hash == curr_hash) {
            entry = &registry[i];
            break;
        }
    }

    return entry;
}

void reg_init() {
    int failed_writes, succ_writes, i;

    // calculate hash/hash_str at runtime
    for (i = 0; i < registry_len; i++) {
        registry[i].key_hash = reg_calc_hashcode(registry[i].key);
        reg_hashcode_tostr(registry[i].key_hash, registry[i].key_hash_str);
    }

    // try to start nvs_flash
    if (nvs_flash_init()) {
        ESP_LOGE(TAG, "invalid nvs flash partition, creating / initializing registry");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    failed_writes = 0;
    succ_writes = 0;
    for (i = 0; i < registry_len; i++) {
        // attempt to read value from flash->ram
        if (reg_read(registry[i].key)) {

            // reg_value didnt exist in flash, write default
            if (reg_write(registry[i].key))
                failed_writes++;
            else
                succ_writes++;
        }
    }

    ESP_LOGI(TAG, "registry initialized with %d entries | %d failed writes | %d successful writes", registry_len,
             failed_writes, succ_writes);
}

// TODO expand read/write functions to use reg_entry/key_hash/etc
int reg_read(const char *key) {
    nvs_handle_t handle;
    struct reg_entry *entry;
    size_t dummy;
    esp_err_t ret;

    entry = reg_lookup(key);
    if (!entry) {
        ESP_LOGE(TAG, "reg_read() on unknown entry: %s", key);
        return ESP_FAIL;
    }

    ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (!ret) {

        switch (entry->type) {
            default:
                return ESP_FAIL;
            case REG_8B:
                ret = nvs_get_u8(handle, entry->key_hash_str, entry->location);
                break;
            case REG_16B:
                ret = nvs_get_u16(handle, entry->key_hash_str, entry->location);
                break;
            case REG_32B:
            case REG_FLT:
                ret = nvs_get_u32(handle, entry->key_hash_str, entry->location);
                break;
            case REG_64B:
                ret = nvs_get_u64(handle, entry->key_hash_str, entry->location);
                break;
            case REG_STR:
                // i know the size will always be ok here because its setter verified
                // thus we need to trick the api into not throwing a ESP_ERR_NVS_INVALID_LENGTH
                dummy = 0xFFFFFFFF;
                ret = nvs_get_str(handle, entry->key_hash_str, entry->location, &dummy);
                break;
        }

        nvs_close(handle);
        return ret;
    } else {
        ESP_LOGE(TAG, "failed to acquire nvs read handle, code = %i", ret);
        return ret;
    }
}

int reg_write(const char *key) {
    nvs_handle_t handle;
    struct reg_entry *entry;
    esp_err_t ret;

    entry = reg_lookup(key);
    if (!entry) {
        ESP_LOGE(TAG, "reg_write() on unknown entry: %s", key);
        return ESP_FAIL;
    }

    if (!nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle)) {

        switch (entry->type) {
            default:
                return ESP_FAIL;
            case REG_8B:
                ret = nvs_set_u8(handle, entry->key_hash_str, *(u8 *) entry->location);
                break;
            case REG_16B:
                ret = nvs_set_u16(handle, entry->key_hash_str, *(u16 *) entry->location);
                break;
            case REG_32B:
            case REG_FLT:
                ret = nvs_set_u32(handle, entry->key_hash_str, *(u32 *) entry->location);
                break;
            case REG_64B:
                ret = nvs_set_u64(handle, entry->key_hash_str, *(u64 *) entry->location);
                break;
            case REG_STR:
                /* only store small strings, will likely never need size data */
                ret = nvs_set_str(handle, entry->key_hash_str, entry->location);
                break;
        }

        nvs_commit(handle);
        nvs_close(handle);
        return ret;
    } else {
        ESP_LOGE(TAG, "failed to acquire nvs read/write handle");
        return ESP_FAIL;
    }
}
