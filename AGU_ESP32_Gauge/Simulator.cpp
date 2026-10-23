#include "Simulator.h"

MotorSimulator::MotorSimulator()
    : currentPhase(SIM_IDLE),
      phaseStartTime(0),
      phaseDuration(4000),
      currentRpm(840.0f),
      currentBoost(-0.68f),
      currentEgt(430.0f),
      currentIat(24.0f),
      currentCoolant(88.0f) {
}

void MotorSimulator::begin() {
    phaseStartTime = millis();
    currentPhase = SIM_IDLE;
}

void MotorSimulator::advancePhase() {
    switch (currentPhase) {
        case SIM_IDLE:
            currentPhase = SIM_CRUISING;
            phaseDuration = 4000;
            break;
        case SIM_CRUISING:
            currentPhase = SIM_BOOST_RAMP;
            phaseDuration = 3000;
            break;
        case SIM_BOOST_RAMP:
            currentPhase = SIM_FULL_PULL_WOT;
            phaseDuration = 4500;
            break;
        case SIM_FULL_PULL_WOT:
            currentPhase = SIM_GEAR_SHIFT;
            phaseDuration = 700;
            break;
        case SIM_GEAR_SHIFT:
            currentPhase = SIM_OVERRUN;
            phaseDuration = 3500;
            break;
        case SIM_OVERRUN:
            currentPhase = SIM_IDLE;
            phaseDuration = 3000;
            break;
    }
    phaseStartTime = millis();
}

void MotorSimulator::update(SensorData &data) {
    unsigned long now = millis();
    unsigned long elapsed = now - phaseStartTime;

    if (elapsed >= phaseDuration) {
        advancePhase();
        elapsed = 0;
    }

    float progress = (float)elapsed / (float)phaseDuration;
    if (progress > 1.0f) progress = 1.0f;

    switch (currentPhase) {
        case SIM_IDLE:
            // Standgas: 840 RPM, -0.68 bar Unterdruck, EGT kühlt auf 420°C ab
            currentRpm = 840.0f + random(-20, 20);
            currentBoost = -0.68f + ((float)random(-2, 2) / 100.0f);
            currentEgt = currentEgt + (420.0f - currentEgt) * 0.05f;
            currentIat = currentIat + (25.0f - currentIat) * 0.02f;
            data.ecuEngineLoadPct = 16.5f;
            data.ecuMafGs = 2.8f;
            data.ecuInjectionMs = 1.8f;
            data.ecuIgnitionAngle = 9.0f;
            data.ecuKnockRetardMax = 0.0f;
            break;

        case SIM_CRUISING:
            // Teillast Stadt/Landstraße: 2200 RPM, leichtes Cruisen (-0.35 bar)
            currentRpm = 1800.0f + progress * 700.0f;
            currentBoost = -0.45f + progress * 0.25f;
            currentEgt = currentEgt + (580.0f - currentEgt) * 0.04f;
            currentIat = currentIat + (28.0f - currentIat) * 0.02f;
            data.ecuEngineLoadPct = 35.0f;
            data.ecuMafGs = 22.0f;
            data.ecuInjectionMs = 3.6f;
            data.ecuIgnitionAngle = 24.0f;
            data.ecuKnockRetardMax = 0.0f;
            break;

        case SIM_BOOST_RAMP:
            // Ladedruckaufbau: K03/K04 Lader spult hoch!
            currentRpm = 2500.0f + progress * 2000.0f; // 2500 -> 4500 RPM
            currentBoost = -0.1f + progress * 1.35f;   // Bis 1.25 bar
            currentEgt = currentEgt + (780.0f - currentEgt) * 0.06f;
            currentIat = currentIat + (38.0f - currentIat) * 0.03f;
            data.ecuEngineLoadPct = 85.0f + progress * 40.0f;
            data.ecuMafGs = 50.0f + progress * 70.0f;
            data.ecuInjectionMs = 8.5f + progress * 6.0f;
            data.ecuIgnitionAngle = 16.0f;
            data.ecuKnockRetardMax = 0.75f;
            break;

        case SIM_FULL_PULL_WOT:
            // Volllast (Wide Open Throttle): 4500 -> 6500 RPM, Overboost 1.38 bar, EGT klettert auf 920°C!
            currentRpm = 4500.0f + progress * 2000.0f;
            currentBoost = 1.38f - progress * 0.18f; // Haltedruck fällt bei hoher Drehzahl leicht ab
            currentEgt = currentEgt + (925.0f - currentEgt) * 0.05f; // Erreicht Alarmzone!
            currentIat = currentIat + (52.0f - currentIat) * 0.04f;  // Ladeluft erwärmt sich
            data.ecuEngineLoadPct = 145.0f;
            data.ecuMafGs = 120.0f + progress * 55.0f; // Bis 175 g/s
            data.ecuInjectionMs = 17.5f;
            data.ecuIgnitionAngle = 18.0f - progress * 3.0f;
            data.ecuKnockRetardMax = (currentEgt > 900.0f) ? 2.25f : 0.75f;
            break;

        case SIM_GEAR_SHIFT:
            // Gangwechsel: Drosselklappe schließt, Blow-Off öffnet!
            currentRpm = 6500.0f - progress * 2200.0f; // Fällt auf 4300
            currentBoost = -0.30f;                      // Ladedruck bricht schlagartig ein
            data.ecuEngineLoadPct = 20.0f;
            data.ecuMafGs = 10.0f;
            data.ecuInjectionMs = 0.0f;
            break;

        case SIM_OVERRUN:
            // Schubabschaltung (Motorbremse): Einspritzung 0ms, starker Unterdruck (-0.80 bar)
            currentRpm = 4300.0f - progress * 2500.0f; // Abtouren
            currentBoost = -0.78f + ((float)random(-1, 1) / 100.0f);
            currentEgt = currentEgt + (480.0f - currentEgt) * 0.03f;
            currentIat = currentIat + (27.0f - currentIat) * 0.02f;
            data.ecuEngineLoadPct = 12.0f;
            data.ecuMafGs = 3.5f;
            data.ecuInjectionMs = 0.0f;
            data.ecuIgnitionAngle = 5.0f;
            data.ecuKnockRetardMax = 0.0f;
            break;
    }

    // Werte in SensorData übertragen
    data.boostBar = currentBoost;
    if (data.boostBar > data.boostPeakBar) data.boostPeakBar = data.boostBar;

    data.exhaustTemp = currentEgt;
    if (data.exhaustTemp > data.exhaustTempPeak) data.exhaustTempPeak = data.exhaustTemp;

    data.chargeAirTemp = currentIat;
    data.ecuRpm = (int)currentRpm;
    data.ecuCoolantTemp = 90.0f + ((float)random(-5, 5) / 10.0f);
    data.ecuIntakeAirTemp = currentIat - 3.0f;
    data.ecuBatteryVolt = 13.8f + ((float)random(-1, 1) / 10.0f);
    data.klineConnected = true;
    data.klinePackets++;

    // Alarme evaluieren
    data.alarmEgt = (data.exhaustTemp >= ALARM_EGT_WARN);
    data.alarmBoost = (data.boostBar >= ALARM_BOOST_OVERBOOST);
    data.alarmCoolant = (data.ecuCoolantTemp >= ALARM_COOLANT_HIGH);
    data.alarmChargeAir = (data.chargeAirTemp >= ALARM_IAT_HIGH);
}
