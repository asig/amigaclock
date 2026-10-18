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

#include "canvas.h"
#include "rasterizer.h"

#include "board_pins.h" // LCD_H_RES and LCD_V_RES

#include "widgets/widget.h"

using canvas::Canvas;

namespace widgets {

struct ClockDimensions {
    lv_coord_t radius;
    lv_coord_t border_w;
    lv_coord_t border_space;
    lv_coord_t hand_min_top;
    lv_coord_t hand_min_bottom;
    lv_coord_t hand_min_width;
    lv_coord_t hand_hour_top;
    lv_coord_t hand_hour_bottom;
    lv_coord_t hand_hour_width;
    lv_coord_t tick_small_h;
    lv_coord_t tick_big_h;
    lv_coord_t tick_big_w;
    int tick_line_width;
    int second_hand_width;
};

class AmigaClockWidget : public Widget {
public:
    static const std::string PROPERTY_SHOW_DATE;
    static const std::string PROPERTY_SHOW_SECONDS;
    static const std::string PROPERTY_RESOLUTION;
    static const std::string PROPERTY_ANTIALIAS;
    
    AmigaClockWidget();
    virtual ~AmigaClockWidget();

    bool tick() override;
    void render() override;
    void set_property(const std::string &name, const std::string &value) override;
    std::string get_property(const std::string &name) const override;

    std::vector<std::string> get_property_names() const override;

private:
    enum class Resolution {
        SuperHiRes,
        HiRes,
        LoRes
    };

    void update_clock_dimensions();
    void set_resolution(Resolution resolution);
    void update_rasterizer();

    void draw_clockface(Canvas *canvas, const struct tm &t);
    void draw_tick(Canvas *canvas, double angle_deg);
    void draw_big_tick(Canvas *canvas, double angle_deg);
    void draw_minute_hand(Canvas *canvas, double angle_deg);
    void draw_hour_hand(Canvas *canvas, double angle_deg);
    void draw_second_hand(Canvas *canvas, double angle_deg);
    void draw_date_line(int day, int month, int year);

    ClockDimensions clock_dims;
    lv_point_t clock_center;

    bool show_date;
    bool show_seconds;   
    Resolution resolution;
    int scale_divider;
    bool antialias;

    Canvas *screen_canvas;
    Canvas *clockface_canvas;
    canvas::Rasterizer *rasterizer;
    lv_obj_t *offscreen;
};

} // namespace widgets
