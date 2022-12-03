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

#ifndef HACKQUAD_REGISTRY_H
#define HACKQUAD_REGISTRY_H

#include "lint_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    REG_8B = 0,
    REG_16B,
    REG_32B,
    REG_64B,
    REG_STR,
    REG_FLT
} reg_type_t;

struct reg_entry {
    /* must be init-ed in registry[] */
    char *key;
    reg_type_t type;
    void *location;

    /* private */
    u32 key_hash;

    /* hex str, used as key for nvs system, not ideal but I dont want to write my own system */
    char key_hash_str[9];
};

extern struct reg_entry registry[];
extern size_t registry_len;

/* MUST USE IN IMPL */
#define DEFINE_REGISTRY(...)                                          \
    struct reg_entry registry[] = __VA_ARGS__;                        \
    size_t registry_len = sizeof(registry) / sizeof(struct reg_entry)


int reg_calc_hashcode(const char *str);

void reg_hashcode_tostr(u32 hash, char *buffer);

/**
 * Initializes registry.
 */
void reg_init();

/**
 * TODO
 * Used to request the value of a registry entry if that entry has no associated
 * global variable.
 */
int reg_request(const char *key, void *buffer);

/**
 * Reads registry value from flash->memory.
 */
int reg_read(const char *key);

/**
 * Writes registry value from memory->flash.
 */
int reg_write(const char *key);

struct reg_entry *reg_lookup(const char *key);

#ifdef __cplusplus
}
#endif

#endif /* HACKQUAD_REGISTRY_H */
