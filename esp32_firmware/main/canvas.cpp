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

#include "canvas.h"

#include <math.h>
#include <stdatomic.h>
#include <time.h>

#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "lvgl.h"

#include "ble_services.h"
#include "board_pins.h"

extern const uint8_t font[];

namespace canvas {

static const char *TAG = "canvas";

Canvas::Canvas(int w, int h, lv_obj_t *screen) {
    this->w = w;
    this->h = h;
    ESP_LOGI(TAG, "Allocating canvas buffer.");
    size_t buf_size = this->w * this->h * sizeof(lv_color_t);
    this->canvas_buf = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);

    ESP_LOGI(TAG, "Creating canvas.");
    this->canvas_obj = lv_canvas_create(screen);
    lv_canvas_set_buffer(this->canvas_obj, this->canvas_buf, this->w, this->h, LV_IMG_CF_TRUE_COLOR);
}

Canvas::~Canvas() {
    if (this->canvas_buf) {
        heap_caps_free(this->canvas_buf);
        this->canvas_buf = nullptr;
    }
    if (this->canvas_obj) {
        lv_obj_del(this->canvas_obj);
        this->canvas_obj = nullptr;
    }
}

void Canvas::clear(lv_color_t color) {
    lv_canvas_fill_bg(this->canvas_obj, color, LV_OPA_COVER);
}

void Canvas::invalidate() {
    lv_obj_invalidate(this->canvas_obj);
}

void Canvas::fill_circle(int x, int y, int w, int h, lv_color_t color, lv_color_t border_col, int border_width) {

    // draw background
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.radius = LV_RADIUS_CIRCLE;
    dsc.bg_color = color;
    dsc.bg_opa = LV_OPA_COVER;
    dsc.border_color = border_col;
    dsc.border_width = border_width;
    dsc.border_opa = LV_OPA_COVER;
    lv_canvas_draw_rect(this->canvas_obj, x, y, w, w, &dsc);

    // draw circle
    lv_draw_rect_dsc_t circle_dsc;
    lv_draw_rect_dsc_init(&circle_dsc);
    circle_dsc.bg_color = color;
    circle_dsc.bg_opa = LV_OPA_COVER;
    circle_dsc.border_color = border_col;
    circle_dsc.border_width = border_width;
    circle_dsc.radius = LV_RADIUS_CIRCLE;

    lv_canvas_draw_rect(this->canvas_obj, x, y, w, h, &circle_dsc);
}

void Canvas::draw_char(uint8_t ch, int x, int y, lv_color_t fg, lv_color_t bg) {
    lv_color_t *dst_buf = this->canvas_buf;
    const uint8_t *chdata = &font[ch*8];
    dst_buf = dst_buf + y*this->w + x;
    for (int j = 0; j < 8; j++) {
        uint8_t v = *(chdata++);
        for (int i = 0; i < 8; i++) {
            if (v & (1 << (7-i))) {
                dst_buf[i] = dst_buf[i+this->w] = fg;
            } else {
                dst_buf[i] = dst_buf[i+this->w] = bg;
            }
        }
        dst_buf += 2*this->w;
    }
}

void Canvas::draw_string(const char *str, int x, int y, lv_color_t fg, lv_color_t bg) {
    while (*str) {
        this->draw_char(*str++, x, y, fg, bg);
        x += 8;
    }
}

void Canvas::draw_filled_poly(const lv_point_t *points, uint16_t point_count, lv_color_t color) {
    lv_draw_rect_dsc_t poly_dsc;
    lv_draw_rect_dsc_init(&poly_dsc);

    poly_dsc.bg_color = color;
    poly_dsc.bg_opa = LV_OPA_COVER;

    lv_canvas_draw_polygon(this->canvas_obj, points, point_count, &poly_dsc);
}

void Canvas::draw_line(lv_point_t *points, int count, lv_color_t color, lv_coord_t width) {
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = color;
    line_dsc.width = width;
    line_dsc.round_start = false;
    line_dsc.round_end = false;

    lv_canvas_draw_line(this->canvas_obj, points, count, &line_dsc);
}

} // namespace canvas

