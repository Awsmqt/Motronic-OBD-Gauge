#ifndef SENSORS_H
#define SENSORS_H

#include "Config.h"
#include <Adafruit_MAX31855.h>

class SensorManager {
public:
    SensorManager();
    void begin();
    void update(SensorData &data);
    void resetPeaks(SensorData &data);

private:
    Adafruit_MAX31855* thermocouple;
    float baroBaselineBar;
    float filteredBoostBar;
    float filteredOilPressureBar;
    float filteredChargeAirTemp;
    float filteredOilTemp;

    float readMapSensorPressure();
    float readOilPressure();
    float readNtcTemperature(int pin);
    float readEgtCelsius();
    float getCorrectedAdcVoltage(int pin);
};

#endif // SENSORS_H
