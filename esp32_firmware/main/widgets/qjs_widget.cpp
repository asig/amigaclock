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
 
#include "widgets/qjs_widget.h"

#include <math.h>
#include <stdatomic.h>
#include <time.h>

#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "lvgl.h"

#include "ble_services.h"
#include "board_pins.h"
#include "graphics.h"

namespace widgets {

static const char *TAG = "qjs_widget";


QjsWidget::QjsWidget() {
    static bool initialized = false;
    if (!initialized) {
        init_clock_dimensions(&clock_dims_without_date, graphics::CANVAS_W-CANVAS_BORDER*2);
        init_clock_dimensions(&clock_dims_with_date, graphics::CANVAS_W-CANVAS_BORDER-CLOCK_DATE_LINE_H);
        initialized = true;
    }

    show_date = true;
    show_seconds = true;
    pick_clock_dimensions();
}

QjsWidget::~QjsWidget() {
}

bool QjsWidget::tick() {    
    struct tm t1, t2;
    localtime_r(&last_tick_time, &t1);
    last_tick_time = time(NULL);
    localtime_r(&last_tick_time, &t2);

    return t1.tm_sec != t2.tm_sec;
}

void QjsWidget::render() {
    graphics::invalidate();
}

void QjsWidget::set_property(const std::string &name, const std::string &value) {
}

std::string QjsWidget::get_property(const std::string &name) const {
    return "";
}

std::vector<std::string> QjsWidget::get_property_names() const {
    return { };
}

} // namespace widgets
