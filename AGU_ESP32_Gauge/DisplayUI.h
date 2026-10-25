#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include <TFT_eSPI.h>
#include "Config.h"

class DisplayUI {
public:
    DisplayUI();
    void begin();
    void update(SensorData &data);
    void nextTheme(SensorData &data);
    void checkTouch(SensorData &data);

private:
    TFT_eSPI tft;
    DisplayTheme lastRenderedTheme;
    unsigned long touchPressStart;
    bool wasTouched;

    const uint16_t COLOR_BG = 0x0821;
    const uint16_t COLOR_LINE = 0x2124;
    const uint16_t COLOR_CYAN = 0x07FF;
    const uint16_t COLOR_GREEN = 0x07E0;
    const uint16_t COLOR_YELLOW = 0xFFE0;
    const uint16_t COLOR_BLUE = 0x03BF;

    void renderMeasurements(const SensorData &data);
    void renderHealth(const SensorData &data);
    void drawBar(int x, int y, int w, float value, float minValue, float maxValue, uint16_t color);
    uint16_t healthColor(bool warning, bool critical) const;
};

#endif // DISPLAY_UI_H
