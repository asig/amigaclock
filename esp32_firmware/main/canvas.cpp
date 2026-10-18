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

#include <algorithm>
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
    this->antialiased = false;
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

void Canvas::fill_circle(int center_x, int center_y, int radius, lv_color_t color) {
    if (this->canvas_buf == nullptr || this->w <= 0 || this->h <= 0 || radius < 0) {
        return;
    }

    int offset_x = 0;
    int offset_y = radius;
    int decision = 1 - radius;
    while (offset_x <= offset_y) {
        const int span_y[] = {center_y + offset_y, center_y - offset_y, center_y + offset_x, center_y - offset_x};
        const int span_left[] = {center_x - offset_x, center_x - offset_x, center_x - offset_y, center_x - offset_y};
        const int span_right[] = {center_x + offset_x, center_x + offset_x, center_x + offset_y, center_x + offset_y};

        for (int span = 0; span < 4; span++) {
            for (int span_x = span_left[span]; span_x <= span_right[span]; span_x++) {
                this->set_pixel(span_x, span_y[span], color);
            }
        }

        offset_x++;
        if (decision < 0) {
            decision += 2 * offset_x + 1;
        } else {
            offset_y--;
            decision += 2 * (offset_x - offset_y) + 1;
        }
    }
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
    if (points == nullptr || point_count < 3 || this->canvas_buf == nullptr || this->w <= 0 || this->h <= 0) {
        return;
    }

    int min_y = points[0].y;
    int max_y = points[0].y;
    for (uint16_t i = 1; i < point_count; i++) {
        min_y = std::min(min_y, static_cast<int>(points[i].y));
        max_y = std::max(max_y, static_cast<int>(points[i].y));
    }

    int first_y = std::max(0, min_y);
    int last_y = std::min(this->h - 1, max_y);
    for (int y = first_y; y <= last_y; y++) {
        int min_x = this->w;
        int max_x = -1;

        for (uint16_t i = 0; i < point_count; i++) {
            int x1 = points[i].x;
            int y1 = points[i].y;
            int x2 = points[(i + 1) % point_count].x;
            int y2 = points[(i + 1) % point_count].y;

            if (y1 == y2) {
                if (y == y1) {
                    min_x = std::min(min_x, std::min(x1, x2));
                    max_x = std::max(max_x, std::max(x1, x2));
                }
                continue;
            }

            if (y1 > y2) {
                std::swap(x1, x2);
                std::swap(y1, y2);
            }
            if (y < y1 || y > y2) {
                continue;
            }

            const int64_t dx = static_cast<int64_t>(x2) - x1;
            const int dy = y2 - y1;
            const int direction = dx < 0 ? -1 : 1;
            const int64_t abs_dx = dx < 0 ? -dx : dx;
            const int x = x1 + direction * ((static_cast<int64_t>(y - y1) * abs_dx + dy / 2) / dy);
            min_x = std::min(min_x, x);
            max_x = std::max(max_x, x);
        }

        if (max_x >= 0) {
            min_x = std::max(0, min_x);
            max_x = std::min(this->w - 1, max_x);
            lv_color_t *row = this->canvas_buf + y * this->w;
            for (int x = min_x; x <= max_x; x++) {
                row[x] = color;
            }
        }
    }
}

void Canvas::draw_line(lv_point_t *points, int count, lv_color_t color, lv_coord_t width) {
    if (points == nullptr || count < 2 || this->canvas_buf == nullptr || this->w <= 0 || this->h <= 0 || width <= 0) {
        return;
    }

    const int stroke_width = std::min(static_cast<int>(width), 2);

    for (int segment = 0; segment < count - 1; segment++) {
        int x0 = points[segment].x;
        int y0 = points[segment].y;
        const int x1 = points[segment + 1].x;
        const int y1 = points[segment + 1].y;
        const int dx = std::abs(x1 - x0);
        const int dy = std::abs(y1 - y0);
        const int sx = x0 < x1 ? 1 : -1;
        const int sy = y0 < y1 ? 1 : -1;
        int error = dx - dy;
        const bool horizontal_major = dx >= dy;

        while (true) {
            if (x0 >= 0 && x0 < this->w && y0 >= 0 && y0 < this->h) {
                this->canvas_buf[y0 * this->w + x0] = color;
            }

            if (stroke_width == 2) {
                const int thick_x = x0 + (horizontal_major ? 0 : 1);
                const int thick_y = y0 + (horizontal_major ? 1 : 0);
                if (thick_x >= 0 && thick_x < this->w && thick_y >= 0 && thick_y < this->h) {
                    this->canvas_buf[thick_y * this->w + thick_x] = color;
                }
            }

            if (x0 == x1 && y0 == y1) {
                break;
            }

            const int twice_error = 2 * error;
            if (twice_error > -dy) {
                error -= dy;
                x0 += sx;
            }
            if (twice_error < dx) {
                error += dx;
                y0 += sy;
            }
        }
    }
}

} // namespace canvas

