/*
 * This file is part of AmigaClock.
 *
 * Copyright (C) 2026 Andreas Signer <asigner@gmail.com>
 *
 * AmigaClock is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AmigaClock is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AmigaClock.  If not, see <https://www.gnu.org/licenses/>.
 */
 
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_STATUS_DISCONNECTED = 0,
    WIFI_STATUS_CONNECTING = 1,
    WIFI_STATUS_CONNECTED = 2,
} wifi_status_t;

typedef struct {
    wifi_status_t status;
    char ssid[33];
    uint8_t ip[4];
} wifi_status_info_t;

/** Initializes Wi-Fi, reconnects using NVS credentials, and starts NTP after an IP is acquired. */
void wifi_init(void);

/** Stores SSID and password and immediately tries to connect. */
void wifi_apply_config(const char *ssid, const char *password);

/** Reads the current Wi-Fi status, SSID, and IP address. */
void wifi_get_status(wifi_status_info_t *status);

#ifdef __cplusplus
}
#endif
