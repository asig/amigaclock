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

#include "rasterizer.h"

#include <algorithm>
#include <cstdlib>

#include "canvas.h"

namespace canvas {

void SoftwareRasterizer::fill_circle(Canvas *canvas, int center_x, int center_y, int radius, lv_color_t color) {
    if (canvas == nullptr || canvas->buffer() == nullptr || canvas->width() <= 0 || canvas->height() <= 0 || radius < 0) {
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
                canvas->set_pixel(span_x, span_y[span], color);
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

void SoftwareRasterizer::draw_filled_poly(Canvas *canvas, const lv_point_t *points, uint16_t point_count,
                                          lv_color_t color) {
    if (canvas == nullptr || points == nullptr || point_count < 3 || canvas->buffer() == nullptr ||
        canvas->width() <= 0 || canvas->height() <= 0) {
        return;
    }

    int min_y = points[0].y;
    int max_y = points[0].y;
    for (uint16_t i = 1; i < point_count; i++) {
        min_y = std::min(min_y, static_cast<int>(points[i].y));
        max_y = std::max(max_y, static_cast<int>(points[i].y));
    }

    const int first_y = std::max(0, min_y);
    const int last_y = std::min(canvas->height() - 1, max_y);
    for (int y = first_y; y <= last_y; y++) {
        int min_x = canvas->width();
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
            max_x = std::min(canvas->width() - 1, max_x);
            for (int x = min_x; x <= max_x; x++) {
                canvas->set_pixel(x, y, color);
            }
        }
    }
}

void SoftwareRasterizer::draw_line(Canvas *canvas, lv_point_t *points, int count, lv_color_t color, lv_coord_t width) {
    if (canvas == nullptr || points == nullptr || count < 2 || canvas->buffer() == nullptr ||
        canvas->width() <= 0 || canvas->height() <= 0 || width <= 0) {
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
            canvas->set_pixel(x0, y0, color);
            if (stroke_width == 2) {
                canvas->set_pixel(x0 + (horizontal_major ? 0 : 1), y0 + (horizontal_major ? 1 : 0), color);
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

void LvglRasterizer::fill_circle(Canvas *canvas, int center_x, int center_y, int radius, lv_color_t color) {
    if (canvas == nullptr || radius < 0) {
        return;
    }

    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.radius = LV_RADIUS_CIRCLE;
    dsc.bg_color = color;
    dsc.bg_opa = LV_OPA_COVER;
    const int diameter = 2 * radius + 1;
    lv_canvas_draw_rect(canvas->canvas(), center_x - radius, center_y - radius, diameter, diameter, &dsc);
}

void LvglRasterizer::draw_filled_poly(Canvas *canvas, const lv_point_t *points, uint16_t point_count, lv_color_t color) {
    if (canvas == nullptr || points == nullptr || point_count < 3) {
        return;
    }

    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = color;
    dsc.bg_opa = LV_OPA_COVER;
    lv_canvas_draw_polygon(canvas->canvas(), points, point_count, &dsc);
}

void LvglRasterizer::draw_line(Canvas *canvas, lv_point_t *points, int count, lv_color_t color, lv_coord_t width) {
    if (canvas == nullptr || points == nullptr || count < 2 || width <= 0) {
        return;
    }

    lv_draw_line_dsc_t dsc;
    lv_draw_line_dsc_init(&dsc);
    dsc.color = color;
    dsc.width = width;
    dsc.round_start = false;
    dsc.round_end = false;
    lv_canvas_draw_line(canvas->canvas(), points, count, &dsc);
}

} // namespace canvas