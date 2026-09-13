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

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the ST7701 panel (3-wire SPI init + RGB data path, via
 * the espressif/esp_lcd_st7701 vendor driver) including backlight, and
 * hooks up LVGL as the display driver (flush callback, buffer in PSRAM).
 */
lv_disp_t *lcd_panel_init(void);

/** Set backlight 0-100 %. */
void lcd_panel_set_backlight(uint8_t percent);

/** Get the currently configured backlight 0-100 %. */
uint8_t lcd_panel_get_backlight(void);

#ifdef __cplusplus
}
#endif
