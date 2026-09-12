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
#include "nvs_flash.h"
#include "lvgl.h"

#include "ble_services.h"
#include "amiga_clock_ui.h"
#include "lcd_panel.h"
#include "touch_panel.h"
#include "wifi.h"

static const char *TAG = "main";

static void lvgl_tick_task(void *arg)
{
    while (1) {
        lv_tick_inc(5);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    lv_init();

    lv_disp_t *disp = lcd_panel_init();
    touch_panel_init(disp);

    xTaskCreate(lvgl_tick_task, "lvgl_tick", 2048, NULL, 5, NULL);

    amiga_clock_ui_create();
    wifi_init();
    ble_services_start();

    ESP_LOGI(TAG, "Amiga clock running. Set time via BLE (Current Time Service, "
                  "0x1805 / 0x2A2B), e.g. with nRF Connect.");

    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
