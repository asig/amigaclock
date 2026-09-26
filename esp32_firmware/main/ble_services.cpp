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

#include "ble_services.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/time.h>

#include "esp_log.h"

#include "widgets/widget.h"
#include "widgets/amiga_clock_widget.h"

static const char *TAG = "ble_services";

void ble_services_apply_display_config(bool show_date, bool show_seconds) {
    widgets::Widget *w = widgets::get_widget();
    if (!w) {
        ESP_LOGW(TAG, "No widget available to apply display config.");
        return;
    }
    w->set_property(widgets::AmigaClockWidget::PROPERTY_SHOW_DATE, show_date ? "true" : "false");
    w->set_property(widgets::AmigaClockWidget::PROPERTY_SHOW_SECONDS, show_seconds ? "true" : "false");
    ESP_LOGI(TAG, "Applied display config: show_date=%s, show_seconds=%s", show_date ? "true" : "false", show_seconds ? "true" : "false");
}

void ble_services_get_display_config(bool *show_date, bool *show_seconds) {
    widgets::Widget *w = widgets::get_widget();
    if (!w) {
        ESP_LOGW(TAG, "No widget available to get display config; defaulting to false for both show_date and show_seconds");
        *show_date = false;
        *show_seconds = false;
        return;
    }
    std::string val = w->get_property(widgets::AmigaClockWidget::PROPERTY_SHOW_DATE);
    *show_date = (val == "true");

    val = w->get_property(widgets::AmigaClockWidget::PROPERTY_SHOW_SECONDS);
    *show_seconds = (val == "true");
}
