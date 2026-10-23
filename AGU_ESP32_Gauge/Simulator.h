#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <Arduino.h>
#include "Config.h"

enum SimPhase {
    SIM_IDLE = 0,
    SIM_CRUISING,
    SIM_BOOST_RAMP,
    SIM_FULL_PULL_WOT,
    SIM_GEAR_SHIFT,
    SIM_OVERRUN
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
    float currentRpm;
    float currentBoost;
    float currentEgt;
    float currentIat;
    float currentCoolant;

    void advancePhase();
};

#endif // SIMULATOR_H
