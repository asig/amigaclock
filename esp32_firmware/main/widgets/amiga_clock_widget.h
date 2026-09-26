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
    static const std::string PROPERTY_SHOW_DATE;
    static const std::string PROPERTY_SHOW_SECONDS;
    
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
