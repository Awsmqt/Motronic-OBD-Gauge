#include "Sensors.h"
#include <math.h>

SensorManager::SensorManager() 
    : thermocouple(nullptr),
      baroBaselineBar(MAP_BARO_DEFAULT),
      filteredBoostBar(0.0f),
      filteredOilPressureBar(0.0f),
      filteredChargeAirTemp(20.0f),
      filteredOilTemp(20.0f),
      sensorInitOk(false) {
}

void SensorManager::begin() {
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);

    // Alle 4 analogen Sensoren (ADC1 Pins 34, 35, 36, 39)
    pinMode(PIN_MAP_SENSOR, INPUT);
    pinMode(PIN_CHARGE_AIR_NTC, INPUT);
    pinMode(PIN_OIL_PRESSURE, INPUT);
    pinMode(PIN_OIL_TEMP, INPUT);

    // MAX31855 Thermoelement-Verstärker (SPI)
    thermocouple = new Adafruit_MAX31855(PIN_EGT_CLK, PIN_EGT_CS, PIN_EGT_DO);
    if (thermocouple->begin()) {
        sensorInitOk = true;
        Serial.println("[Sensors] MAX31855 EGT Sensor bereit.");
    } else {
        Serial.println("[Sensors] WARNUNG: MAX31855 nicht erkannt!");
    }

    // Auto-Zeroing / Barometer Kalibrierung für Ladedruck
    float sumBaro = 0;
    int samples = 20;
    for (int i = 0; i < samples; i++) {
        sumBaro += readMapSensorPressure();
        delay(10);
    }
    float initialBaro = sumBaro / (float)samples;
    if (initialBaro >= 0.70f && initialBaro <= 1.15f) {
        baroBaselineBar = initialBaro;
        Serial.printf("[Sensors] Baro Nullpunkt kalibriert: %.3f bar abs.\n", baroBaselineBar);
    } else {
        baroBaselineBar = MAP_BARO_DEFAULT;
    }
}

float SensorManager::getCorrectedAdcVoltage(int pin) {
    uint32_t adcSum = 0;
    for (int i = 0; i < 16; i++) {
        adcSum += analogRead(pin);
    }
    float rawAdc = (float)adcSum / 16.0f;
    return (rawAdc / 4095.0f) * 3.30f;
}

float SensorManager::readMapSensorPressure() {
    float vPin = getCorrectedAdcVoltage(PIN_MAP_SENSOR);
    float vSensor = vPin / MAP_DIVIDER_RATIO;
    if (vSensor < 0.1f) vSensor = 0.1f;
    if (vSensor > 5.0f) vSensor = 5.0f;

    // Lineare Kennlinie Bosch 3-Bar:
    float absPressure = MAP_P_MIN_BAR + ((vSensor - MAP_V_MIN) / (MAP_V_MAX - MAP_V_MIN)) * (MAP_P_MAX_BAR - MAP_P_MIN_BAR);
    return absPressure;
}

float SensorManager::readOilPressure() {
    float vPin = getCorrectedAdcVoltage(PIN_OIL_PRESSURE);
    float vSensor = vPin / OIL_PRESS_DIVIDER;
    if (vSensor < 0.1f) vSensor = 0.1f;
    if (vSensor > 5.0f) vSensor = 5.0f;

    // Lineare Kennlinie 0-10 bar Geber (0.5V = 0 bar, 4.5V = 10 bar)
    float press = ((vSensor - OIL_PRESS_V_MIN) / (OIL_PRESS_V_MAX - OIL_PRESS_V_MIN)) * OIL_PRESS_MAX_BAR;
    if (press < 0.0f) press = 0.0f;
    if (press > 10.0f) press = 10.0f;
    return press;
}

float SensorManager::readNtcTemperature(int pin) {
    float vPin = getCorrectedAdcVoltage(pin);
    if (vPin <= 0.05f || vPin >= 3.25f) {
        return -99.0f; // Sensor nicht verbunden
    }

    // NTC Widerstand aus Spannungsteiler
    float rNtc = NTC_PULLUP_RESISTOR * (vPin / (3.30f - vPin));

    // Steinhart-Hart / Beta Gleichung
    float steinhart = rNtc / NTC_NOMINAL_RESISTANCE;
    steinhart = log(steinhart);
    steinhart /= NTC_BETA_COEFFICIENT;
    steinhart += 1.0f / (NTC_NOMINAL_TEMP + 273.15f);
    steinhart = 1.0f / steinhart;
    return (steinhart - 273.15f);
}

float SensorManager::readEgtCelsius() {
    if (!thermocouple) return 0.0f;
    float c = thermocouple->readCelsius();
    return isnan(c) ? -1.0f : c;
}

void SensorManager::update(SensorData &data) {
    // 1. DRÜCKE (Ladedruck & Öldruck)
    float absPressure = readMapSensorPressure();
    float rawRelativeBoost = absPressure - baroBaselineBar;
    if (rawRelativeBoost < -1.0f) rawRelativeBoost = -1.0f;

    filteredBoostBar = (FILTER_ALPHA_PRESS * rawRelativeBoost) + ((1.0f - FILTER_ALPHA_PRESS) * filteredBoostBar);
    data.boostBar = filteredBoostBar;
    if (data.boostBar > data.boostPeakBar) {
        data.boostPeakBar = data.boostBar;
    }

    float rawOilPress = readOilPressure();
    filteredOilPressureBar = (FILTER_ALPHA_PRESS * rawOilPress) + ((1.0f - FILTER_ALPHA_PRESS) * filteredOilPressureBar);
    data.oilPressureBar = filteredOilPressureBar;

    // Minimaler Öldruck bei warmem Motor erfassen
    if (data.oilTemp > 80.0f) {
        if (data.oilPressureMinBar <= 0.0f || data.oilPressureBar < data.oilPressureMinBar) {
            data.oilPressureMinBar = data.oilPressureBar;
        }
    }

    // 2. TEMPERATUREN
    // Ladeluft nach LLK
    float rawIat = readNtcTemperature(PIN_CHARGE_AIR_NTC);
    if (rawIat > -50.0f && rawIat < 150.0f) {
        filteredChargeAirTemp = (FILTER_ALPHA_TEMP * rawIat) + ((1.0f - FILTER_ALPHA_TEMP) * filteredChargeAirTemp);
        data.chargeAirTemp = filteredChargeAirTemp;
    } else {
        // Fallback: OBD Ansaugluft
        data.chargeAirTemp = data.ecuIntakeAirTemp;
    }

    // Öltemperatur
    float rawOilTemp = readNtcTemperature(PIN_OIL_TEMP);
    if (rawOilTemp > -40.0f && rawOilTemp < 180.0f) {
        filteredOilTemp = (FILTER_ALPHA_TEMP * rawOilTemp) + ((1.0f - FILTER_ALPHA_TEMP) * filteredOilTemp);
        data.oilTemp = filteredOilTemp;
        if (data.oilTemp > data.oilTempPeak) {
            data.oilTempPeak = data.oilTemp;
        }
    }

    // EGT
    float egt = readEgtCelsius();
    if (egt >= 0.0f) {
        data.exhaustTemp = egt;
        if (data.exhaustTemp > data.exhaustTempPeak) {
            data.exhaustTempPeak = data.exhaustTemp;
        }
    }

    // 3. ALARME PRÜFEN
    data.alarmOilPressure = (data.oilPressureBar < ALARM_OIL_PRESS_MIN_WARN);
    data.alarmOilTemp = (data.oilTemp >= ALARM_OIL_TEMP_WARN);
    data.alarmCoolant = (data.coolantTemp >= ALARM_COOLANT_HIGH);
    data.alarmChargeAir = (data.chargeAirTemp >= ALARM_IAT_HIGH);
    data.alarmEgt = (data.exhaustTemp >= ALARM_EGT_WARN);
    data.alarmBoost = (data.boostBar >= ALARM_BOOST_OVERBOOST);
    data.alarmBattery = (data.batteryVolt < ALARM_BATTERY_LOW || data.batteryVolt > ALARM_BATTERY_HIGH);
}

void SensorManager::resetPeaks(SensorData &data) {
    data.boostPeakBar = data.boostBar;
    data.exhaustTempPeak = data.exhaustTemp;
    data.oilTempPeak = data.oilTemp;
    data.oilPressureMinBar = data.oilPressureBar;
}
