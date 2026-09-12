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

/*
 * Pinout for GUITION / Sunton ESP32-4848S040 (ESP32-S3, ST7701 RGB panel,
 * GT911 touch), 480x480, without back cover.
 *
 * Source: official ESPHome device page for this board (devices.esphome.io)
 * as well as several independently matching community projects (Tasmota,
 * openHASP, PlatformIO reference projects). These boards are sold by
 * several manufacturers in slightly different revisions -- if your
 * display shows nothing or a distorted image on first boot, check the
 * pins against the datasheet/example code linked on the seller page or
 * at http://pan.jczn1688.com.
 */

// ---- 3-wire SPI (only for the ST7701 init commands) ----
#define PIN_LCD_SPI_CS      39
#define PIN_LCD_SPI_SCK     48
#define PIN_LCD_SPI_MOSI    47
// no MISO, no separate DC (9-bit SPI, ST7701 doesn't need a DC pin)

// ---- RGB Timing-Signale ----
#define PIN_LCD_DE          18
#define PIN_LCD_VSYNC       17
#define PIN_LCD_HSYNC       16
#define PIN_LCD_PCLK        21

// ---- RGB565 data lines (R5 G6 B5) ----
#define PIN_LCD_R0          11
#define PIN_LCD_R1          12
#define PIN_LCD_R2          13
#define PIN_LCD_R3          14
#define PIN_LCD_R4          0

#define PIN_LCD_G0          8
#define PIN_LCD_G1          20
#define PIN_LCD_G2          3
#define PIN_LCD_G3          46
#define PIN_LCD_G4          9
#define PIN_LCD_G5          10

#define PIN_LCD_B0          4
#define PIN_LCD_B1          5
#define PIN_LCD_B2          6
#define PIN_LCD_B3          7
#define PIN_LCD_B4          15

// ---- Backlight (LEDC PWM) ----
#define PIN_LCD_BACKLIGHT   38

// ---- LCD Reset ----
// Passive RC circuit on the board (R16/C22) - no GPIO reset needed,
// confirmed by the manufacturer's schematic.
#define PIN_LCD_RST         -1

// ---- Touch GT911 (I2C) ----
#define PIN_TOUCH_I2C_SDA   19
#define PIN_TOUCH_I2C_SCL   45
#define PIN_TOUCH_RST       -1   // not separately wired on most boards
#define PIN_TOUCH_INT       -1   // same here -> use polling instead of interrupt

#define LCD_H_RES           480
#define LCD_V_RES           480
