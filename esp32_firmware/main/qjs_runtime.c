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

#include "qjs_runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#define TAG "qjs_runtime"

struct qjs_widget {
    JSContext *ctx;
};

static QueueHandle_t s_queue;
static JSRuntime *rt = NULL;
static struct qjs_widget *cur_widget = NULL;

static void *qjs_calloc(void *opaque, size_t count, size_t size) {
    return heap_caps_calloc(count, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

static void *qjs_malloc(void *opaque, size_t size) {
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

static void qjs_free(void *opaque, void *ptr) {
    heap_caps_free(ptr);
}

static void *qjs_realloc(void *opaque, void *ptr, size_t size) {
    return heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

static size_t qjs_usable_size(const void *ptr) {
    return heap_caps_get_allocated_size((void *)ptr);
}

static const JSMallocFunctions mf = {
    qjs_calloc, qjs_malloc, qjs_free, qjs_realloc, qjs_usable_size
};

void qjs_init_runtime(void) {
    s_queue = xQueueCreate(16, sizeof(qjs_msg_t));
    rt = JS_NewRuntime2(&mf, NULL);
    JS_SetMemoryLimit(rt, 2 * 1024 * 1024);
    JS_SetMaxStackSize(rt, 64 * 1024);
}

JSContext *qjs_create_context() {
    if (!rt) {
        ESP_LOGE("qjs_runtime", "QuickJS runtime not initialized. Did you forget to call qjs_init_runtime()?");
        return NULL;
    }
    JSContext *ctx = JS_NewContext(rt);
    return ctx;
}


bool qjs_post(const qjs_msg_t *m) {
    return xQueueSend(s_queue, m, 0) == pdTRUE;
}

static void js_dump_error(JSContext *ctx) {
    JSValue exc = JS_GetException(ctx);

    const char *msg = JS_ToCString(ctx, exc);
    ESP_LOGE(TAG, "JS error: %s", msg ? msg : "(unprintable)");
    JS_FreeCString(ctx, msg);

    if (JS_IsObject(exc)) {
        JSValue st = JS_GetPropertyStr(ctx, exc, "stack");
        if (!JS_IsUndefined(st)) {
            const char *s = JS_ToCString(ctx, st);
            ESP_LOGE(TAG, "%s", s != NULL ? s : "<unprintable>");
            JS_FreeCString(ctx, s);
        }
        JS_FreeValue(ctx, st);
    }
    JS_FreeValue(ctx, exc);
}

static void qjs_free_widget(struct qjs_widget *w) {
    if (!w) return;
    JS_FreeContext(w->ctx);
    heap_caps_free(w);
}

static struct qjs_widget *qjs_load_widget(uint8_t *data) {
    struct qjs_widget *w = heap_caps_calloc(1, sizeof(struct qjs_widget), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!w) return NULL;
    w->ctx = qjs_create_context();
    if (!w->ctx) {
        heap_caps_free(w);
        return NULL;
    }
    // TODO(asig): Load the widget using the provided data

    return w;
}

static void qjs_dispatch(const qjs_msg_t *m) {
    switch (m->type) {
    case QJS_MSG_EVENT: 
        // Call the JS callback associated with m->id
        // TODO(asig): Build this
        break;
        
    case QJS_MSG_LOAD:
        if (cur_widget) {
            qjs_free_widget(cur_widget);
            cur_widget = NULL;
        }
        cur_widget = qjs_load_widget(m->data);
        break;
    }
}

static void js_drain_jobs(void) {
    JSContext *c;
    int r;
    while ((r = JS_ExecutePendingJob(rt, &c)) > 0) {}
    if (r < 0) js_dump_error(c);
}

void qjs_poll(uint32_t timeout_ms) {
    TickType_t ticks = pdMS_TO_TICKS(timeout_ms);
    if (ticks == 0) ticks = 1;

    qjs_msg_t m;
    if (xQueueReceive(s_queue, &m, ticks) == pdTRUE) {
        do {
            qjs_dispatch(&m);
        } while (xQueueReceive(s_queue, &m, 0) == pdTRUE);
    }
    js_drain_jobs();
}