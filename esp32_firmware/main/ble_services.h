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
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Starts NimBLE as a peripheral with the Bluetooth SIG standard service
 * "Current Time Service" (0x1805) / characteristic "Current Time" (0x2A2B)
 * as well as its own app-specific BLE service for Wi-Fi configuration.
 */
void ble_services_start();

#ifdef __cplusplus
}
#endif
