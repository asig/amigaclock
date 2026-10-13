# Amiga Clock

AmigaClock is a desk clock, built on GUITION/Sunton ESP32-4848S040 hardware
(ESP32-S3 with a 480x480 RGB display and capacitive touch), that renders an
analog clock face styled after the classic AmigaOS 1.2 Workbench. The clock
gets its time over Bluetooth Low Energy (standard Current Time Service) from
a companion app, and can additionally keep itself in sync over Wi-Fi via NTP
once credentials are configured over BLE.

This repository is organized into:

- [esp32_firmware/](esp32_firmware) - the ESP-IDF firmware that drives the
  display and BLE/Wi-Fi functionality.
- [android_app/](android_app) - the Android companion app used to set the
  time and Wi-Fi credentials over BLE.
- [BLE-SPECS.md](BLE-SPECS.md) - the BLE protocol shared between the firmware
  and the companion app.

## Building

### ESP32 Firmware

**Requirements:**
- ESP-IDF v5.1 or newer (currently tested with v6.1)
- Exported ESP-IDF environment

**Build steps:**

```bash
cd esp32_firmware
. $HOME/esp/esp-idf/export.sh
idf.py set-target esp32s3
idf.py build
```

To flash and monitor a connected board:

```bash
idf.py flash monitor
```

See [esp32_firmware/README.md](esp32_firmware/README.md) for detailed information about hardware setup, pin configuration, and BLE interface.

### Android Companion App

**Requirements:**
- Android Studio (recommended) or Gradle
- Java 11 or newer
- Android SDK level 21 or higher

**Build steps using Gradle:**

```bash
cd android_app
./gradlew build
```

To build and run on a connected device or emulator:

```bash
./gradlew installDebug
```

Or open the project in Android Studio and use the standard build and run commands.

The app connects to the AmigaClock device over BLE and allows you to:
- Sync the device time from your phone
- Configure Wi-Fi credentials
- Adjust display settings (show date, show seconds)
- Control backlight brightness
- Change display resolution (hires/lores)

For more details, see the [BLE protocol specification](BLE-SPECS.md).

## License

Copyright (C) 2026 Andreas Signer <asigner@gmail.com>

AmigaClock is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option)
any later version. See [LICENSE](LICENSE) for the full text.
