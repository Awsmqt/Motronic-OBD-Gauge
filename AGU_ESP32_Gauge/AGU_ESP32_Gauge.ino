/*
 * VW 1.8T AGU (Motronic M3.8.3) ESP32 CYD Gauge
 * Base UI, MAP Sensor (Boost), EGT (MAX31855) and KW1281 Serial init.
 */

#include <SPI.h>
#include <TFT_eSPI.h>       // Display Library (muss in der User_Setup.h für CYD konfiguriert sein!)
#include <Adafruit_MAX31855.h> // Library für EGT Sensor

// --- PINS CYD (Cheap Yellow Display) Erweiterungs-Pins ---
// Ladedrucksensor (3-Bar MAP z.B. Bosch) an einem freien analogen Pin
#define PIN_MAP_SENSOR  35  

// MAX31855 (Abgastemperatur) SPI Pins (Beispiel für freie CYD Pins)
#define MAXDO   22 
#define MAXCS   21 
#define MAXCLK  27 

// K-Line (L9637D Transceiver) an Serial2 des ESP32
#define PIN_KLINE_RX 16
#define PIN_KLINE_TX 17

// --- SENSOR-OBJEKTE ---
TFT_eSPI tft = TFT_eSPI(); 
Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);

// --- VARIABLEN ---
float boostPressure = 0.0;
float exhaustTemp = 0.0;
int rpm = 0;       // Kommt später über K-Line
int coolant = 0;   // Kommt später über K-Line

// Timer für non-blocking Updates
unsigned long lastDisplayUpdate = 0;
unsigned long lastSensorUpdate = 0;

void setup() {
  Serial.begin(115200); // Debug Konsole PC
  
  // 1. Display Initialisieren
  tft.init();
  tft.setRotation(1); // Querformat (Landscape)
  tft.fillScreen(TFT_BLACK);
  
  // UI Rahmen zeichnen
  drawUI();

  // 2. EGT Sensor Initialisieren
  if (!thermocouple.begin()) {
    Serial.println("Fehler: MAX31855 nicht gefunden!");
  }

  // 3. K-Line / KW1281 Hardware Serial2 vorbereiten
  // VW AGU nutzt 10400 Baud für die Diagnose (KW1281)
  Serial2.begin(10400, SERIAL_8N1, PIN_KLINE_RX, PIN_KLINE_TX);
  Serial.println("System gestartet. K-Line bereit (10400 Baud).");
}

void loop() {
  unsigned long currentMillis = millis();

  // Sensoren alle 50ms (20Hz) auslesen
  if (currentMillis - lastSensorUpdate >= 50) {
    readSensors();
    // Hier später readKLine() aufrufen
    lastSensorUpdate = currentMillis;
  }

  // Display alle 100ms (10Hz) updaten (verhindert Flackern)
  if (currentMillis - lastDisplayUpdate >= 100) {
    updateDisplay();
    lastDisplayUpdate = currentMillis;
  }
}

// --- SENSOR LOGIK ---
void readSensors() {
  // 1. MAP Sensor lesen (Ladedruck)
  // Ein 3-Bar Sensor hat typischerweise: 0.5V = 0.2 Bar (Absolut), 4.5V = 3.0 Bar (Absolut)
  // Umgebungsdruck (1 Bar) abziehen für reinen Ladedruck (Relativdruck).
  int rawADC = analogRead(PIN_MAP_SENSOR);
  float voltage = (rawADC / 4095.0) * 3.3; 
  // (Hinweis: Die ESP32 ADC-Kurve ist nicht linear, in der Praxis muss man das noch leicht kalibrieren)
  
  // Dummy-Rechnung: (voltage / max_voltage) * max_pressure - atmosphärischer Druck
  float absolutePressure = (voltage / 5.0) * 3.0; // 5V Sensor am Spannungsteiler oder 3.3V angepasst
  boostPressure = absolutePressure - 1.013;       // 1.013 bar Atmosphärendruck abziehen
  if(boostPressure < -1.0) boostPressure = -1.0;  // Max Unterdruck

  // 2. EGT Sensor lesen
  exhaustTemp = thermocouple.readCelsius();
  
  // (Zum Testen ohne echtes Auto: Dummy-Werte generieren)
  if (isnan(exhaustTemp)) exhaustTemp = 0.0;
  
  // Fake-Werte für RPM & Coolant, bis KW1281 läuft
  rpm = random(2800, 3000); 
  coolant = 90;
}

// --- UI LAYOUT ZEICHNEN (Einmalig) ---
void drawUI() {
  tft.drawFastHLine(0, 120, 320, TFT_WHITE); // Horizontale Linie Mitte
  tft.drawFastVLine(160, 0, 240, TFT_WHITE); // Vertikale Linie Mitte

  // Labels
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setTextDatum(TC_DATUM); // Top Center
  
  tft.drawString("Ladedruck (bar)", 80, 10, 2);
  tft.drawString("Abgastemp (C)", 240, 10, 2);
  tft.drawString("Drehzahl (U/min)", 80, 130, 2);
  tft.drawString("Kuehlwasser (C)", 240, 130, 2);
}

// --- WERTE AKTUALISIEREN ---
void updateDisplay() {
  tft.setTextColor(TFT_GREEN, TFT_BLACK); // Text Grün, Hintergrund Schwarz (überschreibt alte Werte)
  tft.setTextDatum(MC_DATUM); // Middle Center

  // 1. Ladedruck (Oben Links) - Textgröße groß (Font 4 oder 6)
  char boostStr[10];
  dtostrf(boostPressure, 4, 2, boostStr); // Format: x.xx
  tft.drawString(boostStr, 80, 70, 6);

  // 2. EGT (Oben Rechts) - Rot wenn über 900 Grad
  if(exhaustTemp > 900) tft.setTextColor(TFT_RED, TFT_BLACK);
  else tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.drawNumber((int)exhaustTemp, 240, 70, 6);

  // 3. RPM (Unten Links)
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawNumber(rpm, 80, 190, 6);

  // 4. Wasser (Unten Rechts)
  if(coolant > 100) tft.setTextColor(TFT_RED, TFT_BLACK);
  else tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.drawNumber(coolant, 240, 190, 6);
}