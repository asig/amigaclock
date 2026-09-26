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

#include "board_pins.h" // LCD_H_RES and LCD_V_RES

namespace graphics {

const constexpr int CANVAS_W = LCD_H_RES; 
const constexpr int CANVAS_H = LCD_V_RES; 

const constexpr int TEXT_HEIGHT = 16;

void init();

void clear(lv_color_t color);

void fill_circle(int x, int y, int w, int h, lv_color_t color, lv_color_t border_col, int border_width);


void draw_char(uint8_t ch, int x, int y, lv_color_t fg, lv_color_t bg);
void draw_string(const char *str, int x, int y, lv_color_t fg, lv_color_t bg);

void draw_filled_poly(const lv_point_t *points, uint16_t point_count, lv_color_t color);
void draw_line(lv_point_t *points, int count, lv_color_t color, lv_coord_t width);

}