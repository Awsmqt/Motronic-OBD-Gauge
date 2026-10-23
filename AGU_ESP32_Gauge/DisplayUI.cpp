#include "DisplayUI.h"
#include "DtcDatabase.h"
#include <math.h>

DisplayUI::DisplayUI()
    : lastRenderedTheme(THEME_COUNT),
      touchPressStart(0),
      wasTouched(false),
      trendIndex(0),
      trendCount(0),
      lastTrendUpdate(0) {
    memset(trendOilTemp, 0, sizeof(trendOilTemp));
    memset(trendCoolant, 0, sizeof(trendCoolant));
    memset(trendChargeAir, 0, sizeof(trendChargeAir));
}

void DisplayUI::begin() {
    tft.init();
    tft.setRotation(1); // 320x240 Querformat
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
            // Langes Drücken (> 1.5s)
            if (millis() - touchPressStart > 1500) {
                if (data.currentTheme == THEME_DTC_DIAG) {
                    // DTC-Seite: Langes Drücken → DTCs löschen
                    data.dtcClearPending = true;
                } else {
                    // Andere Seiten: Min/Max-Werte zurücksetzen
                    data.boostPeakBar = data.boostBar;
                    data.exhaustTempPeak = data.exhaustTemp;
                    data.oilTempPeak = data.oilTemp;
                    data.oilPressureMinBar = data.oilPressureBar;
                }
                tft.fillScreen(TFT_BLACK);
                touchPressStart = millis();
            }
        }
    } else {
        if (wasTouched) {
            // Kurzer Tipp
            if (millis() - touchPressStart < 1000) {
                if (data.currentTheme == THEME_DTC_DIAG) {
                    // DTC-Seite: Kurzer Tipp → DTCs auslesen (kein Theme-Wechsel)
                    data.dtcReadPending = true;
                } else {
                    // Andere Seiten: Theme wechseln
                    nextTheme(data);
                }
            }
            wasTouched = false;
        }
    }
}

void DisplayUI::update(SensorData &data) {
    if (data.currentTheme != lastRenderedTheme) {
        lastRenderedTheme = data.currentTheme;
        initTheme(data.currentTheme);
    }

    switch (data.currentTheme) {
        case THEME_HEALTH_GRID:
            renderHealthGrid(data);
            break;
        case THEME_PRESSURE_TEMP:
            renderPressureTemp(data);
            break;
        case THEME_OEM_VDO:
            renderOemVdo(data);
            break;
        case THEME_MONITOR_MATRIX:
            renderMonitorMatrix(data);
            break;
        case THEME_DTC_DIAG:
            renderDtcDiag(data);
            break;
        case THEME_TREND_CHART:
            renderTrendChart(data);
            break;
        case THEME_ECU_LIVE:
            renderEcuLive(data);
            break;
        default:
            break;
    }
}

void DisplayUI::initTheme(DisplayTheme theme) {
    tft.fillScreen(TFT_BLACK);

    if (theme == THEME_HEALTH_GRID) {
        // Trennlinien für 6 Kacheln (2 Spalten x 3 Zeilen) + Footer
        tft.drawFastHLine(0, 68, 320, COLOR_GRID_BORDER);
        tft.drawFastHLine(0, 138, 320, COLOR_GRID_BORDER);
        tft.drawFastHLine(0, 208, 320, COLOR_GRID_BORDER);
        tft.drawFastVLine(160, 0, 208, COLOR_GRID_BORDER);
    } else if (theme == THEME_PRESSURE_TEMP) {
        // Oberer Bereich für Drücke, unterer für Temperaturen
        tft.drawFastHLine(0, 115, 320, COLOR_GRID_BORDER);
        tft.drawFastVLine(160, 0, 115, COLOR_GRID_BORDER);
    } else if (theme == THEME_OEM_VDO) {
        tft.fillScreen(COLOR_OEM_BLUE);
        // Zentrales MFA Bordcomputer-Gehäuse
        tft.drawRoundRect(108, 15, 104, 210, 4, COLOR_OEM_RED);
        tft.fillRoundRect(110, 17, 100, 206, 3, 0x2800); // Dunkles MFA-Rot
    } else if (theme == THEME_DTC_DIAG) {
        // Titelleiste für Fehlercode-Diagnose
        tft.fillRect(0, 0, 320, 24, 0x1000); // Dunkelrot-Hintergrund
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, 0x1000);
        tft.drawString("FEHLERSPEICHER (DTC)", 160, 12, 2);
    } else if (theme == THEME_TREND_CHART) {
        // Gitterlinien für das Verlaufsdiagramm zeichnen
        // Y-Achse: 20°C bis 140°C, Chartbereich x=60..260, y=40..200
        tft.setTextDatum(TR_DATUM);
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        const int gridTemps[] = {40, 60, 80, 100, 120};
        for (int i = 0; i < 5; i++) {
            int gy = 200 - (int)((gridTemps[i] - 20.0f) / 120.0f * 160.0f);
            tft.drawFastHLine(60, gy, 200, COLOR_GRID_BORDER);
            char lbl[8];
            snprintf(lbl, sizeof(lbl), "%d", gridTemps[i]);
            tft.drawString(lbl, 56, gy - 4, 1);
        }
        // Rahmen um Chart-Bereich
        tft.drawRect(59, 39, 202, 162, COLOR_GRID_BORDER);
    } else if (theme == THEME_ECU_LIVE) {
        // Titelleiste für ECU Live-Daten
        tft.fillRect(0, 0, 320, 22, COLOR_GRID_BORDER);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(COLOR_ACCENT_CYAN, COLOR_GRID_BORDER);
        tft.drawString("ECU LIVE-DATEN", 160, 11, 2);
    }
}

// ==============================================================================
// THEME 1: ENGINE HEALTH GRID (6 KACHELN: 2 DRÜCKE, 4 TEMPERATUREN + FOOTER)
// ==============================================================================
void DisplayUI::drawHealthTile(int x, int y, int w, int h, const char *label, const char *valStr, const char *unit, const char *subtext, uint16_t valColor) {
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString(label, x + 6, y + 5, 1);

    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(valColor, TFT_BLACK);
    tft.drawString(valStr, x + w - 30, y + 20, 4);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString(unit, x + w - 26, y + 32, 1);

    tft.setTextDatum(BL_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString(subtext, x + 6, y + h - 4, 1);
}

void DisplayUI::renderHealthGrid(const SensorData &data) {
    char val[16], sub[24];

    // --- ZEILE 1: DRÜCKE ---
    // Kachel 1: Ladedruck
    dtostrf(data.boostBar, 4, 2, val);
    snprintf(sub, sizeof(sub), "Peak: %.2f bar", data.boostPeakBar);
    uint16_t boostCol = data.alarmBoost ? TFT_RED : (data.boostBar < 0 ? TFT_WHITE : COLOR_ACCENT_CYAN);
    drawHealthTile(0, 0, 160, 68, "LADEDRUCK", val, "bar", sub, boostCol);

    // Kachel 2: Öldruck
    dtostrf(data.oilPressureBar, 4, 1, val);
    snprintf(sub, sizeof(sub), "Min: %.1f bar", data.oilPressureMinBar);
    uint16_t oilPressCol = data.alarmOilPressure ? TFT_RED : (data.oilPressureBar < 2.0f ? COLOR_ACCENT_YELLOW : COLOR_ACCENT_GREEN);
    drawHealthTile(160, 0, 160, 68, "OELDRUCK", val, "bar", sub, oilPressCol);

    // --- ZEILE 2: MOTOR-TEMPERATUREN ---
    // Kachel 3: Öltemperatur
    snprintf(val, sizeof(val), "%.0f", data.oilTemp);
    snprintf(sub, sizeof(sub), "Max: %.0f C", data.oilTempPeak);
    uint16_t oilTempCol = COLOR_ACCENT_GREEN;
    if (data.oilTemp < ALARM_OIL_TEMP_COLD) oilTempCol = COLOR_COLD_BLUE; // Kalt <75°C
    else if (data.alarmOilTemp) oilTempCol = TFT_RED;
    else if (data.oilTemp > 115.0f) oilTempCol = COLOR_ACCENT_YELLOW;
    drawHealthTile(0, 69, 160, 68, "OELTEMPERATUR", val, "C", sub, oilTempCol);

    // Kachel 4: Kühlwassertemperatur
    snprintf(val, sizeof(val), "%.0f", data.coolantTemp);
    snprintf(sub, sizeof(sub), "Soll: 90 C");
    uint16_t coolCol = data.alarmCoolant ? TFT_RED : (data.coolantTemp > 98.0f ? COLOR_ACCENT_YELLOW : TFT_WHITE);
    drawHealthTile(160, 69, 160, 68, "KUEHLWASSER", val, "C", sub, coolCol);

    // --- ZEILE 3: LADELUFT & ABGAS ---
    // Kachel 5: Ladelufttemperatur (nach LLK)
    snprintf(val, sizeof(val), "%.1f", data.chargeAirTemp);
    snprintf(sub, sizeof(sub), "ECU: %.0f C", data.ecuIntakeAirTemp);
    uint16_t iatCol = data.alarmChargeAir ? TFT_RED : (data.chargeAirTemp > 45.0f ? COLOR_ACCENT_YELLOW : COLOR_ACCENT_GREEN);
    drawHealthTile(0, 139, 160, 68, "LADELUFT (LLK)", val, "C", sub, iatCol);

    // Kachel 6: Abgastemperatur EGT
    snprintf(val, sizeof(val), "%d", (int)data.exhaustTemp);
    snprintf(sub, sizeof(sub), "Max: %d C", (int)data.exhaustTempPeak);
    uint16_t egtCol = data.alarmEgt ? TFT_RED : (data.exhaustTemp > 850.0f ? COLOR_ACCENT_YELLOW : COLOR_ACCENT_ORANGE);
    drawHealthTile(160, 139, 160, 68, "ABGAS (EGT)", val, "C", sub, egtCol);

    // --- FOOTER ZEILE: BORDSPANNUNG & DIAGNOSE ---
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(data.alarmBattery ? TFT_RED : TFT_WHITE, TFT_BLACK);
    snprintf(val, sizeof(val), "BATT: %.1f V", data.batteryVolt);
    tft.drawString(val, 12, 224, 2);

    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(data.klineConnected ? COLOR_ACCENT_GREEN : TFT_DARKGREY, TFT_BLACK);
    tft.drawString(data.klineConnected ? "ECU K-LINE OK" : "K-LINE OFFLINE", 308, 224, 2);
}

// ==============================================================================
// THEME 2: TWIN PRESSURES & QUAD TEMPERATURES
// ==============================================================================
void DisplayUI::drawHorizontalBar(int x, int y, int w, int h, float val, float minV, float maxV, uint16_t barCol) {
    tft.drawRect(x, y, w, h, COLOR_GRID_BORDER);
    float pct = (val - minV) / (maxV - minV);
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 1.0f) pct = 1.0f;
    int fillW = (int)(pct * (w - 4));
    tft.fillRect(x + 2, y + 2, fillW, h - 4, barCol);
    tft.fillRect(x + 2 + fillW, y + 2, (w - 4) - fillW, h - 4, TFT_BLACK);
}

void DisplayUI::renderPressureTemp(const SensorData &data) {
    char buf[16];

    // OBERE HAELFTE: GROSSE DRÜCKE
    // 1. Ladedruck Links
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("LADEDRUCK (bar)", 80, 8, 2);

    dtostrf(data.boostBar, 4, 2, buf);
    uint16_t boostCol = data.alarmBoost ? TFT_RED : (data.boostBar < 0 ? TFT_WHITE : COLOR_ACCENT_CYAN);
    tft.setTextColor(boostCol, TFT_BLACK);
    tft.drawString(buf, 80, 32, 6); // Riesen-Font

    drawHorizontalBar(15, 88, 130, 10, data.boostBar, -1.0f, 2.0f, boostCol);

    // 2. Öldruck Rechts
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("OELDRUCK (bar)", 240, 8, 2);

    dtostrf(data.oilPressureBar, 4, 1, buf);
    uint16_t oilCol = data.alarmOilPressure ? TFT_RED : (data.oilPressureBar < 2.0f ? COLOR_ACCENT_YELLOW : COLOR_ACCENT_GREEN);
    tft.setTextColor(oilCol, TFT_BLACK);
    tft.drawString(buf, 240, 32, 6);

    drawHorizontalBar(175, 88, 130, 10, data.oilPressureBar, 0.0f, 7.0f, oilCol);

    // UNTERE HAELFTE: DIE 4 TEMPERATUREN NEBENEINANDER (4 Spalten à 75px)
    int colW = 75;
    int yTop = 125;

    // Col 1: Öltemp
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("OEL", 40, yTop, 2);
    snprintf(buf, sizeof(buf), "%.0f C", data.oilTemp);
    uint16_t oCol = (data.oilTemp < 75) ? COLOR_COLD_BLUE : (data.alarmOilTemp ? TFT_RED : COLOR_ACCENT_GREEN);
    tft.setTextColor(oCol, TFT_BLACK);
    tft.drawString(buf, 40, yTop + 24, 4);
    drawHorizontalBar(8, yTop + 55, 64, 8, data.oilTemp, 40.0f, 140.0f, oCol);

    // Col 2: Kühlwasser
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("WASSER", 120, yTop, 2);
    snprintf(buf, sizeof(buf), "%.0f C", data.coolantTemp);
    uint16_t wCol = data.alarmCoolant ? TFT_RED : TFT_CYAN;
    tft.setTextColor(wCol, TFT_BLACK);
    tft.drawString(buf, 120, yTop + 24, 4);
    drawHorizontalBar(88, yTop + 55, 64, 8, data.coolantTemp, 40.0f, 120.0f, wCol);

    // Col 3: Ladeluft
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("LADELUFT", 200, yTop, 2);
    snprintf(buf, sizeof(buf), "%.0f C", data.chargeAirTemp);
    uint16_t iCol = data.alarmChargeAir ? TFT_RED : COLOR_ACCENT_GREEN;
    tft.setTextColor(iCol, TFT_BLACK);
    tft.drawString(buf, 200, yTop + 24, 4);
    drawHorizontalBar(168, yTop + 55, 64, 8, data.chargeAirTemp, 10.0f, 70.0f, iCol);

    // Col 4: Abgas EGT
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("EGT", 280, yTop, 2);
    snprintf(buf, sizeof(buf), "%d", (int)data.exhaustTemp);
    uint16_t eCol = data.alarmEgt ? TFT_RED : COLOR_ACCENT_ORANGE;
    tft.setTextColor(eCol, TFT_BLACK);
    tft.drawString(buf, 280, yTop + 24, 4);
    drawHorizontalBar(248, yTop + 55, 64, 8, data.exhaustTemp, 300.0f, 1050.0f, eCol);

    // Batteriespannung ganz unten
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    snprintf(buf, sizeof(buf), "Bordspannung: %.1f Volt", data.batteryVolt);
    tft.drawString(buf, 160, 222, 2);
}

// ==============================================================================
// THEME 3: OEM+ GOLF 4 VDO LOOK (DRÜCKE ALS RUNDINSTRUMENTE, TEMPERATUREN IM MFA)
// ==============================================================================
void DisplayUI::drawVdoDial(int cx, int cy, int r, float val, float minV, float maxV, const char *unit, const char *title, uint16_t needleCol) {
    tft.drawCircle(cx, cy, r, TFT_WHITE);
    tft.drawCircle(cx, cy, r - 1, TFT_WHITE);

    // Beschriftung
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, COLOR_OEM_BLUE);
    tft.drawString(title, cx, cy - 22, 1);
    tft.drawString(unit, cx, cy - 8, 1);

    // Digitalwert
    char buf[10];
    if (maxV <= 5.0f) dtostrf(val, 4, 2, buf);
    else dtostrf(val, 3, 1, buf);
    tft.setTextColor(COLOR_OEM_RED, COLOR_OEM_BLUE);
    tft.drawString(buf, cx, cy + 22, 2);

    // Nadel (Winkel: 135° bis 405°)
    float pct = (val - minV) / (maxV - minV);
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 1.0f) pct = 1.0f;
    float angle = 2.356f + pct * 3.1415f; // Radian

    int nx = cx + (int)(cos(angle) * (r - 8));
    int ny = cy + (int)(sin(angle) * (r - 8));
    tft.drawLine(cx, cy, nx, ny, needleCol);
    tft.fillCircle(cx, cy, 4, TFT_WHITE);
}

void DisplayUI::renderOemVdo(const SensorData &data) {
    // Rundinstrument Links: Ladedruck (-1.0 bis +2.0 bar)
    drawVdoDial(54, 120, 50, data.boostBar, -1.0f, 2.0f, "bar", "BOOST", COLOR_OEM_RED);

    // Rundinstrument Rechts: Öldruck (0 bis 7.0 bar)
    drawVdoDial(266, 120, 50, data.oilPressureBar, 0.0f, 7.0f, "bar", "OELDRUCK", COLOR_OEM_RED);

    // Zentrales MFA Bordcomputer Display (Mitte)
    char buf[20];
    uint16_t mfaBg = 0x2800;
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(COLOR_OEM_RED, mfaBg);
    tft.drawString("VDO MFA", 160, 22, 1);

    // Öltemperatur
    tft.drawString("OELTEMP", 160, 42, 1);
    snprintf(buf, sizeof(buf), "%.0f C", data.oilTemp);
    tft.drawString(buf, 160, 56, 4);

    // Kühlwasser
    tft.drawString("WASSER", 160, 88, 1);
    snprintf(buf, sizeof(buf), "%.0f C", data.coolantTemp);
    tft.drawString(buf, 160, 102, 4);

    // Ladeluft
    tft.drawString("LADELUFT", 160, 134, 1);
    snprintf(buf, sizeof(buf), "%.1f C", data.chargeAirTemp);
    tft.drawString(buf, 160, 148, 2);

    // Abgastemperatur EGT
    tft.drawString("ABGAS EGT", 160, 172, 1);
    snprintf(buf, sizeof(buf), "%d C", (int)data.exhaustTemp);
    tft.drawString(buf, 160, 186, 2);

    // Batteriespannung
    snprintf(buf, sizeof(buf), "%.1f V", data.batteryVolt);
    tft.drawString(buf, 160, 208, 1);
}

// ==============================================================================
// THEME 4: MONITOR MATRIX (STATUS-LEISTEN MIT ALARMGRENZEN)
// ==============================================================================
void DisplayUI::renderMonitorMatrix(const SensorData &data) {
    char label[16], val[16];
    int y0 = 6;
    int rowH = 34;

    struct ParamRow {
        const char *name;
        float value;
        float minV;
        float maxV;
        const char *unit;
        uint16_t col;
        bool alarm;
    };

    ParamRow rows[6] = {
        { "OELDRUCK",   data.oilPressureBar, 0.0f,  7.0f,  "bar", COLOR_ACCENT_GREEN,  data.alarmOilPressure },
        { "OELTEMP",    data.oilTemp,       40.0f, 140.0f, "C",   (data.oilTemp < 75 ? COLOR_COLD_BLUE : COLOR_ACCENT_GREEN), data.alarmOilTemp },
        { "WASSER",     data.coolantTemp,   40.0f, 120.0f, "C",   TFT_CYAN,            data.alarmCoolant },
        { "LADELUFT",   data.chargeAirTemp, 10.0f,  70.0f, "C",   COLOR_ACCENT_GREEN,  data.alarmChargeAir },
        { "ABGAS EGT",  data.exhaustTemp,  300.0f, 1050.0f,"C",   COLOR_ACCENT_ORANGE, data.alarmEgt },
        { "LADEDRUCK",  data.boostBar,      -1.0f,  2.0f,  "bar", COLOR_ACCENT_CYAN,   data.alarmBoost }
    };

    for (int i = 0; i < 6; i++) {
        int y = y0 + i * rowH;

        // Label
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        tft.drawString(rows[i].name, 8, y + 2, 1);

        // Wert
        tft.setTextDatum(TR_DATUM);
        uint16_t c = rows[i].alarm ? TFT_RED : rows[i].col;
        tft.setTextColor(c, TFT_BLACK);
        if (rows[i].maxV <= 3.0f || rows[i].maxV >= 100.0f) {
            snprintf(val, sizeof(val), "%.1f %s", rows[i].value, rows[i].unit);
        } else {
            snprintf(val, sizeof(val), "%.2f %s", rows[i].value, rows[i].unit);
        }
        tft.drawString(val, 312, y + 2, 2);

        // Statusbalken
        drawHorizontalBar(8, y + 20, 304, 8, rows[i].value, rows[i].minV, rows[i].maxV, c);
    }

    // Fußzeile
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    snprintf(val, sizeof(val), "BATT: %.1fV | PEAK BOOST: %.2f bar", data.batteryVolt, data.boostPeakBar);
    tft.drawString(val, 8, 225, 1);
}

// ==============================================================================
// THEME 5: FEHLERCODE-DIAGNOSE (DTC READ & CLEAR)
// ==============================================================================
void DisplayUI::formatDtcCode(uint16_t raw, char *out) {
    dtcFormatCode(raw, out, 12);
}

void DisplayUI::renderDtcDiag(SensorData &data) {
    // Kopfzeile: Status
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE, 0x1000);
    tft.drawString("FEHLERSPEICHER (DTC)", 8, 5, 2);

    // MIL Status-Indikator
    if (data.milSeen || data.dtcCount > 0) {
        tft.fillCircle(270, 12, 6, TFT_RED);
        tft.setTextColor(TFT_RED, 0x1000);
        tft.drawString("MIL AN", 282, 5, 1);
    } else {
        tft.fillCircle(270, 12, 6, COLOR_ACCENT_GREEN);
        tft.setTextColor(COLOR_ACCENT_GREEN, 0x1000);
        tft.drawString("MIL OK", 282, 5, 1);
    }

    // Inhalt
    int yStart = 35;
    if (data.dtcCount == 0) {
        // Keine Fehler
        tft.fillRect(0, yStart, 320, 150, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(COLOR_ACCENT_GREEN, TFT_BLACK);
        tft.drawString("KEINE FEHLER GESPEICHERT", 160, 95, 4);

        tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        tft.drawString("Steuergeraet Fehlerspeicher ist sauber.", 160, 130, 2);

        char klineStr[40];
        snprintf(klineStr, sizeof(klineStr), "K-Line: %s (%lu Pakete)",
                 data.klineConnected ? "Verbunden" : "Offline", (unsigned long)data.klinePackets);
        tft.drawString(klineStr, 160, 155, 1);
    } else {
        // Fehlerliste anzeigen
        tft.fillRect(0, yStart, 320, 150, TFT_BLACK);
        char dtcBuf[16];
        char lineBuf[48];

        int displayCount = data.dtcCount > 5 ? 5 : data.dtcCount;
        for (int i = 0; i < displayCount; i++) {
            formatDtcCode(data.dtcCodes[i], dtcBuf);
            uint8_t stat = data.dtcStatus[i];

            const char *dtcText;
            const char *dtcCause;
            dtcLookup(data.dtcCodes[i], &dtcText, &dtcCause);

            // Zeile 1: Code + Status (rot), Zeile 2: Klartext (hellgrau)
            snprintf(lineBuf, sizeof(lineBuf), "%s - %s", dtcBuf, dtcStatusText(stat));
            tft.setTextDatum(TL_DATUM);
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.drawString(lineBuf, 12, yStart + i * 26, 2);

            snprintf(lineBuf, sizeof(lineBuf), "%.44s", dtcText);
            tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            tft.drawString(lineBuf, 120, yStart + i * 26 + 4, 1);
        }

        if (data.dtcCount > 5) {
            snprintf(lineBuf, sizeof(lineBuf), "... und %d weitere Fehler (siehe Web-Dashboard)", data.dtcCount - 5);
            tft.setTextColor(COLOR_ACCENT_YELLOW, TFT_BLACK);
            tft.drawString(lineBuf, 12, yStart + 5 * 26 + 4, 1);
        }
    }

    // Fußzeile: Bedienungsanleitung
    tft.drawFastHLine(0, 195, 320, COLOR_GRID_BORDER);
    tft.fillRect(0, 196, 320, 44, 0x0821);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COLOR_ACCENT_CYAN, 0x0821);
    tft.drawString("TIPPEN: Neu auslesen", 90, 218, 2);

    tft.drawFastVLine(180, 200, 36, COLOR_GRID_BORDER);

    tft.setTextColor(COLOR_ACCENT_ORANGE, 0x0821);
    tft.drawString("HALTEN: Speicher loeschen", 250, 218, 1);
}

// ==============================================================================
// THEME 6: TREND-CHART (TEMPERATUR-VERLAUF UEBER 60 SEKUNDEN)
// ==============================================================================
void DisplayUI::drawTrendLine(const float *buf, uint8_t idx, uint8_t count, int x, int y, int w, int h, float minV, float maxV, uint16_t color) {
    if (count < 2) return;

    float dx = (float)w / (float)(TREND_BUF_SIZE - 1);

    for (int i = 0; i < count - 1; i++) {
        int bufIdx1 = (idx - count + i + TREND_BUF_SIZE) % TREND_BUF_SIZE;
        int bufIdx2 = (idx - count + i + 1 + TREND_BUF_SIZE) % TREND_BUF_SIZE;

        float v1 = buf[bufIdx1];
        float v2 = buf[bufIdx2];

        // Skalieren auf Y
        float p1 = (v1 - minV) / (maxV - minV);
        float p2 = (v2 - minV) / (maxV - minV);
        if (p1 < 0.0f) p1 = 0.0f; else if (p1 > 1.0f) p1 = 1.0f;
        if (p2 < 0.0f) p2 = 0.0f; else if (p2 > 1.0f) p2 = 1.0f;

        int y1 = (y + h) - (int)(p1 * (float)h);
        int y2 = (y + h) - (int)(p2 * (float)h);

        int x1 = x + (int)((float)i * dx);
        int x2 = x + (int)((float)(i + 1) * dx);

        tft.drawLine(x1, y1, x2, y2, color);
        tft.drawLine(x1, y1 + 1, x2, y2 + 1, color); // Dickere Linie
    }
}

void DisplayUI::renderTrendChart(const SensorData &data) {
    unsigned long now = millis();

    // Ringpuffer einmal pro Sekunde aktualisieren
    if (now - lastTrendUpdate >= 1000) {
        lastTrendUpdate = now;
        trendOilTemp[trendIndex] = data.oilTemp;
        trendCoolant[trendIndex] = data.coolantTemp;
        trendChargeAir[trendIndex] = data.chargeAirTemp;
        trendIndex = (trendIndex + 1) % TREND_BUF_SIZE;
        if (trendCount < TREND_BUF_SIZE) trendCount++;
    }

    // Chart-Innenraum leeren
    tft.fillRect(60, 40, 200, 160, TFT_BLACK);

    // Gitterlinien neu zeichnen
    const int gridTemps[] = {40, 60, 80, 100, 120};
    for (int i = 0; i < 5; i++) {
        int gy = 200 - (int)((gridTemps[i] - 20.0f) / 120.0f * 160.0f);
        tft.drawFastHLine(60, gy, 200, COLOR_GRID_BORDER);
    }

    // 3 Kurven zeichnen: Öl (Orange), Wasser (Cyan), Ladeluft (Grün)
    drawTrendLine(trendOilTemp, trendIndex, trendCount, 60, 40, 200, 160, 20.0f, 140.0f, COLOR_ACCENT_ORANGE);
    drawTrendLine(trendCoolant, trendIndex, trendCount, 60, 40, 200, 160, 20.0f, 140.0f, TFT_CYAN);
    drawTrendLine(trendChargeAir, trendIndex, trendCount, 60, 40, 200, 160, 20.0f, 140.0f, COLOR_ACCENT_GREEN);

    // Legende & Live-Werte oben
    char leg[32];
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COLOR_ACCENT_ORANGE, TFT_BLACK);
    snprintf(leg, sizeof(leg), "OEL %.0fC", data.oilTemp);
    tft.drawString(leg, 10, 8, 2);

    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    snprintf(leg, sizeof(leg), "H2O %.0fC", data.coolantTemp);
    tft.drawString(leg, 150, 8, 2);

    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(COLOR_ACCENT_GREEN, TFT_BLACK);
    snprintf(leg, sizeof(leg), "LLK %.1fC", data.chargeAirTemp);
    tft.drawString(leg, 310, 8, 2);

    // Drücke rechts außen
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("BOOST", 265, 55, 1);
    char bStr[12];
    dtostrf(data.boostBar, 4, 2, bStr);
    tft.setTextColor(data.alarmBoost ? TFT_RED : COLOR_ACCENT_CYAN, TFT_BLACK);
    tft.drawString(bStr, 265, 70, 2);

    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("OEL-P", 265, 115, 1);
    dtostrf(data.oilPressureBar, 4, 1, bStr);
    tft.setTextColor(data.alarmOilPressure ? TFT_RED : COLOR_ACCENT_GREEN, TFT_BLACK);
    tft.drawString(bStr, 265, 130, 2);

    // Fußzeile
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    snprintf(leg, sizeof(leg), "60s Trend | EGT: %dC | Batt: %.1fV",
             (int)data.exhaustTemp, data.batteryVolt);
    tft.drawString(leg, 10, 225, 1);
}

// ==============================================================================
// THEME 7: ECU LIVE-DATEN (LAMBDA, EINSPRITZUNG, ZUENDWINKEL, KLOPFEN)
// ==============================================================================
void DisplayUI::renderEcuLive(const SensorData &data) {
    char valBuf[24];

    // Titelleiste
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE, COLOR_GRID_BORDER);
    tft.drawString("ECU LIVE-DATEN (KW1281 MWB)", 8, 4, 2);

    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(data.klineConnected ? COLOR_ACCENT_GREEN : TFT_RED, COLOR_GRID_BORDER);
    snprintf(valBuf, sizeof(valBuf), "%d RPM", data.ecuRpm);
    tft.drawString(valBuf, 312, 4, 2);

    int y0 = 30;
    int rowH = 30;

    // 1. Lambda-Wert
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("LAMBDA", 10, y0 + 2, 2);

    float lam = data.ecuLambda > 0.1f ? data.ecuLambda : 1.000f;
    uint16_t lamCol = (lam >= 0.95f && lam <= 1.05f) ? COLOR_ACCENT_GREEN : (lam < 0.90f ? COLOR_ACCENT_YELLOW : TFT_RED);
    snprintf(valBuf, sizeof(valBuf), "%.3f", lam);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(lamCol, TFT_BLACK);
    tft.drawString(valBuf, 180, y0 + 2, 2);

    // Lambda Balken (0.75 bis 1.25, Mitte = 1.00)
    drawHorizontalBar(190, y0 + 6, 120, 10, lam, 0.75f, 1.25f, lamCol);

    // 2. Einspritzzeit
    int y = y0 + rowH;
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("EINSPRITZZEIT", 10, y + 2, 2);

    snprintf(valBuf, sizeof(valBuf), "%.2f ms", data.ecuInjectionMs);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(COLOR_ACCENT_CYAN, TFT_BLACK);
    tft.drawString(valBuf, 180, y + 2, 2);
    drawHorizontalBar(190, y + 6, 120, 10, data.ecuInjectionMs, 0.0f, 18.0f, COLOR_ACCENT_CYAN);

    // 3. Zündwinkel
    y += rowH;
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("ZUENDWINKEL", 10, y + 2, 2);

    snprintf(valBuf, sizeof(valBuf), "%.1f deg", data.ecuIgnitionTiming);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(COLOR_ACCENT_ORANGE, TFT_BLACK);
    tft.drawString(valBuf, 180, y + 2, 2);
    drawHorizontalBar(190, y + 6, 120, 10, data.ecuIgnitionTiming, 0.0f, 40.0f, COLOR_ACCENT_ORANGE);

    // 4. Klopfregelung Rücknahme
    y += rowH;
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("KLOPFRUECKNAHME", 10, y + 2, 2);

    uint16_t knockCol = data.ecuKnockRetard <= 0.5f ? COLOR_ACCENT_GREEN : (data.ecuKnockRetard <= 3.0f ? COLOR_ACCENT_YELLOW : TFT_RED);
    snprintf(valBuf, sizeof(valBuf), "%.1f deg", data.ecuKnockRetard);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(knockCol, TFT_BLACK);
    tft.drawString(valBuf, 180, y + 2, 2);
    drawHorizontalBar(190, y + 6, 120, 10, data.ecuKnockRetard, 0.0f, 12.0f, knockCol);

    // 5. Drosselklappe
    y += rowH;
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("DROSSELKLAPPE", 10, y + 2, 2);

    snprintf(valBuf, sizeof(valBuf), "%.1f %%", data.ecuThrottlePct);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(valBuf, 180, y + 2, 2);
    drawHorizontalBar(190, y + 6, 120, 10, data.ecuThrottlePct, 0.0f, 100.0f, TFT_WHITE);

    // 6. Last & Luftmasse
    y += rowH;
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("LAST / MAF", 10, y + 2, 2);

    snprintf(valBuf, sizeof(valBuf), "%.0f%% | %.1f g/s", data.ecuEngineLoadPct, data.ecuMafGs);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(COLOR_ACCENT_CYAN, TFT_BLACK);
    tft.drawString(valBuf, 312, y + 2, 2);

    // Fußzeile
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    snprintf(valBuf, sizeof(valBuf), "K-Line: %lu Pakete | SD-Log: %lu Z.",
             (unsigned long)data.klinePackets, (unsigned long)data.logLinesWritten);
    tft.drawString(valBuf, 10, 225, 1);
}
