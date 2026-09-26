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

#include <string>
#include <vector>

#include "lvgl.h"

namespace widgets {

class Widget {
public:
    Widget() = default;
    virtual ~Widget() = default;

    virtual bool tick() = 0;
    virtual void render() = 0;

    virtual std::vector<std::string> get_property_names() const = 0;

    virtual void set_property(const std::string &name, const std::string &value) = 0;
    virtual std::string get_property(const std::string &name) const = 0;
};

void set_widget(Widget *widget);
Widget *get_widget();

} // namespace widgets

