# This file is part of AmigaClock.
#
# Copyright (C) 2026 Andreas Signer <asigner@gmail.com>
#
# AmigaClock is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# AmigaClock is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with AmigaClock.  If not, see <https://www.gnu.org/licenses/>.
 
idf_component_register(
    SRCS
        "main.c"
        "lcd_panel.c"
        "touch_panel.c"
        "wifi.c"
        "ble_services.c"
        "amiga_clock_ui.c"
        "fontdata.c"
    INCLUDE_DIRS "."
    REQUIRES esp_lcd esp_timer driver esp_driver_ledc nvs_flash bt
)
