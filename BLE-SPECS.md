# BLE protocol specification for AmigaClock

This document describes the Bluetooth Low Energy (BLE) interface used by the ESP32 firmware and the Android companion app.

## Overview

The device exposes:

1. The standard Bluetooth SIG Current Time Service for clock time
2. A custom vendor-specific service for Wi‑Fi configuration and Wi‑Fi status
3. A custom display configuration characteristic

This keeps time handling interoperable while making Wi‑Fi setup app-specific.

When Wi-Fi credentials are configured and the device receives an IP address,
the firmware starts SNTP using `pool.ntp.org`. NTP can update the clock after
the initial BLE time has been set.

---

## 1) Standard time service

Service:
- UUID: `0x1805`
- Name: Current Time Service

Characteristic:
- UUID: `0x2A2B`
- Name: Current Time

Properties:
- Read
- Write

### Time format

The value is 10 bytes, little-endian:

| Byte index | Field | Size | Meaning |
| --- | --- | ---: | --- |
| 0 | year low | 1 | low byte of year |
| 1 | year high | 1 | high byte of year |
| 2 | month | 1 | 1..12 |
| 3 | day | 1 | 1..31 |
| 4 | hour | 1 | 0..23 |
| 5 | minute | 1 | 0..59 |
| 6 | second | 1 | 0..59 |
| 7 | day of week | 1 | 1=Monday..7=Sunday, 0=unknown |
| 8 | fractions256 | 1 | usually 0 |
| 9 | adjust reason | 1 | usually 0 |

### Example

Set the device time to 2026-09-10 14:30:00 Thursday.

- year = 2026 = `0x07EA`
- month = 9
- day = 10
- hour = 14
- minute = 30
- second = 0
- weekday = 4

Payload:

```text
EA 07 09 0A 0E 1E 00 04 00 00
```

This is the exact format expected by the ESP32 firmware.

### Read behavior

Reading the characteristic returns the current device time in the same 10-byte layout.

---

## 2) Custom Wi‑Fi service

Use a custom service for Wi‑Fi credentials and status. There is no standard BLE service that cleanly represents SSID/password and connection state.

Recommended service UUID:
- `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`

### 2.1 Wi‑Fi config characteristic

Characteristic UUID:
- `6E400002-B5A3-F393-E0A9-E50E24DCCA9E`

Properties:
- Write

Purpose:
- configure Wi‑Fi SSID and password

Payload format:

```text
[ssid_len][ssid_bytes...][password_len][password_bytes...]
```

Rules:
- `ssid_len` is 1 byte
- `password_len` is 1 byte
- SSID max length: 31 bytes
- password max length: 63 bytes
- total payload length must match `2 + ssid_len + password_len`

Example:

- SSID: `MyWiFi`
- Password: `secret123`

ASCII bytes:

```text
MyWiFi = 4D 79 57 69 46 69
secret123 = 73 65 63 72 65 74 31 32 33
```

Payload:

```text
06 4D 79 57 69 46 69 09 73 65 63 72 65 74 31 32 33
```

Interpretation:
- `06` = SSID length = 6
- `4D 79 57 69 46 69` = `MyWiFi`
- `09` = password length = 9
- `73 65 63 72 65 74 31 32 33` = `secret123`

### 2.2 Wi‑Fi status characteristic

Characteristic UUID:
- `6E400003-B5A3-F393-E0A9-E50E24DCCA9E`

Properties:
- Read

Purpose:
- report current Wi‑Fi status, associated SSID, and IP address

Payload format:

```text
[status][ssid_len][ssid_bytes...][ip[4]]
```

Status values:

| Value | Meaning |
| --- | --- |
| 0 | disconnected |
| 1 | connecting |
| 2 | connected |

If disconnected, IP bytes may be all zero.

Example connected state:

- status = `2`
- SSID = `MyWiFi`
- IP = `192.168.1.42`

Bytes:

```text
02 06 4D 79 57 69 46 69 C0 A8 01 2A
```

Interpretation:
- `02` = connected
- `06` = SSID length = 6
- `4D 79 57 69 46 69` = `MyWiFi`
- `C0 A8 01 2A` = `192.168.1.42`

### 2.3 Display configuration characteristic

Characteristic UUID:
- `0xFFF3`

Properties:
- Read
- Write

Purpose:
- configure whether the clock shows the date and seconds

Payload format:

```text
[show_date][show_seconds]
```

Each value is one byte:
- `0` = disabled
- `1` = enabled

Read returns the current two-byte configuration. Write requests must contain exactly two bytes, and values other than `0` or `1` are rejected.

---

## 3) Why this is the best fit

This is the best BLE design for your requirements because:

- time is a standard BLE domain, so use the standard Current Time Service
- Wi‑Fi setup is not a standard BLE domain, so use a custom service
- SSID and password belong to one config operation, so combine them into one write characteristic
- Wi‑Fi status is a different read-only concern and should be placed in its own characteristic

---

## 4) Android app usage summary

### Set time
- connect to device
- write to service `0x1805`, characteristic `0x2A2B`
- send 10-byte current time payload

### Get time
- read service `0x1805`, characteristic `0x2A2B`

### Set Wi‑Fi config
- connect to device
- write to custom service `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- write to characteristic `6E400002-B5A3-F393-E0A9-E50E24DCCA9E`
- send `[ssid_len][ssid...][password_len][password...]`

### Get Wi‑Fi status
- read from characteristic `6E400003-B5A3-F393-E0A9-E50E24DCCA9E`
- parse `status`, `ssid`, and `ip`

### Configure display
- read or write service `0xFFF0`, characteristic `0xFFF3`
- send `[show_date][show_seconds]`

---

## 5) Notes

- The ESP32 firmware must persist the SSID/password in NVS so the device reconnects on reboot.
- Bluetooth and Wi‑Fi are separate subsystems; the BLE access functions should not block the UI or network tasks for long.
- For production use, consider adding optional authentication or pairing before accepting Wi‑Fi configuration commands.
