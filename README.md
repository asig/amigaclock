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

## License

Copyright (C) 2026 Andreas Signer <asigner@gmail.com>

AmigaClock is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option)
any later version. See [LICENSE](LICENSE) for the full text.
