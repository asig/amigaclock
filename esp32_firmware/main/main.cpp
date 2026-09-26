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
 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "lvgl.h"

#include "ble_services.h"
#include "widgets/widget.h"
#include "widgets/amiga_clock_widget.h"
#include "graphics.h"

// #include "amiga_clock_ui.h"

#include "lcd_panel.h"
#include "touch_panel.h"
#include "wifi.h"
#include "qjs_runtime.h"
#include "widgets/widget.h"

static const char *TAG = "main";

static void main_loop(void *arg) {
    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(10); // 10 ms (100 Hz)

    for (;;) {
        lv_tick_inc(10);
        widgets::Widget *w = widgets::get_widget();
        bool updated = w->tick();
        if (updated) {
            w->render();
        }

        lv_timer_handler();
        qjs_poll(10); // TODO(asigner): Turn this into a generic event queue!
        
        vTaskDelayUntil(&last_wake, period);
    }
}

extern "C" {

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Init order is not *that* critical, besides this:
    // 1) LVGL needs to be initialized before creating the UI.
    // 1) amiga_clock_ui_create() needs to be called BEFORE creating the
    //    main loop task.
    // 2) Main task needs to be created early so that there's a big enough 
    //    block in the heap to satisfy its stack requirements.

    lv_init();

    lv_disp_t *disp = lcd_panel_init();
    touch_panel_init(disp);

    qjs_init_runtime();

    graphics::init();
    auto *w = new widgets::AmigaClockWidget();
    widgets::set_widget(w);

    xTaskCreatePinnedToCore(main_loop, "main_loop", 32*1024, NULL, 5, NULL, 0);
    wifi_init();
    ble_services_start();

    ESP_LOGI(TAG, "Amiga clock running.");
}

} // extern "C"
