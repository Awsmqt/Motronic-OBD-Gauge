#include "DisplayUI.h"
#include <math.h>

DisplayUI::DisplayUI()
    : lastRenderedTheme(THEME_COUNT),
      touchPressStart(0),
      wasTouched(false) {
}

void DisplayUI::begin() {
    tft.init();
    tft.setRotation(1); // Querformat (320x240)
    tft.fillScreen(TFT_BLACK);
}

void DisplayUI::nextTheme(SensorData &data) {
    int next = (int)data.currentTheme + 1;
    if (next >= THEME_COUNT) next = 0;
    data.currentTheme = (DisplayTheme)next;
    tft.fillScreen(TFT_BLACK);
}

void DisplayUI::checkTouch(SensorData &data) {
    uint16_t touchX = 0, touchY = 0;
    bool isTouched = tft.getTouch(&touchX, &touchY);

    if (isTouched) {
        if (!wasTouched) {
            wasTouched = true;
            touchPressStart = millis();
        } else {
            // Langes Drücken (> 1.5 Sekunden) setzt Peak-Werte zurück
            if (millis() - touchPressStart > 1500) {
                data.boostPeakBar = data.boostBar;
                data.exhaustTempPeak = data.exhaustTemp;
                tft.fillScreen(TFT_BLACK);
                touchPressStart = millis();
            }
        }
    } else {
        if (wasTouched) {
            // Kurzer Tipp -> Theme wechseln
            if (millis() - touchPressStart < 1000) {
                nextTheme(data);
            }
            wasTouched = false;
        }
    }
}

void DisplayUI::update(const SensorData &data) {
    // Falls Theme gewechselt wurde, Bildschirmhintergrund neu zeichnen
    if (data.currentTheme != lastRenderedTheme) {
        lastRenderedTheme = data.currentTheme;
        initTheme(data.currentTheme);
    }

    switch (data.currentTheme) {
        case THEME_CLASSIC_QUAD:
            renderClassicQuad(data);
            break;
        case THEME_SPORT_CLUSTER:
            renderSportCluster(data);
            break;
        case THEME_OEM_PLUS:
            renderOemPlus(data);
            break;
        case THEME_DATA_MATRIX:
            renderDataMatrix(data);
            break;
        default:
            break;
    }
}

void DisplayUI::initTheme(DisplayTheme theme) {
    tft.fillScreen(TFT_BLACK);

    if (theme == THEME_CLASSIC_QUAD) {
        // Trennlinien für 4 Kacheln
        tft.drawFastHLine(0, 120, 320, COLOR_GRID_BORDER);
        tft.drawFastVLine(160, 0, 240, COLOR_GRID_BORDER);
    } else if (theme == THEME_OEM_PLUS) {
        // Golf 4 tiefblauer Hintergrund
        tft.fillScreen(COLOR_OEM_BLUE);
        // Zentrales MFA Display (Rot-brauner Rahmen)
        tft.drawRoundRect(105, 30, 110, 180, 4, COLOR_OEM_RED);
        tft.fillRoundRect(107, 32, 106, 176, 3, 0x3800); // Sehr dunkles MFA Rot
    }
}

// ==============================================================================
// DESIGN-STUDIE 1: KLASSISCHES 4-GRID (CLASSIC QUAD)
// ==============================================================================
void DisplayUI::renderClassicQuad(const SensorData &data) {
    char buf[16];

    // --- KACHEL 1 (Oben Links): LADEDRUCK ---
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("LADEDRUCK (bar)", 80, 8, 2);

    // Wert & Farbe (Cyan im Ladedruckbereich, Weiß im Unterdruck, Rot bei Overboost)
    uint16_t boostCol = COLOR_ACCENT_CYAN;
    if (data.boostBar < 0.0f) boostCol = TFT_WHITE;
    if (data.alarmBoost) boostCol = TFT_RED;

    dtostrf(data.boostBar, 4, 2, buf);
    tft.setTextColor(boostCol, TFT_BLACK);
    tft.drawString(buf, 80, 35, 6); // Große Font

    // Peak-Anzeige darunter
    snprintf(buf, sizeof(buf), "Peak: %.2f bar", data.boostPeakBar);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString(buf, 80, 95, 2);

    // --- KACHEL 2 (Oben Rechts): ABGASTEMPERATUR EGT ---
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("ABGASTEMP (C)", 240, 8, 2);

    uint16_t egtCol = COLOR_ACCENT_ORANGE;
    if (data.exhaustTemp >= ALARM_EGT_CRITICAL) egtCol = TFT_RED;
    else if (data.exhaustTemp >= ALARM_EGT_WARN) egtCol = COLOR_ACCENT_YELLOW;

    snprintf(buf, sizeof(buf), "%d", (int)data.exhaustTemp);
    tft.setTextColor(egtCol, TFT_BLACK);
    tft.drawString(buf, 240, 35, 6);

    snprintf(buf, sizeof(buf), "Max: %d C", (int)data.exhaustTempPeak);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString(buf, 240, 95, 2);

    // --- KACHEL 3 (Unten Links): LADELUFTTEMPERATUR (IAT) ---
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("LADELUFT (C)", 80, 128, 2);

    uint16_t iatCol = COLOR_ACCENT_GREEN;
    if (data.alarmChargeAir) iatCol = TFT_RED;
    else if (data.chargeAirTemp > 45.0f) iatCol = COLOR_ACCENT_YELLOW;

    snprintf(buf, sizeof(buf), "%.1f", data.chargeAirTemp);
    tft.setTextColor(iatCol, TFT_BLACK);
    tft.drawString(buf, 80, 155, 6);

    snprintf(buf, sizeof(buf), "ECU: %.0f C", data.ecuIntakeAirTemp);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString(buf, 80, 215, 2);

    // --- KACHEL 4 (Unten Rechts): DREHZAHL & KUEHLWASSER ---
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("DREHZAHL / WASSER", 240, 128, 2);

    snprintf(buf, sizeof(buf), "%d", data.ecuRpm);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(buf, 240, 155, 6);

    uint16_t waterCol = TFT_CYAN;
    if (data.alarmCoolant) waterCol = TFT_RED;
    snprintf(buf, sizeof(buf), "Wasser: %.0f C", data.ecuCoolantTemp);
    tft.setTextColor(waterCol, TFT_BLACK);
    tft.drawString(buf, 240, 215, 2);
}

// ==============================================================================
// DESIGN-STUDIE 2: SPORT / TRACK RACE CLUSTER
// ==============================================================================
void DisplayUI::drawRpmShiftBar(int x, int y, int w, int h, int rpm, int maxRpm) {
    int segments = 16;
    int segW = (w / segments) - 2;
    int activeSegs = (rpm * segments) / maxRpm;
    if (activeSegs > segments) activeSegs = segments;

    for (int i = 0; i < segments; i++) {
        uint16_t segCol = TFT_DARKGREY;
        if (i < activeSegs) {
            if (i < 9) segCol = COLOR_ACCENT_GREEN;       // 0 - 4500 RPM
            else if (i < 13) segCol = COLOR_ACCENT_YELLOW; // 4500 - 5800 RPM
            else segCol = TFT_RED;                         // > 5800 RPM (Schaltblitz)
        }
        tft.fillRect(x + i * (segW + 2), y, segW, h, segCol);
    }
}

void DisplayUI::renderSportCluster(const SensorData &data) {
    char buf[16];

    // 1. Drehzahlband oben (F1 / GT3 Shift-Light)
    drawRpmShiftBar(10, 10, 300, 14, data.ecuRpm, 7000);

    // Drehzahl in Ziffern klein daneben
    snprintf(buf, sizeof(buf), "%d RPM", data.ecuRpm);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(buf, 310, 28, 2);

    // 2. Riesige Ladedruckanzeige in der Bildschirmmitte
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("BOOST PRESSURE", 160, 48, 2);

    uint16_t boostCol = COLOR_ACCENT_CYAN;
    if (data.alarmBoost) boostCol = TFT_RED;
    dtostrf(data.boostBar, 4, 2, buf);
    tft.setTextColor(boostCol, TFT_BLACK);
    tft.drawString(buf, 160, 70, 7); // 7-Segment Riesenfont

    snprintf(buf, sizeof(buf), "PEAK: %.2f BAR", data.boostPeakBar);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString(buf, 160, 145, 2);

    // 3. Linker Vertikalbalken: Abgastemperatur EGT (0 - 1000 °C)
    int egtHeight = (int)((data.exhaustTemp / 1000.0f) * 80.0f);
    if (egtHeight > 80) egtHeight = 80;
    tft.drawRect(15, 60, 18, 84, TFT_DARKGREY);
    tft.fillRect(17, 62 + (80 - egtHeight), 14, egtHeight, (data.alarmEgt ? TFT_RED : COLOR_ACCENT_ORANGE));
    tft.fillRect(17, 62, 14, 80 - egtHeight, TFT_BLACK); // Löschen

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    snprintf(buf, sizeof(buf), "EGT: %d C", (int)data.exhaustTemp);
    tft.drawString(buf, 10, 150, 2);

    // 4. Untere Kacheln: Ladeluft & Kühlmittel & K-Line Status
    tft.drawFastHLine(0, 180, 320, COLOR_GRID_BORDER);

    // Ladeluft
    tft.drawString("IAT (Ladeluft):", 15, 190, 2);
    snprintf(buf, sizeof(buf), "%.1f C", data.chargeAirTemp);
    tft.setTextColor((data.alarmChargeAir ? TFT_RED : COLOR_ACCENT_GREEN), TFT_BLACK);
    tft.drawString(buf, 120, 190, 2);

    // Kühlwasser
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Kuehlwasser:", 15, 212, 2);
    snprintf(buf, sizeof(buf), "%.0f C", data.ecuCoolantTemp);
    tft.setTextColor((data.alarmCoolant ? TFT_RED : TFT_CYAN), TFT_BLACK);
    tft.drawString(buf, 120, 212, 2);

    // Luftmasse & Last
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    snprintf(buf, sizeof(buf), "MAF: %.1f g/s", data.ecuMafGs);
    tft.drawString(buf, 200, 190, 2);

    snprintf(buf, sizeof(buf), "Load: %.0f %%", data.ecuEngineLoadPct);
    tft.drawString(buf, 200, 212, 2);
}

// ==============================================================================
// DESIGN-STUDIE 3: OEM+ LOOK (GOLF 4 / AUDI A3 TACHO-DESIGN)
// ==============================================================================
void DisplayUI::renderOemPlus(const SensorData &data) {
    char buf[16];

    // Linke Rundinstrument-Simulation: Ladedruck (-1 bis +2 bar)
    // Tacho-Zifferblatt Links (Zentrum x=52, y=110, r=46)
    tft.drawCircle(52, 110, 46, TFT_WHITE);
    tft.drawCircle(52, 110, 45, TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, COLOR_OEM_BLUE);
    tft.drawString("BAR", 52, 85, 2);

    dtostrf(data.boostBar, 4, 2, buf);
    tft.setTextColor(COLOR_OEM_RED, COLOR_OEM_BLUE);
    tft.drawString(buf, 52, 115, 4);

    tft.setTextColor(TFT_WHITE, COLOR_OEM_BLUE);
    tft.drawString("-1.0", 25, 138, 1);
    tft.drawString("+2.0", 80, 138, 1);

    // Rechte Rundinstrument-Simulation: EGT (0 - 1100 °C)
    // Tacho-Zifferblatt Rechts (Zentrum x=268, y=110, r=46)
    tft.drawCircle(268, 110, 46, TFT_WHITE);
    tft.drawCircle(268, 110, 45, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, COLOR_OEM_BLUE);
    tft.drawString("EGT", 268, 85, 2);

    snprintf(buf, sizeof(buf), "%d", (int)data.exhaustTemp);
    tft.setTextColor(COLOR_OEM_RED, COLOR_OEM_BLUE);
    tft.drawString(buf, 268, 115, 4);

    tft.setTextColor(TFT_WHITE, COLOR_OEM_BLUE);
    tft.drawString("0", 240, 138, 1);
    tft.drawString("1100", 295, 138, 1);

    // Zentrales MFA Display (VW Bordcomputer / FIS Look in der Mitte)
    uint16_t mfaBg = 0x3800;
    tft.setTextColor(COLOR_OEM_RED, mfaBg);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("MFA 1.8T", 160, 40, 2);

    tft.drawString("LADELUFT", 160, 68, 1);
    snprintf(buf, sizeof(buf), "%.1f C", data.chargeAirTemp);
    tft.drawString(buf, 160, 80, 4);

    tft.drawString("WASSER", 160, 115, 1);
    snprintf(buf, sizeof(buf), "%.0f C", data.ecuCoolantTemp);
    tft.drawString(buf, 160, 127, 4);

    tft.drawString("DREHZAHL", 160, 160, 1);
    snprintf(buf, sizeof(buf), "%d", data.ecuRpm);
    tft.drawString(buf, 160, 172, 4);
}

// ==============================================================================
// DESIGN-STUDIE 4: TUNER & DIAGNOSTICS MATRIX (8 TELEMETRIE-WERTE)
// ==============================================================================
void DisplayUI::drawStatBox(int x, int y, int w, int h, const char *label, const char *valStr, const char *unit, uint16_t valColor) {
    tft.drawRoundRect(x, y, w, h, 3, COLOR_GRID_BORDER);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString(label, x + 5, y + 4, 1);

    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(valColor, TFT_BLACK);
    tft.drawString(valStr, x + w - 5, y + 18, 4);

    tft.setTextDatum(BR_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString(unit, x + w - 5, y + h - 3, 1);
}

void DisplayUI::renderDataMatrix(const SensorData &data) {
    char buf[16];

    // 8 Kacheln: 2 Spalten à 4 Zeilen
    int boxW = 152;
    int boxH = 52;
    int xLeft = 6;
    int xRight = 162;

    // Zeile 1
    dtostrf(data.boostBar, 4, 2, buf);
    drawStatBox(xLeft, 6, boxW, boxH, "LADEDRUCK", buf, "bar", (data.alarmBoost ? TFT_RED : COLOR_ACCENT_CYAN));

    snprintf(buf, sizeof(buf), "%.2f", data.boostPeakBar);
    drawStatBox(xRight, 6, boxW, boxH, "BOOST PEAK", buf, "bar", TFT_WHITE);

    // Zeile 2
    snprintf(buf, sizeof(buf), "%d", (int)data.exhaustTemp);
    drawStatBox(xLeft, 64, boxW, boxH, "ABGASTEMP (EGT)", buf, "deg C", (data.alarmEgt ? TFT_RED : COLOR_ACCENT_ORANGE));

    snprintf(buf, sizeof(buf), "%.1f", data.chargeAirTemp);
    drawStatBox(xRight, 64, boxW, boxH, "LADELUFT (LLK)", buf, "deg C", (data.alarmChargeAir ? TFT_RED : COLOR_ACCENT_GREEN));

    // Zeile 3
    snprintf(buf, sizeof(buf), "%.1f", data.ecuMafGs);
    drawStatBox(xLeft, 122, boxW, boxH, "LUFTMASSE (MAF)", buf, "g/s", TFT_YELLOW);

    snprintf(buf, sizeof(buf), "%.0f", data.ecuEngineLoadPct);
    drawStatBox(xRight, 122, boxW, boxH, "MOTORLAST", buf, "%", TFT_WHITE);

    // Zeile 4
    snprintf(buf, sizeof(buf), "%.1f", data.ecuIgnitionAngle);
    drawStatBox(xLeft, 180, boxW, boxH, "ZUENDWINKEL", buf, "deg KW", COLOR_ACCENT_CYAN);

    snprintf(buf, sizeof(buf), "%.1f", data.ecuBatteryVolt);
    drawStatBox(xRight, 180, boxW, boxH, "BORDSPANNUNG", buf, "Volt", TFT_WHITE);
}
