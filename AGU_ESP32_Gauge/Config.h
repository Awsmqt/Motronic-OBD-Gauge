#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==============================================================================
// BETRIEBSMODUS / TESTUMGEBUNG
// ==============================================================================
// Auf 'true' setzen für Bench-Testing am Schreibtisch (ohne Auto / ohne Sensoren).
// Auf 'false' setzen für den echten Einsatz im VW 1.8T mit K-Line & Sensoren.
#define ENABLE_SIMULATION false

// ==============================================================================
// HARDWARE PIN-BELEGUNG (ESP32 CYD - ESP32-2432S028)
// Alle analogen Sensoren liegen auf ADC1 (Pins 34, 35, 36, 39)
// ==============================================================================
// 1. Ladedruck (MAP-Sensor 0-5V über Spannungsteiler 5V -> 3.3V)
#define PIN_MAP_SENSOR        35    // ADC1_CH7

// 2. Ladelufttemperatur (Charge Air Temp NTC nach Ladeluftkühler)
#define PIN_CHARGE_AIR_NTC    34    // ADC1_CH6 (mit Vorwiderstand zu 3.3V)

// 3. Öldruck (0-10 bar Geber z.B. 0.5-4.5V Druckwandler am Ölfilterhalter)
#define PIN_OIL_PRESSURE      36    // ADC1_CH0 (Sensor VP, mit Teiler 5V -> 3.3V)

// 4. Öltemperatur (NTC/VDO Sensor M10x1 am Ölfilterhalter)
#define PIN_OIL_TEMP          39    // ADC1_CH3 (Sensor VN, mit Vorwiderstand zu 3.3V)

// 5. Abgastemperatur (EGT) - Adafruit MAX31855 SPI Modul
#define PIN_EGT_CS            21    // Chip Select
#define PIN_EGT_DO            22    // MISO (Data Out)
#define PIN_EGT_CLK           27    // SPI Clock

// 6. K-Line / KW1281 Diagnose-Schnittstelle (L9637D Transceiver)
// Liefert digital: Kühlwasser (°C), Batteriespannung (V), Luftmasse etc.
#define PIN_KLINE_RX          16    // Hardware Serial2 RX
#define PIN_KLINE_TX          17    // Hardware Serial2 TX

// 7. CYD Touchscreen (XPT2046 Controller)
#define PIN_TOUCH_CS          33    // SPI Chip Select für Touch

// ==============================================================================
// SENSOR-KALIBRIERUNG
// ==============================================================================
// MAP-Sensor (Bosch 3-Bar: 0.5V = 0.20 bar abs, 4.5V = 3.00 bar abs)
#define MAP_V_MIN             0.50f
#define MAP_V_MAX             4.50f
#define MAP_P_MIN_BAR         0.20f
#define MAP_P_MAX_BAR         3.00f
#define MAP_DIVIDER_RATIO     0.60f  // Spannungsteiler: 3.3k / (2.2k + 3.3k)
#define MAP_BARO_DEFAULT      1.013f // Standard-Umgebungsdruck

// Öldruck-Sensor (Standard: 0-10 bar Geber mit 0.5V - 4.5V Ausgang)
#define OIL_PRESS_V_MIN       0.50f  // 0.5V = 0 bar
#define OIL_PRESS_V_MAX       4.50f  // 4.5V = 10 bar
#define OIL_PRESS_MAX_BAR     10.0f
#define OIL_PRESS_DIVIDER     0.60f  // Spannungsteiler: 3.3k / (2.2k + 3.3k)

// NTC-Sensoren (Ladeluft & Öltemperatur)
#define NTC_PULLUP_RESISTOR   2200.0f // Vorwiderstand 2.2 kΩ gegen 3.3V
#define NTC_NOMINAL_RESISTANCE 2252.0f // Nennwiderstand bei 25°C
#define NTC_NOMINAL_TEMP      25.0f   // Nenntemperatur 25°C
#define NTC_BETA_COEFFICIENT  3977.0f // Beta-Wert

// Filterung
#define FILTER_ALPHA_PRESS    0.25f  // Filter für Drücke (Ladedruck, Öldruck)
#define FILTER_ALPHA_TEMP     0.10f  // Stärkere Glättung für Temperaturen

// ==============================================================================
// ALARMSCHWELLEN & GRENZWERTE (MOTOR-BETRIEBSZUSTAND)
// ==============================================================================
// Öldruck
#define ALARM_OIL_PRESS_MIN_WARN   1.20f // Öldruck zu gering (< 1.2 bar im Leerlauf)
#define ALARM_OIL_PRESS_CRITICAL   0.80f // Akute Motorgefahr (< 0.8 bar)

// Öltemperatur
#define ALARM_OIL_TEMP_COLD        75.0f // Motor noch kalt (< 75°C)
#define ALARM_OIL_TEMP_WARN       120.0f // Öltemperatur hoch (> 120°C)
#define ALARM_OIL_TEMP_CRITICAL   135.0f // Überhitzung Motoröl (> 135°C)

// Kühlwassertemperatur
#define ALARM_COOLANT_HIGH        103.0f // Kühlwasser zu heiß
#define ALARM_COOLANT_CRITICAL    112.0f // Kochendes Kühlmittel

// Ladelufttemperatur
#define ALARM_IAT_HIGH             55.0f // Heat-Soak Warnung

// Abgastemperatur (EGT)
#define ALARM_EGT_WARN            900.0f // Gelbe Warnstufe
#define ALARM_EGT_CRITICAL        950.0f // Rote Alarmstufe (Bauteilschutz)

// Ladedruck
#define ALARM_BOOST_OVERBOOST      1.45f // Unkontrollierter Ladedruck

// Batteriespannung
#define ALARM_BATTERY_LOW         12.0f // Lichtmaschine defekt / Batterie schwach
#define ALARM_BATTERY_HIGH        15.0f // Überspannung / Regler defekt

// ==============================================================================
// DISPLAY-ANSICHTEN
// ==============================================================================
enum DisplayTheme {
    THEME_MEASUREMENTS = 0,
    THEME_HEALTH = 1,
    THEME_COUNT = 2
};

// ==============================================================================
// GEMEINSAME DATENSTRUKTUR (SENSOR & TELEMETRIE)
// ==============================================================================
struct SensorData {
    // 1. DRÜCKE (Pressures)
    float boostBar;           // Relativer Ladedruck (bar)
    float boostPeakBar;       // Maximaler Ladedruck (bar)
    float oilPressureBar;     // Öldruck (bar)
    float oilPressureMinBar;  // Minimaler Öldruck bei Betriebstemperatur (bar)

    // 2. TEMPERATUREN (Temperatures)
    float oilTemp;            // Öltemperatur (°C)
    float oilTempPeak;        // Maximale Öltemperatur (°C)
    float coolantTemp;        // Kühlmitteltemperatur (°C, aus KW1281 oder Geber)
    float chargeAirTemp;      // Ladelufttemperatur nach LLK (°C, externer NTC)
    float exhaustTemp;        // Abgastemperatur EGT (°C, MAX31855)
    float exhaustTempPeak;    // Höchste EGT (°C)

    // 3. ELEKTRIK & MOTOR-STATUS (ECU / KW1281)
    float batteryVolt;        // Bordspannung (V)
    float ecuIntakeAirTemp;   // Ansaugluft vor Lader (aus MAF)
    float ecuEngineLoadPct;   // Motorlast (%)
    float ecuMafGs;           // Luftmasse (g/s)
    int   ecuRpm;             // Motordrehzahl (zur Hintergrund-Berechnung)

    // 4. ERWEITERTE ECU-DATEN (zusätzliche Messwertblöcke)
    float ecuLambda;           // Lambda-Wert (0.0 - 2.0)
    float ecuInjectionMs;      // Einspritzzeit (ms)
    float ecuIgnitionTiming;   // Zündwinkel (°KW v.OT)
    float ecuKnockRetard;      // Klopfregelung Rücknahme (°)
    float ecuThrottlePct;      // Drosselklappenstellung (%)
    uint8_t ecuStatusBits;     // Statusbits (Bit-Feld)

    // 5. DTC (Fehlercodes / Diagnose)
    uint8_t  dtcCount;                // Anzahl gespeicherter Fehlercodes
    uint16_t dtcCodes[16];            // DTC-Nummern (max. 16 Stück)
    uint8_t  dtcStatus[16];           // DTC-Status pro Code
    bool     dtcReadPending;          // Anforderung: DTCs auslesen
    bool     dtcClearPending;         // Anforderung: DTCs löschen
    bool     milSeen;                 // MIL-Leuchte: mind. 1 DTC vorhanden
    uint32_t dtcLastReadMs;           // millis() des letzten erfolgreichen DTC-Lesens (0 = noch nie)

    // 6. LOGGER & SD-KARTE
    bool     sdCardReady;             // SD-Karte eingelegt und bereit
    uint32_t logLinesWritten;         // Anzahl geschriebener Log-Zeilen

    // 7. ALARME & STATUS
    bool  klineConnected;     // Verbindung zur ECU aktiv
    uint32_t klinePackets;
    bool  alarmOilPressure;   // Öldruck zu niedrig!
    bool  alarmOilTemp;       // Öltemperatur zu hoch!
    bool  alarmCoolant;       // Kühlwasser zu heiß!
    bool  alarmChargeAir;     // Ladeluft überhitzt!
    bool  alarmEgt;           // EGT kritisch!
    bool  alarmBoost;         // Overboost!
    bool  alarmBattery;       // Batteriespannung unplausibel

    DisplayTheme currentTheme;
};

// ==============================================================================
// SD-KARTEN LOGGING (HSPI — CYD eingebauter SD-Slot)
// ==============================================================================
#define PIN_SD_CS              5     // HSPI Chip Select für SD-Karte
#define LOG_INTERVAL_MS        1000  // Schreib-Intervall in ms (1 Hz)
#define LOG_BUFFER_LINES       10    // Zeilen im RAM-Puffer vor Flush auf SD
#define LOG_MAX_FILE_SIZE_MB   10    // Maximale Dateigröße pro Session (MB)
#define LOG_MAX_SESSIONS       50    // Maximale Anzahl Session-Dateien auf SD
#define ENABLE_SD_LOGGING      true  // SD-Karten Logging aktivieren/deaktivieren

// ==============================================================================
// TIMING / ABTASTRATEN
// ==============================================================================
#define SENSOR_SAMPLE_RATE_HZ  40    // Sensor-Abtastrate (Hz)
#define DISPLAY_FPS_TARGET     20    // Display-Aktualisierungsrate (FPS)

// ==============================================================================
// WIFI & WEB-DASHBOARD (FÜR IPHONE / SMARTPHONE LIVE-ANZEIGE)
// ==============================================================================
#define ENABLE_WIFI_DASHBOARD  true               // ESP32 Access-Point & Web-Server
#define WIFI_AP_SSID           "VW-1.8T-AGU-Gauge" // WiFi-Netzwerkname
#define WIFI_AP_PASS           "motronic18t"      // WiFi-Passwort (mind. 8 Zeichen)
#define WIFI_AP_CHANNEL        1                  // WiFi-Kanal
#define WEB_SERVER_PORT        80                 // HTTP-Port (Standard 80)

#endif // CONFIG_H
