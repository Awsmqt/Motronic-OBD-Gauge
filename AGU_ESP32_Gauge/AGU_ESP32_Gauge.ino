/*
 * ==============================================================================
 * VW 1.8T AGU (Bosch Motronic M3.8.3) - ESP32 CYD Multi-Gauge & OBD Telemetry
 * ==============================================================================
 * 
 * Hardware:
 *   - ESP32 CYD (Cheap Yellow Display - ESP32-2432S028)
 *   - Bosch 3-Bar / 4-Bar MAP Sensor (Ladedruck analog an GPIO 35)
 *   - K-Typ Thermoelement mit Adafruit MAX31855 (Abgastemperatur EGT via SPI)
 *   - Externer Ladeluft-NTC Sensor (Ladeluftkühler-Ausgang an GPIO 34)
 *   - L9637D / Si9241 K-Line Transceiver an Serial2 (KW1281 OBD Protokoll)
 * 
 * Features:
 *   - KW1281 5-Baud Slow Init & automatisches Auslesen von MWB 001, 002, 003
 *   - Auto-Zeroing des barometrischen Umgebungsdrucks beim Einschalten
 *   - 4 umschaltbare Design-Themes (Touch auf den Bildschirm wechselt das Design)
 *   - Integrierter Motor-Simulator für Bench-Testing ohne echtes Fahrzeug
 * ==============================================================================
 */

#include "Config.h"
#include "Sensors.h"
#include "KW1281.h"
#include "Simulator.h"
#include "DisplayUI.h"

// Globale Instanzen
SensorData telemetryData;
SensorManager sensors;
KW1281Protocol kw1281;
MotorSimulator simulator;
DisplayUI ui;

// Timer für Hauptschleife
unsigned long lastSensorSampleTime = 0;
unsigned long lastDisplayRefreshTime = 0;
unsigned long lastSerialLogTime = 0;

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=======================================================");
    Serial.println("  VW 1.8T (AGU) Motronic M3.8.3 ESP32 CYD Multi-Gauge  ");
    Serial.println("=======================================================");

    // Datenstruktur initialisieren
    memset(&telemetryData, 0, sizeof(SensorData));
    telemetryData.currentTheme = THEME_CLASSIC_QUAD;
    telemetryData.boostPeakBar = -1.0f;
    telemetryData.exhaustTempPeak = 0.0f;

    // 1. Display und UI initialisieren
    ui.begin();
    Serial.println("[Setup] Display initialisiert.");

    // 2. Analoge & SPI Sensoren initialisieren (inkl. Auto-Zeroing)
    sensors.begin();
    Serial.println("[Setup] Sensoren kalibriert & bereit.");

    // 3. KW1281 K-Line Diagnose initialisieren
    #if !ENABLE_SIMULATION
        kw1281.begin();
        Serial.println("[Setup] K-Line (KW1281) Protokoll gestartet.");
    #else
        simulator.begin();
        Serial.println("[Setup] SIMULATOR-MODUS AKTIV - Generiere synthetische Motordaten.");
    #endif

    Serial.println("[Setup] System betriebsbereit!\n");
}

void loop() {
    unsigned long now = millis();

    // --------------------------------------------------------------------------
    // 1. SENSOR- & DIAGNOSE-SAMPLING (40 Hz / alle 25ms)
    // --------------------------------------------------------------------------
    if (now - lastSensorSampleTime >= (1000 / SENSOR_SAMPLE_RATE_HZ)) {
        lastSensorSampleTime = now;

        #if ENABLE_SIMULATION
            // Testumgebung: Dynamischer Motor- & Sensordaten-Generator
            simulator.update(telemetryData);
        #else
            // Reales Fahrzeug: Externe Sensoren & K-Line abfragen
            sensors.update(telemetryData);
            kw1281.update(telemetryData);
        #endif
    }

    // --------------------------------------------------------------------------
    // 2. DISPLAY RENDERING & TOUCH (20 Hz / alle 50ms)
    // --------------------------------------------------------------------------
    if (now - lastDisplayRefreshTime >= (1000 / DISPLAY_FPS_TARGET)) {
        lastDisplayRefreshTime = now;

        // Touchscreen prüfen (kurzer Tipp = nächstes Theme, langer Druck = Peak-Reset)
        ui.checkTouch(telemetryData);

        // Aktuelles Layout rendern
        ui.update(telemetryData);
    }

    // --------------------------------------------------------------------------
    // 3. SERIAL TELEMETRIE LOGGING (1 Hz / alle 1000ms auf USB Konsole)
    // --------------------------------------------------------------------------
    if (now - lastSerialLogTime >= 1000) {
        lastSerialLogTime = now;
        Serial.printf("[Telemetrie] Boost: %.2f bar (Peak: %.2f) | EGT: %.0f C | LLK: %.1f C | RPM: %d | Coolant: %.0f C | K-Line: %s\n",
                      telemetryData.boostBar,
                      telemetryData.boostPeakBar,
                      telemetryData.exhaustTemp,
                      telemetryData.chargeAirTemp,
                      telemetryData.ecuRpm,
                      telemetryData.ecuCoolantTemp,
                      (telemetryData.klineConnected ? "ONLINE" : "OFFLINE"));
    }
}
