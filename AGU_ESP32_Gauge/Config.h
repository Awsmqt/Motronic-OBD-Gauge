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
// ==============================================================================
// 1. Ladedruck (MAP-Sensor 0-5V über Spannungsteiler 5V -> 3.3V)
#define PIN_MAP_SENSOR        35    // ADC1_CH7 (Freier Analogeingang am CYD CN1 Header)

// 2. Ladelufttemperatur (Charge Air Temp NTC z.B. Bosch M12 oder 10k NTC)
#define PIN_CHARGE_AIR_NTC    34    // ADC1_CH6 (Analogeingang mit Vorwiderstand zu 3.3V)

// 3. Abgastemperatur (EGT) - Adafruit MAX31855 SPI Modul
#define PIN_EGT_CS            21    // Chip Select
#define PIN_EGT_DO            22    // MISO (Data Out)
#define PIN_EGT_CLK           27    // SPI Clock

// 4. K-Line / KW1281 Diagnose-Schnittstelle (L9637D oder Si9241 Transceiver)
#define PIN_KLINE_RX          16    // Hardware Serial2 RX
#define PIN_KLINE_TX          17    // Hardware Serial2 TX

// 5. CYD Touchscreen (XPT2046 Controller)
#define PIN_TOUCH_CS          33    // SPI Chip Select für Touch
#define PIN_TOUCH_IRQ         36    // Interrupt Pin (optional)

// 6. CYD On-board RGB LED & LDR Helligkeitssensor
#define PIN_CYD_LDR           34    // Lichtsensor (sofern nicht für NTC belegt)
#define PIN_CYD_RGB_RED        4    // Status-LED Rot (active LOW)
#define PIN_CYD_RGB_GREEN     16    // Falls K-Line frei, sonst via Software

// ==============================================================================
// SENSOR-KALIBRIERUNG
// ==============================================================================
// MAP-Sensor Kennlinie (Standard: Bosch 3-Bar Sensor 0 281 002 437)
// Ausgang: 0.5V = 0.20 bar absolut, 4.5V = 3.00 bar absolut
#define MAP_V_MIN             0.50f  // Spannung bei Minimaldruck (Volt)
#define MAP_V_MAX             4.50f  // Spannung bei Maximaldruck (Volt)
#define MAP_P_MIN_BAR         0.20f  // Druck bei MAP_V_MIN (bar absolut)
#define MAP_P_MAX_BAR         3.00f  // Druck bei MAP_V_MAX (bar absolut)
#define MAP_DIVIDER_RATIO     0.60f  // Spannungsteiler Faktor: z.B. 3.3k / (2.2k + 3.3k) = 0.60
#define MAP_BARO_DEFAULT      1.013f // Standard-Umgebungsdruck auf Meereshöhe (bar)

// Ladeluft-NTC Sensor (Standard: 2.252k NTC oder 10k NTC)
#define NTC_PULLUP_RESISTOR   2200.0f // Vorwiderstand in Ohm (Pull-up zu 3.3V)
#define NTC_NOMINAL_RESISTANCE 2252.0f // Widerstand bei 25°C
#define NTC_NOMINAL_TEMP      25.0f   // Nenntemperatur in °C
#define NTC_BETA_COEFFICIENT  3977.0f // Beta-Wert des Sensors (B25/85)

// Glättungsfaktor für analoge ADC-Messungen (Exponential Moving Average 0.0 - 1.0)
#define FILTER_ALPHA_BOOST    0.25f  // Höher = schneller, Niedriger = ruhiger
#define FILTER_ALPHA_TEMP     0.10f  // Stärkere Glättung für Temperatur

// ==============================================================================
// ALARMSCHWELLEN & WARNUNGEN
// ==============================================================================
#define ALARM_EGT_WARN        900.0f // Gelb/Orange Warnung ab 900°C
#define ALARM_EGT_CRITICAL    950.0f // Rot blinkend / Alarm ab 950°C
#define ALARM_BOOST_OVERBOOST 1.45f  // Overboost Warnung ab 1.45 bar Überdruck
#define ALARM_COOLANT_HIGH    105.0f // Kühlmittelwarnung ab 105°C
#define ALARM_IAT_HIGH        60.0f  // Ladelufttemperatur hoch ab 60°C

// ==============================================================================
// DISPLAY KONFIGURATION
// ==============================================================================
#define SCREEN_WIDTH          320
#define SCREEN_HEIGHT         240
#define DISPLAY_FPS_TARGET    20     // 20 Hz Display Refresh (alle 50ms)
#define SENSOR_SAMPLE_RATE_HZ 40     // 40 Hz Sensor Sampling (alle 25ms)

// UI Design Themes
enum DisplayTheme {
    THEME_CLASSIC_QUAD = 0,   // 4 Kacheln (Ladedruck, EGT, Ladeluft, Drehzahl/Wasser)
    THEME_SPORT_CLUSTER = 1,  // Rennsport Tacho mit Shift-Light & zentralem Ladedruck
    THEME_OEM_PLUS = 2,       // Golf 4 VDO Look (Tiefblau / Rot)
    THEME_DATA_MATRIX = 3,    // 8 Telemetrie-Werte für Diagnose / Logging
    THEME_COUNT = 4
};

// ==============================================================================
// GEMEINSAME DATENSTRUKTUR (SENSOR & TELEMETRIE)
// ==============================================================================
struct SensorData {
    // Externe analoge & SPI Sensoren
    float boostBar;           // Relativer Ladedruck in bar (-1.0 bis +2.5)
    float boostPeakBar;       // Höchster erreichter Ladedruck seit Start
    float exhaustTemp;        // Abgastemperatur EGT in °C (MAX31855)
    float exhaustTempPeak;    // Höchste EGT seit Start
    float chargeAirTemp;      // Ladelufttemperatur in °C (externer NTC)
    float rawMapVoltage;      // Gemessene Spannung am MAP-Sensor (zur Diagnose)

    // Aus dem VW Motronic M3.8.3 Steuergerät (KW1281 via K-Line)
    int   ecuRpm;             // Motordrehzahl (U/min)
    float ecuCoolantTemp;     // Kühlmitteltemperatur (°C)
    float ecuIntakeAirTemp;   // Ansauglufttemperatur ab MAF (°C)
    float ecuEngineLoadPct;   // Motorlast (%)
    float ecuMafGs;           // Luftmasse (g/s)
    float ecuInjectionMs;     // Einspritzzeit (ms)
    float ecuBatteryVolt;     // Bordspannung (V)
    float ecuIgnitionAngle;   // Zündwinkel (°KW)
    float ecuKnockRetardMax;  // Maximale Klopfrücknahme über Zyl. 1-4 (°KW)

    // Status & Diagnose
    bool  klineConnected;     // True wenn Verbindung zur ECU steht
    uint32_t klinePackets;    // Zähler für empfangene KW1281 Pakete
    bool  alarmEgt;           // EGT Alarmschwelle überschritten
    bool  alarmBoost;         // Overboost Alarm
    bool  alarmCoolant;       // Überhitzungs-Alarm
    bool  alarmChargeAir;     // Ladeluft zu heiß (Heat-Soak)

    // Aktuelles Display-Theme
    DisplayTheme currentTheme;
};

#endif // CONFIG_H
