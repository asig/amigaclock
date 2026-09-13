package com.github.asig.amigaclock

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.content.Context
import android.os.Build
import kotlinx.coroutines.flow.MutableStateFlow
import java.util.UUID

@SuppressLint("MissingPermission")
class BleManager(private val context: Context) {
    private val adapter: BluetoothAdapter? =
        (context.getSystemService(Context.BLUETOOTH_SERVICE) as? BluetoothManager)?.adapter

    private var gatt: BluetoothGatt? = null

    val isConnected = MutableStateFlow(false)
    val isScanning = MutableStateFlow(false)
    val discoveredDevices = MutableStateFlow<List<BluetoothDevice>>(emptyList())
    val wifiStatus = MutableStateFlow<AmigaClockProtocol.WifiStatus?>(null)
    val displayConfig = MutableStateFlow<AmigaClockProtocol.DisplayConfig?>(null)
    val backlightPercent = MutableStateFlow<Int?>(null)
    val logMessage = MutableStateFlow("Bereit")

    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val device = result.device
            if (device.name != null && !discoveredDevices.value.contains(device)) {
                discoveredDevices.value = discoveredDevices.value + device
            }
        }
    }

    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                isConnected.value = true
                logMessage.value = "Verbunden. Suche Services..."
                gatt.discoverServices()
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                isConnected.value = false
                wifiStatus.value = null
                displayConfig.value = null
                backlightPercent.value = null
                logMessage.value = "Verbindung getrennt"
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                logMessage.value = "Services gefunden. Lese Status..."
                // Sequentielles Lesen starten
                readWifiStatus()
            }
        }

        override fun onCharacteristicRead(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            value: ByteArray,
            status: Int
        ) {
            val data = if (status == BluetoothGatt.GATT_SUCCESS) value else null
            handleCharacteristicData(characteristic.uuid, data)
        }

        @Deprecated("Deprecated for SDK >= 33")
        override fun onCharacteristicRead(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            status: Int
        ) {
            val data = if (status == BluetoothGatt.GATT_SUCCESS) characteristic.value else null
            handleCharacteristicData(characteristic.uuid, data)
        }

        override fun onCharacteristicWrite(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            status: Int
        ) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                logMessage.value = "Befehl erfolgreich gesendet"
            } else {
                logMessage.value = "Schreibfehler ($status)"
            }
        }
    }

    private fun handleCharacteristicData(uuid: UUID, data: ByteArray?) {
        when (uuid) {
            AmigaClockProtocol.WIFI_STATUS_CHAR_UUID -> {
                if (data != null) wifiStatus.value = AmigaClockProtocol.parseWifiStatus(data)
                // Kette fortsetzen: Display-Config lesen
                readDisplayConfig()
            }
            AmigaClockProtocol.DISPLAY_CHAR_UUID -> {
                if (data != null) displayConfig.value = AmigaClockProtocol.parseDisplayConfig(data)
                // Kette fortsetzen: Backlight lesen
                readBacklight()
            }
            AmigaClockProtocol.BACKLIGHT_CHAR_UUID -> {
                if (data != null) backlightPercent.value = AmigaClockProtocol.parseBacklight(data)
                logMessage.value = "Status aktualisiert"
            }
        }
    }
    fun startScan() {
        discoveredDevices.value = emptyList()
        isScanning.value = true
        adapter?.bluetoothLeScanner?.startScan(scanCallback)
    }

    fun stopScan() {
        isScanning.value = false
        adapter?.bluetoothLeScanner?.stopScan(scanCallback)
    }

    fun connect(device: BluetoothDevice) {
        stopScan()
        gatt?.close()
        gatt = device.connectGatt(context, false, gattCallback, BluetoothDevice.TRANSPORT_LE)
    }

    fun disconnect() {
        gatt?.disconnect()
    }

    fun syncTime() {
        val payload = AmigaClockProtocol.buildTimePayload()
        writeCharacteristic(
            AmigaClockProtocol.TIME_SERVICE_UUID,
            AmigaClockProtocol.CURRENT_TIME_CHAR_UUID,
            payload
        )
    }

    fun sendWifiConfig(ssid: String, pass: String) {
        val payload = AmigaClockProtocol.buildWifiPayload(ssid, pass)
        writeCharacteristic(
            AmigaClockProtocol.WIFI_SERVICE_UUID,
            AmigaClockProtocol.WIFI_CONFIG_CHAR_UUID,
            payload
        )
    }

    fun readWifiStatus() {
        readCharacteristic(
            AmigaClockProtocol.WIFI_SERVICE_UUID,
            AmigaClockProtocol.WIFI_STATUS_CHAR_UUID
        )
    }

    fun sendDisplayConfig(showDate: Boolean, showSeconds: Boolean) {
        // 1. Lokalen Zustand sofort aktualisieren, damit UI und Folgebefehle den aktuellen Stand haben
        displayConfig.value = AmigaClockProtocol.DisplayConfig(showDate, showSeconds)

        val payload = AmigaClockProtocol.buildDisplayPayload(showDate, showSeconds)
        writeCharacteristic(
            AmigaClockProtocol.DISPLAY_SERVICE_UUID,
            AmigaClockProtocol.DISPLAY_CHAR_UUID,
            payload
        )
    }

    fun readDisplayConfig() {
        readCharacteristic(
            AmigaClockProtocol.DISPLAY_SERVICE_UUID,
            AmigaClockProtocol.DISPLAY_CHAR_UUID
        )
    }

    fun sendBacklight(percent: Int) {
        val clamped = percent.coerceIn(0, 100)
        // Lokalen Zustand sofort nachführen
        backlightPercent.value = clamped

        val payload = AmigaClockProtocol.buildBacklightPayload(clamped)
        writeCharacteristic(
            AmigaClockProtocol.DISPLAY_SERVICE_UUID,
            AmigaClockProtocol.BACKLIGHT_CHAR_UUID,
            payload
        )
    }

    fun readBacklight() {
        readCharacteristic(
            AmigaClockProtocol.DISPLAY_SERVICE_UUID,
            AmigaClockProtocol.BACKLIGHT_CHAR_UUID
        )
    }

    private fun writeCharacteristic(serviceUuid: UUID, charUuid: UUID, data: ByteArray) {
        val service = gatt?.getService(serviceUuid) ?: return
        val char = service.getCharacteristic(charUuid) ?: return

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            gatt?.writeCharacteristic(char, data, BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
        } else {
            @Suppress("DEPRECATION")
            char.value = data
            @Suppress("DEPRECATION")
            gatt?.writeCharacteristic(char)
        }
    }

    private fun readCharacteristic(serviceUuid: UUID, charUuid: UUID) {
        val service = gatt?.getService(serviceUuid) ?: return
        val char = service.getCharacteristic(charUuid) ?: return
        gatt?.readCharacteristic(char)
    }
}