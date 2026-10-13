# Android companion app

This Android companion app allows you to configure and control the AmigaClock device over Bluetooth Low Energy (BLE).

## Features

- **Time Sync**: Set the device time from your smartphone
- **Wi-Fi Configuration**: Configure Wi-Fi credentials over BLE
- **Display Settings**: Control date and seconds display
- **Backlight Control**: Adjust display brightness
- **Display Resolution**: Switch between high-resolution (hires) and low-resolution (lores) modes

## Requirements

- Android 5.0 (API level 21) or higher
- Bluetooth Low Energy (BLE) support
- Permissions: BLUETOOTH_SCAN, BLUETOOTH_CONNECT (Android 12+), ACCESS_FINE_LOCATION (Android 11 and below)

## Building

### Using Gradle (command line)

```bash
cd android_app
./gradlew build
```

### Using Gradle (install on device)

```bash
./gradlew installDebug
```

### Using Android Studio

1. Open Android Studio
2. Select "File" → "Open..."
3. Navigate to the `android_app` directory
4. Select "Build" → "Make Project" or press `Ctrl+F9` to build
5. Select "Run" → "Run 'app'" or press `Shift+F10` to run on a device

## Usage

1. Ensure the AmigaClock device is powered on and advertising
2. Launch the app
3. Tap "Nach AmigaClock suchen" (Search for AmigaClock) to scan for devices
4. Select the AmigaClock device from the list to connect
5. Use the UI panels to:
   - Sync time
   - Configure Wi-Fi
   - Adjust display options
   - Control backlight
   - Change resolution

## BLE Protocol

For details on the BLE protocol used to communicate with the device, see [BLE-SPECS.md](../BLE-SPECS.md).
