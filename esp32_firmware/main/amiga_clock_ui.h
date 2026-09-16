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

#ifdef __cplusplus
extern "C" {

#endif

/** Builds the Amiga Workbench 1.2-style clock face and starts the
 *  1-Hz timer that redraws the hands and title bar on every second
 *  change. Must be called after lcd_panel_init() has registered an active
 *  LVGL display. */
void amiga_clock_ui_create(void);

void amiga_clock_ui_configure(bool show_date, bool show_seconds);

void amiga_clock_ui_get_config(bool *show_date, bool *show_seconds);

#ifdef __cplusplus
}
#endif
