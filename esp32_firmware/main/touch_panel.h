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

/** Initializes I2C + GT911 and registers an LVGL input driver.
 *  Not strictly necessary for a plain clock display, but useful for
 *  later extensions (e.g. tapping to manually adjust the time). */
void touch_panel_init(lv_disp_t *disp);

#ifdef __cplusplus
}
#endif
