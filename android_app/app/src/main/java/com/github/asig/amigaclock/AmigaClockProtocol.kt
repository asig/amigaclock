package com.github.asig.amigaclock

import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.time.LocalDateTime
import java.util.UUID

object AmigaClockProtocol {
    // Service & Characteristic UUIDs
    val TIME_SERVICE_UUID: UUID = UUID.fromString("00001805-0000-1000-8000-00805f9b34fb")
    val CURRENT_TIME_CHAR_UUID: UUID = UUID.fromString("00002a2b-0000-1000-8000-00805f9b34fb")

    val WIFI_SERVICE_UUID: UUID = UUID.fromString("6E400001-B5A3-F393-E0A9-E50E24DCCA9E")
    val WIFI_CONFIG_CHAR_UUID: UUID = UUID.fromString("6E400002-B5A3-F393-E0A9-E50E24DCCA9E")
    val WIFI_STATUS_CHAR_UUID: UUID = UUID.fromString("6E400003-B5A3-F393-E0A9-E50E24DCCA9E")

    val DISPLAY_SERVICE_UUID: UUID = UUID.fromString("0000fff0-0000-1000-8000-00805f9b34fb")
    val DISPLAY_CHAR_UUID: UUID = UUID.fromString("0000fff3-0000-1000-8000-00805f9b34fb")
    val BACKLIGHT_CHAR_UUID: UUID = UUID.fromString("0000fff4-0000-1000-8000-00805f9b34fb")

    data class WifiStatus(val status: Int, val ssid: String, val ip: String) {
        val statusText: String
            get() = when (status) {
                0 -> "Getrennt"
                1 -> "Verbinde..."
                2 -> "Verbunden"
                else -> "Unbekannt ($status)"
            }
    }

    data class DisplayConfig(val showDate: Boolean, val showSeconds: Boolean)

    /** Packt die aktuelle Zeit in 10 Bytes (Little-Endian) */
    fun buildTimePayload(time: LocalDateTime = LocalDateTime.now()): ByteArray {
        val buffer = ByteBuffer.allocate(10).order(ByteOrder.LITTLE_ENDIAN)
        buffer.putShort(time.year.toShort())
        buffer.put(time.monthValue.toByte())
        buffer.put(time.dayOfMonth.toByte())
        buffer.put(time.hour.toByte())
        buffer.put(time.minute.toByte())
        buffer.put(time.second.toByte())
        buffer.put(time.dayOfWeek.value.toByte()) // 1 = Montag .. 7 = Sonntag
        buffer.put(0.toByte())                   // fractions256
        buffer.put(0.toByte())                   // adjust reason
        return buffer.array()
    }

    /** Packt [ssid_len][ssid][pwd_len][pwd] */
    fun buildWifiPayload(ssid: String, pass: String): ByteArray {
        val ssidBytes = ssid.toByteArray(Charsets.UTF_8).take(31).toByteArray()
        val passBytes = pass.toByteArray(Charsets.UTF_8).take(63).toByteArray()

        val buffer = ByteBuffer.allocate(2 + ssidBytes.size + passBytes.size)
        buffer.put(ssidBytes.size.toByte())
        buffer.put(ssidBytes)
        buffer.put(passBytes.size.toByte())
        buffer.put(passBytes)
        return buffer.array()
    }

    /** Packt [show_date][show_seconds] */
    fun buildDisplayPayload(showDate: Boolean, showSeconds: Boolean): ByteArray {
        return byteArrayOf(
            if (showDate) 1.toByte() else 0.toByte(),
            if (showSeconds) 1.toByte() else 0.toByte()
        )
    }

    /** Packt [percent] (0..100) */
    fun buildBacklightPayload(percent: Int): ByteArray {
        val clamped = percent.coerceIn(0, 100)
        return byteArrayOf(clamped.toByte())
    }

    /** Liest [status][ssid_len][ssid...][ip[4]] */
    fun parseWifiStatus(data: ByteArray): WifiStatus? {
        if (data.size < 2) return null
        val status = data[0].toInt()
        val ssidLen = data[1].toInt() and 0xFF
        if (data.size < 2 + ssidLen + 4) return null

        val ssid = String(data, 2, ssidLen, Charsets.UTF_8)
        val ipOffset = 2 + ssidLen
        val ip = "${data[ipOffset].toUByte()}.${data[ipOffset + 1].toUByte()}." +
                 "${data[ipOffset + 2].toUByte()}.${data[ipOffset + 3].toUByte()}"

        return WifiStatus(status, ssid, ip)
    }

    /** Liest [show_date][show_seconds] */
    fun parseDisplayConfig(data: ByteArray): DisplayConfig? {
        if (data.size < 2) return null
        return DisplayConfig(data[0] == 1.toByte(), data[1] == 1.toByte())
    }

    /** Liest [percent] (0..100) */
    fun parseBacklight(data: ByteArray): Int? {
        if (data.isEmpty()) return null
        return data[0].toInt() and 0xFF
    }
}