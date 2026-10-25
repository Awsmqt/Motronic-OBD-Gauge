#include "DisplayUI.h"

DisplayUI::DisplayUI()
    : lastRenderedTheme(THEME_COUNT),
      touchPressStart(0),
      wasTouched(false) {
}

void DisplayUI::begin() {
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(COLOR_BG);
}

void DisplayUI::nextTheme(SensorData &data) {
    data.currentTheme = data.currentTheme == THEME_MEASUREMENTS ? THEME_HEALTH : THEME_MEASUREMENTS;
    lastRenderedTheme = THEME_COUNT;
}

void DisplayUI::checkTouch(SensorData &data) {
    uint16_t touchX = 0;
    uint16_t touchY = 0;
    const bool isTouched = tft.getTouch(&touchX, &touchY);

    if (isTouched && !wasTouched) {
        wasTouched = true;
        touchPressStart = millis();
        return;
    }

    if (!isTouched && wasTouched) {
        const unsigned long pressDuration = millis() - touchPressStart;
        if (pressDuration < 1000) {
            nextTheme(data);
        } else if (pressDuration >= 1500) {
            data.boostPeakBar = data.boostBar;
            data.exhaustTempPeak = data.exhaustTemp;
            data.oilTempPeak = data.oilTemp;
            data.oilPressureMinBar = data.oilPressureBar;
            lastRenderedTheme = THEME_COUNT;
        }
        wasTouched = false;
    }
}

void DisplayUI::update(SensorData &data) {
    if (data.currentTheme >= THEME_COUNT) {
        data.currentTheme = THEME_MEASUREMENTS;
    }

    if (data.currentTheme != lastRenderedTheme) {
        lastRenderedTheme = data.currentTheme;
        tft.fillScreen(COLOR_BG);
        tft.drawFastHLine(0, 25, 320, COLOR_LINE);
        tft.drawFastHLine(0, 208, 320, COLOR_LINE);
        tft.drawFastVLine(160, 26, 182, COLOR_LINE);
    }

    if (data.currentTheme == THEME_HEALTH) {
        renderHealth(data);
    } else {
        renderMeasurements(data);
    }
}

void DisplayUI::renderMeasurements(const SensorData &data) {
    struct Tile {
        const char *label;
        float value;
        const char *unit;
        uint16_t color;
    };

    const Tile tiles[] = {
        { "LADEDRUCK", data.boostBar, "bar", data.alarmBoost ? TFT_RED : COLOR_CYAN },
        { "OELDRUCK", data.oilPressureBar, "bar", data.alarmOilPressure ? TFT_RED : COLOR_GREEN },
        { "OELTEMP", data.oilTemp, "C", data.alarmOilTemp ? TFT_RED : (data.oilTemp < ALARM_OIL_TEMP_COLD ? COLOR_BLUE : COLOR_GREEN) },
        { "KUEHLWASSER", data.coolantTemp, "C", data.alarmCoolant ? TFT_RED : COLOR_CYAN },
        { "LADELUFT", data.chargeAirTemp, "C", data.alarmChargeAir ? TFT_RED : COLOR_GREEN },
        { "ABGAS EGT", data.exhaustTemp, "C", data.alarmEgt ? TFT_RED : COLOR_YELLOW },
        { "LAMBDA", data.ecuLambda, "", COLOR_CYAN },
        { "DREHZAHL", (float)data.ecuRpm, "rpm", COLOR_GREEN }
    };

    for (uint8_t i = 0; i < sizeof(tiles) / sizeof(tiles[0]); ++i) {
        const int x = (i % 2) * 160;
        const int y = 26 + (i / 2) * 45;
        char value[16];
        if (i == 6 && !data.klineConnected) {
            snprintf(value, sizeof(value), "--");
        } else if (i == 7 && !data.klineConnected) {
            snprintf(value, sizeof(value), "--");
        } else if (i == 6) {
            snprintf(value, sizeof(value), "%.3f", tiles[i].value);
        } else if (i == 7) {
            snprintf(value, sizeof(value), "%d", (int)tiles[i].value);
        } else if (i == 0) {
            snprintf(value, sizeof(value), "%.2f", tiles[i].value);
        } else {
            snprintf(value, sizeof(value), "%.1f", tiles[i].value);
        }

        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_LIGHTGREY, COLOR_BG);
        tft.drawString(tiles[i].label, x + 8, y + 2, 1);
        tft.setTextDatum(BL_DATUM);
        tft.setTextColor(tiles[i].color, COLOR_BG);
        tft.drawString(value, x + 8, y + 40, 4);
        tft.setTextDatum(BR_DATUM);
        tft.setTextColor(TFT_DARKGREY, COLOR_BG);
        tft.drawString(tiles[i].unit, x + 151, y + 37, 1);
    }

    char footer[32];
    snprintf(footer, sizeof(footer), "%.1f V   %s",
             data.batteryVolt, data.klineConnected ? "ECU OK" : "ECU OFFLINE");
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(data.alarmBattery ? TFT_RED : TFT_LIGHTGREY, COLOR_BG);
    tft.drawString(footer, 160, 224, 2);
}

void DisplayUI::renderHealth(const SensorData &data) {
    const bool critical =
        data.oilPressureBar < ALARM_OIL_PRESS_CRITICAL ||
        data.oilTemp >= ALARM_OIL_TEMP_CRITICAL ||
        data.coolantTemp >= ALARM_COOLANT_CRITICAL ||
        data.exhaustTemp >= ALARM_EGT_CRITICAL;
    const bool warning =
        data.alarmOilPressure || data.alarmOilTemp || data.alarmCoolant ||
        data.alarmChargeAir || data.alarmEgt || data.alarmBoost || data.alarmBattery;
    const uint16_t overallColor = healthColor(warning, critical);

    tft.fillRect(0, 0, 320, 25, COLOR_BG);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, COLOR_BG);
    tft.drawString("MOTORZUSTAND", 8, 7, 2);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(overallColor, COLOR_BG);
    tft.drawString(critical ? "KRITISCH" : (warning ? "PRUEFEN" : "OK"), 312, 7, 2);

    struct Row {
        const char *label;
        float value;
        const char *unit;
        float minValue;
        float maxValue;
        bool warning;
        bool critical;
    };
    const Row rows[] = {
        { "OELDRUCK", data.oilPressureBar, "bar", 0.0f, 7.0f,
          data.alarmOilPressure, data.oilPressureBar < ALARM_OIL_PRESS_CRITICAL },
        { "OELTEMP", data.oilTemp, "C", 40.0f, 140.0f,
          data.alarmOilTemp, data.oilTemp >= ALARM_OIL_TEMP_CRITICAL },
        { "WASSER", data.coolantTemp, "C", 40.0f, 120.0f,
          data.alarmCoolant, data.coolantTemp >= ALARM_COOLANT_CRITICAL },
        { "LADELUFT", data.chargeAirTemp, "C", 10.0f, 70.0f,
          data.alarmChargeAir, false },
        { "ABGAS", data.exhaustTemp, "C", 300.0f, 1050.0f,
          data.alarmEgt, data.exhaustTemp >= ALARM_EGT_CRITICAL },
        { "BOOST", data.boostBar, "bar", -1.0f, 2.0f,
          data.alarmBoost, false }
    };

    for (uint8_t i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        const int y = 31 + i * 28;
        const uint16_t color = i == 1 && data.oilTemp < ALARM_OIL_TEMP_COLD
            ? COLOR_BLUE
            : healthColor(rows[i].warning, rows[i].critical);
        char value[20];
        snprintf(value, sizeof(value), "%s %.1f %s",
                 rows[i].label, rows[i].value, rows[i].unit);

        tft.fillRect(0, y, 320, 27, COLOR_BG);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(color, COLOR_BG);
        tft.drawString(value, 8, y + 2, 2);
        drawBar(8, y + 20, 304, rows[i].value, rows[i].minValue, rows[i].maxValue, color);
    }

    char footer[40];
    snprintf(footer, sizeof(footer), "%.1f V  |  %s",
             data.batteryVolt, data.klineConnected ? "ECU VERBUNDEN" : "ECU OFFLINE");
    tft.fillRect(0, 208, 320, 32, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(data.alarmBattery ? TFT_RED : TFT_LIGHTGREY, COLOR_BG);
    tft.drawString(footer, 160, 224, 2);
}

void DisplayUI::drawBar(int x, int y, int w, float value, float minValue, float maxValue, uint16_t color) {
    tft.drawRect(x, y, w, 7, COLOR_LINE);
    float fraction = (value - minValue) / (maxValue - minValue);
    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 1.0f) fraction = 1.0f;
    const int fillWidth = (int)(fraction * (w - 4));
    tft.fillRect(x + 2, y + 2, fillWidth, 3, color);
    tft.fillRect(x + 2 + fillWidth, y + 2, (w - 4) - fillWidth, 3, COLOR_BG);
}

uint16_t DisplayUI::healthColor(bool warning, bool critical) const {
    if (critical) return TFT_RED;
    if (warning) return COLOR_YELLOW;
    return COLOR_GREEN;
}
