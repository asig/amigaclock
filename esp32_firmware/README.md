# Amiga Workbench 1.2 Clock for ESP32-4848S040

This ESP-IDF project implements an analog clock inspired by the classic
AmigaOS 1.2 Workbench. It draws a clock face and clock hands on the 480x480
RGB display of the ESP32-4848S040, with optional date and seconds display.

The clock can receive its time through Bluetooth Low Energy using the standard
Current Time Service. When configured Wi-Fi is available, it also synchronizes
the system clock with NTP.

## Requirements

- ESP-IDF **v5.1 or newer**. The current build uses v6.1.
- An exported ESP-IDF environment, for example:
  `. $HOME/esp/esp-idf/export.sh`
- GUITION/Sunton **ESP32-4848S040** hardware:
  ESP32-S3, ST7701 display controller, GT911 touch controller, 8 MB octal
  PSRAM, and 16 MB flash.

## Build

From the repository root:

```bash
cd esp32_firmware
. $HOME/esp/esp-idf/export.sh
idf.py set-target esp32s3
idf.py build
```

`idf.py set-target esp32s3` is only needed once per build directory. The first
build downloads the components listed in `main/idf_component.yml`
(`esp_lcd_st7701`, `esp_lcd_touch_gt911`, and `lvgl`) into
`managed_components/`.

To flash a connected board and open its serial log:

```bash
idf.py flash monitor
```

Specify the serial port with `idf.py -p PORT ...` when necessary.

## Hardware

The board pin assignments are defined in `main/board_pins.h`. They were
checked against the board documentation and several community projects. If a
particular board revision behaves differently, verify its pin assignments
against the vendor documentation.

`main/lcd_panel.c` contains the ST7701 initialization sequence and the RGB
panel setup. The exact ESP-IDF and managed-component versions matter because
panel configuration fields have changed between ESP-IDF releases.

## Startup and time synchronization

`app_main()` performs the following initialization sequence:

1. Initialize NVS. If the NVS partition is full or has an incompatible
   version, erase it and initialize it again.
2. Initialize LVGL, the ST7701 display, and the GT911 touch controller.
3. Start the LVGL tick task and create the clock UI.
4. Initialize Wi-Fi in station mode and load saved credentials from NVS.
5. Start the NimBLE peripheral and its GATT services.
6. Run the LVGL event loop.

If saved Wi-Fi credentials exist, the device connects automatically. After it
receives an IP address, `wifi.c` starts SNTP using `pool.ntp.org`. NTP can
therefore update the clock after BLE has set the initial time.

If no Wi-Fi credentials exist, Wi-Fi is initialized but does not start a
connection. Credentials can be supplied through the BLE Wi-Fi configuration
characteristic.

The firmware does not configure a timezone or daylight-saving rule.
`localtime_r()` uses the device's current C library timezone configuration.

## BLE interface

The device advertises as **AmigaClock**. Use a BLE scanner such as nRF Connect
to connect to it.

### Set or read the time

The standard Bluetooth SIG Current Time Service is exposed at `0x1805`, with
the Current Time characteristic at `0x2A2B`. The characteristic supports read
and write.

The time payload is 10 bytes in little-endian order:

```text
[year low][year high][month][day][hour][minute][second][weekday][0][0]
```

For 2026-09-10 14:30:00, Thursday (`4`), write:

```text
EA 07 09 0A 0E 1E 00 04 00 00
```

Writing the characteristic calls `settimeofday()`. The clock continues using
the local system clock even when the BLE connection is closed.

### Wi-Fi and display commands

The firmware also exposes the custom 16-bit service `0xFFF0`:

| Characteristic | Properties | Payload |
| --- | --- | --- |
| `0xFFF1` Wi-Fi configuration | Write | `[ssid_len][password_len][ssid][password]` |
| `0xFFF2` Wi-Fi status | Read | `[status][ssid_len][ssid][ip[4]]` |
| `0xFFF3` display configuration | Read, Write | `[show_date][show_seconds]` |

Wi-Fi SSIDs are limited to 31 bytes and passwords to 63 bytes. Wi-Fi status
values are `0` disconnected, `1` connecting, and `2` connected. Display flags
are `0` disabled and `1` enabled. Invalid payload lengths and display flag
values are rejected.

For the complete byte-level protocol, see the repository-level
`BLE-SPECS.md`.

## Project structure

```text
main/
  board_pins.h          Pin assignments for the ESP32-4848S040
  lcd_panel.c/.h        ST7701/RGB panel initialization and LVGL integration
  touch_panel.c/.h      GT911 touch input integration
  wifi.c/.h             Wi-Fi initialization, NTP, NVS, and status
  ble_services.c/.h     NimBLE GATT server and BLE command handlers
  amiga_clock_ui.c/.h   Clock face, hands, date, and seconds rendering
  main.c                app_main and the LVGL event loop
```

## Possible extensions

- Add a touch-driven settings menu.
- Configure a timezone and daylight-saving rules with `TZ` and `tzset()`.
- Add a companion Android or Web Bluetooth app to automate time, Wi-Fi, and
  display configuration.
