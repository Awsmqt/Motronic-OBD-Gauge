#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <Arduino.h>
#include "Config.h"

enum SimPhase {
    SIM_COLD_IDLE = 0,
    SIM_WARM_CRUISING,
    SIM_BOOST_PULL,
    SIM_HARD_TRACK_PULL,
    SIM_HOT_IDLE
};

class MotorSimulator {
public:
    MotorSimulator();
    void begin();
    void update(SensorData &data);

private:
    SimPhase currentPhase;
    unsigned long phaseStartTime;
    unsigned long phaseDuration;

    float currentBoost;
    float currentOilPress;
    float currentOilTemp;
    float currentCoolant;
    float currentChargeAir;
    float currentEgt;
    float currentBattery;

    void advancePhase();
};

#endif // SIMULATOR_H
