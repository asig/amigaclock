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

#include "quickjs.h"

typedef enum {
    QJS_MSG_EVENT, // Call the JS callback associated with m->id
    QJS_MSG_LOAD, // Load and start the widget from m->data
} qjs_msg_type_t;

typedef struct {
    qjs_msg_type_t type;
    uint32_t id;
    uint8_t *data;
} qjs_msg_t;

// Initialize the QuickJS runtime environment.
void qjs_init_runtime(void);

// Post a message to the QuickJS runtime message queue. Can be called from any task (BLE, Wi-Fi, ...).
bool qjs_post(const qjs_msg_t *m);

// Poll the QuickJS runtime message queue. Should only be called from the UI task.
void qjs_poll(uint32_t timeout_ms);

