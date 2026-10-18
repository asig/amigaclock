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

#include "lvgl.h"

namespace canvas {

class Canvas;

class Rasterizer {
public:
    virtual ~Rasterizer() = default;

    virtual void draw_line(Canvas *canvas, lv_point_t *points, int count, lv_color_t color, lv_coord_t width) = 0;
    virtual void draw_filled_poly(Canvas *canvas, const lv_point_t *points, uint16_t point_count,
                                  lv_color_t color) = 0;
    virtual void fill_circle(Canvas *canvas, int center_x, int center_y, int radius, lv_color_t color) = 0;
};

class SoftwareRasterizer final : public Rasterizer {
public:
    void draw_line(Canvas *canvas, lv_point_t *points, int count, lv_color_t color, lv_coord_t width) override;
    void draw_filled_poly(Canvas *canvas, const lv_point_t *points, uint16_t point_count, lv_color_t color) override;
    void fill_circle(Canvas *canvas, int center_x, int center_y, int radius, lv_color_t color) override;
};

class LvglRasterizer final : public Rasterizer {
public:
    void draw_line(Canvas *canvas, lv_point_t *points, int count, lv_color_t color, lv_coord_t width) override;
    void draw_filled_poly(Canvas *canvas, const lv_point_t *points, uint16_t point_count, lv_color_t color) override;
    void fill_circle(Canvas *canvas, int center_x, int center_y, int radius, lv_color_t color) override;
};

} // namespace canvas