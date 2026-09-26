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

#include "widgets/widget.h"

namespace widgets {

struct ClockDimensions;

class AmigaClockWidget : public Widget {
public:
    const std::string PROPERTY_SHOW_DATE = "show_date";
    const std::string PROPERTY_SHOW_SECONDS = "show_seconds";
    
    AmigaClockWidget();
    virtual ~AmigaClockWidget();

    bool tick() override;
    void render() override;

    void set_property(const std::string &name, const std::string &value) override;
    std::string get_property(const std::string &name) const override;

    std::vector<std::string> get_property_names() const override;

private:
    void pick_clock_dimensions();
    void draw_tick(double angle_deg);
    void draw_big_tick(double angle_deg);
    void draw_minute_hand(double angle_deg);
    void draw_hour_hand(double angle_deg);
    void draw_second_hand(double angle_deg);
    void draw_date_line(int day, int month, int year);

    ClockDimensions *clock_dims;

    bool show_date;
    bool show_seconds;    
};

} // namespace widgets


// /** Builds the Amiga Workbench 1.2-style clock face and starts the
//  *  1-Hz timer that redraws the hands and title bar on every second
//  *  change. Must be called after lcd_panel_init() has registered an active
//  *  LVGL display. */
// void amiga_clock_ui_create(void);

// void amiga_clock_ui_configure(bool show_date, bool show_seconds);

// void amiga_clock_ui_get_config(bool *show_date, bool *show_seconds);

// // Ticks the Amiga clock UI. Returns true if the UI needs to be redrawn.
// bool amiga_clock_ui_tick(void);

// // Renders the Amiga clock UI immediately. Only call this if amiga_clock_ui_tick() returned true.
// void amiga_clock_ui_render(void);

// #ifdef __cplusplus
// }
// #endif
