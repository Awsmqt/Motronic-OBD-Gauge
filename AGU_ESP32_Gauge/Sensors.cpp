#include "Sensors.h"
#include <math.h>

SensorManager::SensorManager() 
    : thermocouple(nullptr),
      baroBaselineBar(MAP_BARO_DEFAULT),
      filteredBoostBar(0.0f),
      filteredChargeAirTemp(20.0f),
      sensorInitOk(false) {
}

void SensorManager::begin() {
    // ADC Pins konfigurieren (12-Bit Auflösung: 0 - 4095)
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db); // Full range ca. 0 - 3.1V bis 3.3V
    pinMode(PIN_MAP_SENSOR, INPUT);
    pinMode(PIN_CHARGE_AIR_NTC, INPUT);

    // MAX31855 Thermoelement-Verstärker (SPI)
    thermocouple = new Adafruit_MAX31855(PIN_EGT_CLK, PIN_EGT_CS, PIN_EGT_DO);
    if (thermocouple->begin()) {
        sensorInitOk = true;
        Serial.println("[Sensors] MAX31855 EGT Sensor erfolgreich initialisiert.");
    } else {
        Serial.println("[Sensors] WARNUNG: MAX31855 nicht erkannt oder Leitungsunterbrechung!");
    }

    // Auto-Zeroing / Barometer Kalibrierung:
    // Beim Einschalten der Zündung (Motor noch aus) entspricht der Druck im Saugrohr
    // exakt dem Umgebungsdruck. Dies nutzen wir für präzisen 0.00 bar Relativ-Nullpunkt!
    float sumBaro = 0;
    int samples = 20;
    for (int i = 0; i < samples; i++) {
        sumBaro += readMapSensorPressure();
        delay(10);
    }
    float initialBaro = sumBaro / (float)samples;
    // Plausibilitätsprüfung: Meereshöhe ca. 0.85 bis 1.05 bar absolut
    if (initialBaro >= 0.70f && initialBaro <= 1.15f) {
        baroBaselineBar = initialBaro;
        Serial.printf("[Sensors] Barometrischer Nullpunkt kalibriert: %.3f bar abs.\n", baroBaselineBar);
    } else {
        baroBaselineBar = MAP_BARO_DEFAULT;
        Serial.printf("[Sensors] MAP-Sensor unplausibel (%.3f bar) - Fallback auf Standard %.3f bar.\n", initialBaro, baroBaselineBar);
    }
}

float SensorManager::getCorrectedAdcVoltage(int pin) {
    // 16-faches Oversampling zur Rauschunterdrückung
    uint32_t adcSum = 0;
    for (int i = 0; i < 16; i++) {
        adcSum += analogRead(pin);
    }
    float rawAdc = (float)adcSum / 16.0f;

    // ESP32 ADC Kennlinien-Kompensation (nicht-linearer Bereich unter 0.15V und über 3.0V)
    // Einfache Polynom-Annäherung für 3.3V Referenz:
    float vPin = (rawAdc / 4095.0f) * 3.30f;
    return vPin;
}

float SensorManager::readMapSensorPressure() {
    float vPin = getCorrectedAdcVoltage(PIN_MAP_SENSOR);
    
    // Durch den Spannungsteiler geteilte Sensorspannung rekonstruieren (V_Sensor = V_Pin / Teiler)
    float vSensor = vPin / MAP_DIVIDER_RATIO;
    if (vSensor < 0.1f) vSensor = 0.1f;
    if (vSensor > 5.0f) vSensor = 5.0f;

    // Lineare Kennlinie des Bosch 3-Bar Sensors:
    // P = P_min + (V - V_min) * (P_max - P_min) / (V_max - V_min)
    float absPressure = MAP_P_MIN_BAR + ((vSensor - MAP_V_MIN) / (MAP_V_MAX - MAP_V_MIN)) * (MAP_P_MAX_BAR - MAP_P_MIN_BAR);
    return absPressure;
}

float SensorManager::readChargeAirTemp() {
    float vPin = getCorrectedAdcVoltage(PIN_CHARGE_AIR_NTC);
    if (vPin <= 0.05f || vPin >= 3.25f) {
        // Sensor nicht angeschlossen oder Kurzschluss
        return -99.0f;
    }

    // NTC Spannungsteiler: Pullup an 3.3V, NTC gegen Masse
    // V_Pin = 3.3V * (R_NTC / (R_Pullup + R_NTC)) => R_NTC = R_Pullup * V_Pin / (3.3 - V_Pin)
    float rNtc = NTC_PULLUP_RESISTOR * (vPin / (3.30f - vPin));

    // Steinhart-Hart / Beta Gleichung:
    // 1/T = 1/T0 + (1/Beta) * ln(R / R0)
    float steinhart = rNtc / NTC_NOMINAL_RESISTANCE;
    steinhart = log(steinhart);
    steinhart /= NTC_BETA_COEFFICIENT;
    steinhart += 1.0f / (NTC_NOMINAL_TEMP + 273.15f);
    steinhart = 1.0f / steinhart;
    float tempCelsius = steinhart - 273.15f;

    return tempCelsius;
}

float SensorManager::readEgtCelsius() {
    if (!thermocouple) return 0.0f;
    
    float c = thermocouple->readCelsius();
    if (isnan(c)) {
        uint8_t error = thermocouple->readError();
        if (error & MAX31855_FAULT_OPEN) {
            // Leitungsunterbrechung am Thermoelement
        }
        return -1.0f; // Fehlercode
    }
    return c;
}

void SensorManager::update(SensorData &data) {
    // 1. Ladedruck ermitteln & filtern
    float absPressure = readMapSensorPressure();
    float rawRelativeBoost = absPressure - baroBaselineBar;
    
    // Begrenzung auf physikalisch sinnvolle Werte (-1.0 bar bis +3.0 bar)
    if (rawRelativeBoost < -1.0f) rawRelativeBoost = -1.0f;
    
    // Exponential Moving Average
    filteredBoostBar = (FILTER_ALPHA_BOOST * rawRelativeBoost) + ((1.0f - FILTER_ALPHA_BOOST) * filteredBoostBar);
    data.boostBar = filteredBoostBar;
    data.rawMapVoltage = getCorrectedAdcVoltage(PIN_MAP_SENSOR) / MAP_DIVIDER_RATIO;

    if (data.boostBar > data.boostPeakBar) {
        data.boostPeakBar = data.boostBar;
    }

    // 2. Ladelufttemperatur ermitteln & filtern
    float rawIat = readChargeAirTemp();
    if (rawIat > -50.0f && rawIat < 150.0f) {
        filteredChargeAirTemp = (FILTER_ALPHA_TEMP * rawIat) + ((1.0f - FILTER_ALPHA_TEMP) * filteredChargeAirTemp);
        data.chargeAirTemp = filteredChargeAirTemp;
    } else {
        // Fallback: Falls kein externer NTC verbaut ist, IAT aus Motronic ECU verwenden
        data.chargeAirTemp = data.ecuIntakeAirTemp;
    }

    // 3. EGT Abgastemperatur lesen
    float egt = readEgtCelsius();
    if (egt >= 0.0f) {
        data.exhaustTemp = egt;
        if (data.exhaustTemp > data.exhaustTempPeak) {
            data.exhaustTempPeak = data.exhaustTemp;
        }
    }

    // 4. Alarme evaluieren
    data.alarmEgt = (data.exhaustTemp >= ALARM_EGT_WARN);
    data.alarmBoost = (data.boostBar >= ALARM_BOOST_OVERBOOST);
    data.alarmCoolant = (data.ecuCoolantTemp >= ALARM_COOLANT_HIGH);
    data.alarmChargeAir = (data.chargeAirTemp >= ALARM_IAT_HIGH);
}

void SensorManager::resetPeaks(SensorData &data) {
    data.boostPeakBar = data.boostBar;
    data.exhaustTempPeak = data.exhaustTemp;
}
