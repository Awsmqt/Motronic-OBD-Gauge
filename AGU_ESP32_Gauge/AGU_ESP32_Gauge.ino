/*
 * ==============================================================================
 * VW 1.8T AGU (Bosch Motronic M3.8.3) - ESP32 CYD Motor-Betriebszustand Monitor
 * ==============================================================================
 * 
 * Fokus: Motor-Gesundheit & Vitaldaten (Drücke, Temperaturen, Elektrik):
 *   - Ladedruck (relativ in bar) & Öldruck (0-10 bar Geber)
 *   - Öltemperatur, Kühlwasser, Ladeluft (LLK), Abgastemperatur (EGT)
 *   - Bordspannung / Batterie (V)
 * 
 * Hardware:
 *   - ESP32 CYD (Cheap Yellow Display - ESP32-2432S028)
 *   - Bosch 3-Bar / 4-Bar MAP Sensor (Ladedruck analog an GPIO 35)
 *   - 0-10 Bar Öldruck-Sensor (analog an GPIO 36)
 *   - NTC Öltemperatur-Sensor (analog an GPIO 39)
 *   - NTC Ladelufttemperatur-Sensor (analog an GPIO 34 nach LLK)
 *   - K-Typ Thermoelement mit Adafruit MAX31855 (Abgastemperatur EGT via SPI)
 *   - L9637D K-Line Transceiver an Serial2 (KW1281 OBD für Kühlwasser & Bordspannung)
 * ==============================================================================
 */

#include "Config.h"
#include "Sensors.h"
#include "KW1281.h"
#include "Simulator.h"
#include "DisplayUI.h"
#include "Logger.h"
#include "WebDashboard.h"

// Globale Instanzen
SensorData telemetryData;
SensorManager sensors;
KW1281Protocol kw1281;
MotorSimulator simulator;
DisplayUI ui;
DataLogger logger;
#if ENABLE_WIFI_DASHBOARD
WebDashboard webDashboard;
#endif

// Timer für Hauptschleife
unsigned long lastSensorSampleTime = 0;
unsigned long lastDisplayRefreshTime = 0;
unsigned long lastSerialLogTime = 0;

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=======================================================");
    Serial.println("  VW 1.8T (AGU) Motronic M3.8.3 Motor-Zustand Monitor  ");
    Serial.println("=======================================================");

    // Datenstruktur initialisieren
    memset(&telemetryData, 0, sizeof(SensorData));
    telemetryData.currentTheme = THEME_MEASUREMENTS;
    telemetryData.boostPeakBar = -1.0f;
    telemetryData.exhaustTempPeak = 0.0f;
    telemetryData.oilTempPeak = 0.0f;
    telemetryData.oilPressureMinBar = 0.0f;

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

    // 4. SD-Karten Datenlogger initialisieren
    #if ENABLE_SD_LOGGING
        bool sdOk = logger.begin();
        telemetryData.sdCardReady = sdOk;
        if (sdOk) {
            Serial.printf("[Setup] SD-Logger aktiv! Datei: %s\n", logger.getFilename().c_str());
        }
    #endif

    // 5. WiFi Access-Point & Web-Dashboard (iPhone / Smartphone) starten
    #if ENABLE_WIFI_DASHBOARD
        webDashboard.begin();
        Serial.println("[Setup] WiFi Web-Dashboard bereit.");
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
            simulator.update(telemetryData);
        #else
            sensors.update(telemetryData);
            kw1281.update(telemetryData);
        #endif

        // Status für SD-Logging aktualisieren
        telemetryData.sdCardReady = logger.isReady();
        telemetryData.logLinesWritten = logger.getLogCount();
    }

    #if !ENABLE_SIMULATION
        if (kw1281.isSlowInitActive()) {
            kw1281.update(telemetryData);
        }
    #endif

    // --------------------------------------------------------------------------
    // 2. SD-KARTEN DATENLOGGING (1 Hz / zyklisch in RAM-Puffer schreiben)
    // --------------------------------------------------------------------------
    #if ENABLE_SD_LOGGING
        logger.update(telemetryData);
    #endif

    // --------------------------------------------------------------------------
    // 3. WIFI WEB-DASHBOARD (iPhone Anfragen bedienen)
    // --------------------------------------------------------------------------
    #if ENABLE_WIFI_DASHBOARD
        webDashboard.update(telemetryData);
    #endif

    // --------------------------------------------------------------------------
    // 4. DISPLAY RENDERING & TOUCH (20 Hz / alle 50ms)
    // --------------------------------------------------------------------------
    if (now - lastDisplayRefreshTime >= (1000 / DISPLAY_FPS_TARGET)) {
        lastDisplayRefreshTime = now;

        // Touchscreen prüfen (kurzer Tipp = nächstes Theme, langer Druck = Peak-Reset)
        ui.checkTouch(telemetryData);

        // Aktuelles Layout rendern
        ui.update(telemetryData);
    }

    // --------------------------------------------------------------------------
    // 5. SERIAL TELEMETRIE LOGGING (1 Hz / alle 1000ms auf USB Konsole)
    // --------------------------------------------------------------------------
    if (now - lastSerialLogTime >= 1000) {
        lastSerialLogTime = now;
        Serial.printf("[Motor-Status] Boost: %.2f bar | Oel-P: %.1f bar | Oel-T: %.0f C | H2O: %.0f C | LLK: %.1f C | EGT: %.0f C | Lambda: %.3f | Batt: %.1f V\n",
                      telemetryData.boostBar,
                      telemetryData.oilPressureBar,
                      telemetryData.oilTemp,
                      telemetryData.coolantTemp,
                      telemetryData.chargeAirTemp,
                      telemetryData.exhaustTemp,
                      telemetryData.ecuLambda,
                      telemetryData.batteryVolt);
    }
}
