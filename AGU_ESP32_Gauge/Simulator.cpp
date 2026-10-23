#include "Simulator.h"

MotorSimulator::MotorSimulator()
    : currentPhase(SIM_COLD_IDLE),
      phaseStartTime(0),
      phaseDuration(4500),
      currentBoost(-0.68f),
      currentOilPress(4.5f),
      currentOilTemp(45.0f),
      currentCoolant(70.0f),
      currentChargeAir(22.0f),
      currentEgt(420.0f),
      currentBattery(14.1f),
      currentLambda(1.00f),
      currentInjectionMs(3.5f),
      currentIgnitionTiming(12.0f),
      currentKnockRetard(0.0f),
      currentThrottlePct(3.0f) {
}

void MotorSimulator::begin() {
    phaseStartTime = millis();
    currentPhase = SIM_COLD_IDLE;
}

void MotorSimulator::advancePhase() {
    switch (currentPhase) {
        case SIM_COLD_IDLE:
            currentPhase = SIM_WARM_CRUISING;
            phaseDuration = 5000;
            break;
        case SIM_WARM_CRUISING:
            currentPhase = SIM_BOOST_PULL;
            phaseDuration = 4000;
            break;
        case SIM_BOOST_PULL:
            currentPhase = SIM_HARD_TRACK_PULL;
            phaseDuration = 4500;
            break;
        case SIM_HARD_TRACK_PULL:
            currentPhase = SIM_HOT_IDLE;
            phaseDuration = 4000;
            break;
        case SIM_HOT_IDLE:
            currentPhase = SIM_WARM_CRUISING;
            phaseDuration = 4500;
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
        case SIM_COLD_IDLE:
            // Kaltlauf: Öldruck noch hoch (kaltes Öl ist dickflüssig ~4.2-4.5 bar)
            currentBoost = -0.65f + ((float)random(-1, 1) / 100.0f);
            currentOilPress = 4.2f + ((float)random(-5, 5) / 100.0f);
            currentOilTemp = currentOilTemp + (75.0f - currentOilTemp) * 0.02f;
            currentCoolant = currentCoolant + (88.0f - currentCoolant) * 0.03f;
            currentChargeAir = 22.0f;
            currentEgt = 430.0f;
            currentBattery = 14.2f;
            // Erweiterte ECU-Werte (Kaltlauf)
            currentLambda = 0.98f + ((float)random(-2, 2) / 100.0f);   // Fett im Kaltlauf
            currentInjectionMs = 5.5f + ((float)random(-3, 3) / 10.0f);
            currentIgnitionTiming = 8.0f;
            currentKnockRetard = 0.0f;
            currentThrottlePct = 3.0f + ((float)random(-5, 5) / 10.0f);
            break;

        case SIM_WARM_CRUISING:
            // Warmgefahrener Motor im Teillastbetrieb
            currentBoost = -0.30f + progress * 0.2f;
            currentOilPress = 3.6f + progress * 0.4f; // 3.6 -> 4.0 bar bei Drehzahl
            currentOilTemp = currentOilTemp + (92.0f - currentOilTemp) * 0.04f;
            currentCoolant = 90.0f + ((float)random(-3, 3) / 10.0f);
            currentChargeAir = 26.0f + progress * 4.0f;
            currentEgt = 560.0f + progress * 100.0f;
            currentBattery = 13.9f;
            // Erweiterte ECU-Werte (Teillast)
            currentLambda = 1.00f + ((float)random(-1, 1) / 100.0f);
            currentInjectionMs = 4.0f + progress * 2.0f;
            currentIgnitionTiming = 12.0f + progress * 3.0f;
            currentKnockRetard = 0.0f;
            currentThrottlePct = 15.0f + progress * 25.0f;
            break;

        case SIM_BOOST_PULL:
            // Beschleunigung mit Ladedruck
            currentBoost = 0.2f + progress * 1.05f; // Bis 1.25 bar
            currentOilPress = 4.5f + progress * 0.4f; // ca. 4.8 bar Öldruck bei hoher Drehzahl
            currentOilTemp = currentOilTemp + (102.0f - currentOilTemp) * 0.05f;
            currentCoolant = 91.0f + progress * 2.0f;
            currentChargeAir = 30.0f + progress * 12.0f; // LLK erwärmt sich auf 42°C
            currentEgt = 680.0f + progress * 180.0f;     // EGT steigt auf 860°C
            currentBattery = 13.8f;
            // Erweiterte ECU-Werte (Volllast mit Ladedruck)
            currentLambda = 0.88f + ((float)random(-2, 2) / 100.0f);   // Angefettet unter Last
            currentInjectionMs = 8.0f + progress * 4.0f;                // Bis 12ms
            currentIgnitionTiming = 15.0f - progress * 6.0f;            // Zündung zurücknehmen
            currentKnockRetard = progress * 2.5f;                       // Leichtes Klopfen
            currentThrottlePct = 60.0f + progress * 40.0f;              // Bis 100%
            break;

        case SIM_HARD_TRACK_PULL:
            // Volllast / Autobahn-Sprint: Hohe Belastung für Öl, Wasser und Ladeluft
            currentBoost = 1.35f - progress * 0.10f;
            currentOilPress = 4.7f;
            currentOilTemp = currentOilTemp + (116.0f - currentOilTemp) * 0.05f; // Öl wird heiß (112°C+)
            currentCoolant = 93.0f + progress * 3.0f;
            currentChargeAir = 42.0f + progress * 14.0f; // Ladeluft klettert auf 56°C (Heat-Soak)
            currentEgt = 860.0f + progress * 65.0f;      // EGT klettert auf 925°C (Warnschwelle!)
            currentBattery = 13.7f;
            // Erweiterte ECU-Werte (Dauervollgas)
            currentLambda = 0.86f + ((float)random(-1, 2) / 100.0f);
            currentInjectionMs = 11.0f + progress * 1.0f;
            currentIgnitionTiming = 9.0f - progress * 3.0f;             // Zündung weit zurück
            currentKnockRetard = 1.5f + progress * 1.5f;                // Bis 3° Rücknahme
            currentThrottlePct = 100.0f;
            break;

        case SIM_HOT_IDLE:
            // Ampelstopp nach harter Fahrt (Öl heiß & dünnflüssig -> Öldruck fällt auf 1.5 bar)
            currentBoost = -0.72f;
            currentOilPress = 1.55f + ((float)random(-3, 3) / 100.0f); // Warmer Leerlauf-Öldruck
            currentOilTemp = currentOilTemp + (98.0f - currentOilTemp) * 0.03f;
            currentCoolant = 94.0f; // Lüfter läuft an
            currentChargeAir = currentChargeAir + (35.0f - currentChargeAir) * 0.03f;
            currentEgt = currentEgt + (490.0f - currentEgt) * 0.05f;
            currentBattery = 13.6f;
            // Erweiterte ECU-Werte (Heißer Leerlauf)
            currentLambda = 1.00f + ((float)random(-1, 1) / 100.0f);
            currentInjectionMs = 3.8f + ((float)random(-2, 2) / 10.0f);
            currentIgnitionTiming = 10.0f;
            currentKnockRetard = 0.0f;
            currentThrottlePct = 2.5f + ((float)random(-5, 5) / 10.0f);
            break;
    }

    // Werte in SensorData übertragen
    data.boostBar = currentBoost;
    if (data.boostBar > data.boostPeakBar) data.boostPeakBar = data.boostBar;

    data.oilPressureBar = currentOilPress;
    if (data.oilTemp > 80.0f) {
        if (data.oilPressureMinBar <= 0.0f || data.oilPressureBar < data.oilPressureMinBar) {
            data.oilPressureMinBar = data.oilPressureBar;
        }
    }

    data.oilTemp = currentOilTemp;
    if (data.oilTemp > data.oilTempPeak) data.oilTempPeak = data.oilTemp;

    data.coolantTemp = currentCoolant;
    data.chargeAirTemp = currentChargeAir;
    data.exhaustTemp = currentEgt;
    if (data.exhaustTemp > data.exhaustTempPeak) data.exhaustTempPeak = data.exhaustTemp;

    data.batteryVolt = currentBattery;
    data.ecuRpm = (currentBoost > 0.0f) ? 4500 : 880;
    data.klineConnected = true;
    data.klinePackets++;

    // Erweiterte ECU-Simulationswerte übertragen
    data.ecuLambda = currentLambda;
    data.ecuInjectionMs = currentInjectionMs;
    data.ecuIgnitionTiming = currentIgnitionTiming;
    data.ecuKnockRetard = currentKnockRetard;
    data.ecuThrottlePct = currentThrottlePct;
    data.ecuStatusBits = (currentBoost > 0.0f) ? 0x03 : 0x01; // Bit 0: Motorlauf, Bit 1: Volllast

    // Simulierte DTCs: In BOOST_PULL und HARD_TRACK_PULL Phasen 2 Test-DTCs setzen
    if (currentPhase == SIM_BOOST_PULL || currentPhase == SIM_HARD_TRACK_PULL) {
        data.dtcCount = 2;
        data.dtcCodes[0]  = 0x4444; // P1092 - Simulierter Ladedruck-Fehler
        data.dtcStatus[0] = 0x20;   // Status: sporadisch
        data.dtcCodes[1]  = 0x0565; // P0565 - Simulierter Geschwindigkeitssignal-Fehler
        data.dtcStatus[1] = 0x60;   // Status: bestätigt
        data.milSeen = true;
    } else {
        data.dtcCount = 0;
        data.milSeen = false;
    }

    // Alarme evaluieren
    data.alarmOilPressure = (data.oilPressureBar < ALARM_OIL_PRESS_MIN_WARN);
    data.alarmOilTemp = (data.oilTemp >= ALARM_OIL_TEMP_WARN);
    data.alarmCoolant = (data.coolantTemp >= ALARM_COOLANT_HIGH);
    data.alarmChargeAir = (data.chargeAirTemp >= ALARM_IAT_HIGH);
    data.alarmEgt = (data.exhaustTemp >= ALARM_EGT_WARN);
    data.alarmBoost = (data.boostBar >= ALARM_BOOST_OVERBOOST);
    data.alarmBattery = (data.batteryVolt < ALARM_BATTERY_LOW || data.batteryVolt > ALARM_BATTERY_HIGH);
}
