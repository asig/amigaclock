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
    private val readQueue = ArrayDeque<Pair<UUID, UUID>>()

    val isConnected = MutableStateFlow(false)
    val isScanning = MutableStateFlow(false)
    val discoveredDevices = MutableStateFlow<List<BluetoothDevice>>(emptyList())
    val wifiStatus = MutableStateFlow<AmigaClockProtocol.WifiStatus?>(null)
    val displayConfig = MutableStateFlow<AmigaClockProtocol.DisplayConfig?>(null)
    val backlightPercent = MutableStateFlow<Int?>(null)
    val resolution = MutableStateFlow<AmigaClockProtocol.DisplayResolution?>(null)
    val antialias = MutableStateFlow<Boolean?>(null)
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
                resolution.value = null
                antialias.value = null
                readQueue.clear()
                logMessage.value = "Verbindung getrennt"
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                logMessage.value = "Services gefunden. Lese Status..."
                readQueue.clear()
                enqueueRead(AmigaClockProtocol.WIFI_SERVICE_UUID, AmigaClockProtocol.WIFI_STATUS_CHAR_UUID)
                enqueueRead(AmigaClockProtocol.DISPLAY_SERVICE_UUID, AmigaClockProtocol.DISPLAY_CHAR_UUID)
                enqueueRead(AmigaClockProtocol.DISPLAY_SERVICE_UUID, AmigaClockProtocol.BACKLIGHT_CHAR_UUID)
                enqueueRead(AmigaClockProtocol.DISPLAY_SERVICE_UUID, AmigaClockProtocol.RESOLUTION_CHAR_UUID)
                enqueueRead(AmigaClockProtocol.DISPLAY_SERVICE_UUID, AmigaClockProtocol.ANTIALIAS_CHAR_UUID)
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
            advanceReadQueue(characteristic.uuid)
        }

        @Deprecated("Deprecated for SDK >= 33")
        override fun onCharacteristicRead(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            status: Int
        ) {
            val data = if (status == BluetoothGatt.GATT_SUCCESS) characteristic.value else null
            handleCharacteristicData(characteristic.uuid, data)
            advanceReadQueue(characteristic.uuid)
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

    private fun advanceReadQueue(uuid: UUID) {
        val currentRead = readQueue.firstOrNull()
        if (currentRead != null && currentRead.second == uuid) {
            readQueue.removeFirst()
        }
        readNextCharacteristic()
    }

    private fun handleCharacteristicData(uuid: UUID, data: ByteArray?) {
        when (uuid) {
            AmigaClockProtocol.WIFI_STATUS_CHAR_UUID -> {
                if (data != null) wifiStatus.value = AmigaClockProtocol.parseWifiStatus(data)
            }
            AmigaClockProtocol.DISPLAY_CHAR_UUID -> {
                if (data != null) displayConfig.value = AmigaClockProtocol.parseDisplayConfig(data)
            }
            AmigaClockProtocol.BACKLIGHT_CHAR_UUID -> {
                if (data != null) backlightPercent.value = AmigaClockProtocol.parseBacklight(data)
            }
            AmigaClockProtocol.RESOLUTION_CHAR_UUID -> {
                if (data != null) resolution.value = AmigaClockProtocol.parseResolution(data)
                logMessage.value = "Status aktualisiert"
            }
            AmigaClockProtocol.ANTIALIAS_CHAR_UUID -> {
                if (data != null) antialias.value = AmigaClockProtocol.parseAntialias(data)
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
        enqueueRead(
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
        enqueueRead(
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
        enqueueRead(
            AmigaClockProtocol.DISPLAY_SERVICE_UUID,
            AmigaClockProtocol.BACKLIGHT_CHAR_UUID
        )
    }

    fun sendResolution(resolution: AmigaClockProtocol.DisplayResolution) {
        // Lokalen Zustand sofort nachführen
        this.resolution.value = resolution

        val payload = AmigaClockProtocol.buildResolutionPayload(resolution)
        writeCharacteristic(
            AmigaClockProtocol.DISPLAY_SERVICE_UUID,
            AmigaClockProtocol.RESOLUTION_CHAR_UUID,
            payload
        )
    }

    fun readResolution() {
        enqueueRead(
            AmigaClockProtocol.DISPLAY_SERVICE_UUID,
            AmigaClockProtocol.RESOLUTION_CHAR_UUID
        )
    }

    fun sendAntialias(enabled: Boolean) {
        antialias.value = enabled
        writeCharacteristic(
            AmigaClockProtocol.DISPLAY_SERVICE_UUID,
            AmigaClockProtocol.ANTIALIAS_CHAR_UUID,
            AmigaClockProtocol.buildAntialiasPayload(enabled)
        )
    }

    fun readAntialias() {
        enqueueRead(
            AmigaClockProtocol.DISPLAY_SERVICE_UUID,
            AmigaClockProtocol.ANTIALIAS_CHAR_UUID
        )
    }

    private fun enqueueRead(serviceUuid: UUID, charUuid: UUID) {
        val wasEmpty = readQueue.isEmpty()
        readQueue.add(Pair(serviceUuid, charUuid))
        if (wasEmpty) {
            readNextCharacteristic()
        }
    }

    private fun readNextCharacteristic() {

        if (readQueue.isEmpty()) return
        val (serviceUuid, charUuid) = readQueue.first()
        val service = gatt?.getService(serviceUuid)
        if (service != null) {
            val char = service.getCharacteristic(charUuid)
            if (char != null) {
                if (gatt?.readCharacteristic(char) == true) {
                    return // Wait for onCharacteristicRead callback
                }
            }
        }
        // Could not start read (missing service/characteristic or read failed), skip to next
        readQueue.removeFirst()
        readNextCharacteristic()
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


}