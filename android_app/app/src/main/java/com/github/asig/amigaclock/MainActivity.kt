package com.github.asig.amigaclock

import android.Manifest
import android.annotation.SuppressLint
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.Checkbox
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Slider
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.getValue
import androidx.compose.runtime.setValue
import androidx.compose.foundation.lazy.items
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.unit.dp

class MainActivity : ComponentActivity() {
    private lateinit var bleManager: BleManager

    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        bleManager = BleManager(this)

        requestBlePermissions()

        setContent {
            MaterialTheme {
                Surface(modifier = Modifier.fillMaxSize()) {
                    AmigaClockScreen(bleManager)
                }
            }
        }
    }

    private fun requestBlePermissions() {
        val perms = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            arrayOf(
                Manifest.permission.BLUETOOTH_SCAN,
                Manifest.permission.BLUETOOTH_CONNECT
            )
        } else {
            arrayOf(Manifest.permission.ACCESS_FINE_LOCATION)
        }
        permissionLauncher.launch(perms)
    }
}

@SuppressLint("MissingPermission")
@Composable
fun AmigaClockScreen(ble: BleManager) {
    val isConnected by ble.isConnected.collectAsState()
    val isScanning by ble.isScanning.collectAsState()
    val devices by ble.discoveredDevices.collectAsState()
    val wifiStatus by ble.wifiStatus.collectAsState()
    val displayConfig by ble.displayConfig.collectAsState()
    val backlightPercent by ble.backlightPercent.collectAsState()
    val logMessage by ble.logMessage.collectAsState()

    var ssid by remember { mutableStateOf("") }
    var password by remember { mutableStateOf("") }
    var sliderPosition by remember { mutableFloatStateOf(100f) }

    LaunchedEffect(backlightPercent) {
        backlightPercent?.let {
            sliderPosition = it.toFloat()
        }
    }

    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        Text("AmigaClock Konfiguration", style = MaterialTheme.typography.headlineMedium)
        Text(
            "Status: $logMessage",
            style = MaterialTheme.typography.bodySmall,
            color = MaterialTheme.colorScheme.secondary
        )

        Spacer(modifier = Modifier.height(12.dp))

        if (!isConnected) {
            Button(
                onClick = { if (isScanning) ble.stopScan() else ble.startScan() },
                modifier = Modifier.fillMaxWidth()
            ) {
                Text(if (isScanning) "Scan stoppen" else "Nach AmigaClock suchen")
            }

            Spacer(modifier = Modifier.height(8.dp))

            LazyColumn {
                items(devices) { device ->
                    Card(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(vertical = 4.dp)
                            .clickable { ble.connect(device) }
                    ) {
                        Column(modifier = Modifier.padding(12.dp)) {
                            Text(
                                device.name ?: "Unbekanntes Gerät",
                                style = MaterialTheme.typography.titleMedium
                            )
                            Text(device.address, style = MaterialTheme.typography.bodySmall)
                        }
                    }
                }
            }
        } else {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .verticalScroll(rememberScrollState())
            ) {
                Button(
                    onClick = { ble.disconnect() },
                    colors = ButtonDefaults.buttonColors(containerColor = MaterialTheme.colorScheme.error),
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("Trennen")
                }

                Spacer(modifier = Modifier.height(16.dp))

                // 1. Zeitabgleich
                Card(modifier = Modifier.fillMaxWidth()) {
                    Column(modifier = Modifier.padding(12.dp)) {
                        Text(
                            "1. Uhrzeit abgleichen (0x1805)",
                            style = MaterialTheme.typography.titleMedium
                        )
                        Spacer(modifier = Modifier.height(8.dp))
                        Button(onClick = { ble.syncTime() }) {
                            Text("Smartphone-Zeit senden")
                        }
                    }
                }

                Spacer(modifier = Modifier.height(12.dp))

                // 2. Wi-Fi Setup
                Card(modifier = Modifier.fillMaxWidth()) {
                    Column(modifier = Modifier.padding(12.dp)) {
                        Text("2. Wi-Fi Konfiguration", style = MaterialTheme.typography.titleMedium)

                        wifiStatus?.let {
                            Text("Status: ${it.statusText} | IP: ${it.ip} | SSID: ${it.ssid}")
                        }

                        OutlinedTextField(
                            value = ssid,
                            onValueChange = { ssid = it },
                            label = { Text("SSID") },
                            modifier = Modifier.fillMaxWidth()
                        )
                        OutlinedTextField(
                            value = password,
                            onValueChange = { password = it },
                            label = { Text("Passwort") },
                            visualTransformation = PasswordVisualTransformation(),
                            modifier = Modifier.fillMaxWidth()
                        )
                        Spacer(modifier = Modifier.height(8.dp))
                        Row {
                            Button(onClick = { ble.sendWifiConfig(ssid, password) }) {
                                Text("Senden")
                            }
                            Spacer(modifier = Modifier.width(8.dp))
                            OutlinedButton(onClick = { ble.readWifiStatus() }) {
                                Text("Status abfragen")
                            }
                        }
                    }
                }

                Spacer(modifier = Modifier.height(12.dp))

                // 3. Display-Optionen
                Card(modifier = Modifier.fillMaxWidth()) {
                    Column(modifier = Modifier.padding(12.dp)) {
                        Text(
                            "3. Display Anzeige (0xFFF3)",
                            style = MaterialTheme.typography.titleMedium
                        )
                        val showDate = displayConfig?.showDate ?: false
                        val showSeconds = displayConfig?.showSeconds ?: false

                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Checkbox(
                                checked = showDate,
                                onCheckedChange = { ble.sendDisplayConfig(it, showSeconds) }
                            )
                            Text("Datum anzeigen")
                        }
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Checkbox(
                                checked = showSeconds,
                                onCheckedChange = { ble.sendDisplayConfig(showDate, it) }
                            )
                            Text("Sekunden anzeigen")
                        }
                    }
                }

                Spacer(modifier = Modifier.height(12.dp))

                // 4. Backlight Helligkeit
                Card(modifier = Modifier.fillMaxWidth()) {
                    Column(modifier = Modifier.padding(12.dp)) {
                        Text(
                            "4. Hintergrundbeleuchtung (0xFFF4)",
                            style = MaterialTheme.typography.titleMedium
                        )
                        Spacer(modifier = Modifier.height(8.dp))
                        Text("Helligkeit: ${sliderPosition.toInt()}%")
                        Slider(
                            value = sliderPosition,
                            onValueChange = { sliderPosition = it },
                            valueRange = 0f..100f,
                            steps = 99,
                            onValueChangeFinished = {
                                ble.sendBacklight(sliderPosition.toInt())
                            },
                            modifier = Modifier.fillMaxWidth()
                        )
                        Row {
                            Button(onClick = { ble.sendBacklight(sliderPosition.toInt()) }) {
                                Text("Setzen (${sliderPosition.toInt()}%)")
                            }
                            Spacer(modifier = Modifier.width(8.dp))
                            OutlinedButton(onClick = { ble.readBacklight() }) {
                                Text("Helligkeit abfragen")
                            }
                        }
                    }
                }
            }
        }
    }
}