# VW 1.8T (AGU) Motronic M3.8.3 – ESP32 CYD Multi-Gauge 🚗💨

[![ESP32](https://img.shields.io/badge/Hardware-ESP32%20CYD-yellow.svg)](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)
[![Engine](https://img.shields.io/badge/Engine-1.8T%2020V%20AGU-blue.svg)](#)
[![ECU](https://img.shields.io/badge/ECU-Bosch%20Motronic%20M3.8.3-orange.svg)](#)
[![Protocol](https://img.shields.io/badge/OBD-KW1281%20%2F%20K--Line-red.svg)](#)
[![IDE](https://img.shields.io/badge/IDE-Arduino%20IDE-00979D.svg)](https://www.arduino.cc/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

Ein multifunktionales digitales Zusatzinstrument für Fahrzeuge der VAG-Gruppe mit dem beliebten **1.8T 20V (AGU)** Motor und **Bosch Motronic M3.8.3** Steuergerät (z. B. VW Golf 4, Audi A3 8L, Škoda Octavia 1U). 

Basierend auf dem kostengünstigen **ESP32 CYD ("Cheap Yellow Display" / ESP32-2432S028)** vereint dieses Projekt echte analoge Sensormessungen (Ladedruck, Abgastemperatur EGT, Ladelufttemperatur) mit nativen Live-Motordaten direkt aus dem Steuergerät über die **K-Line (KW1281-Protokoll)**.

---

## 🎨 4 Umschaltbare Display-Design Studien

Durch einfaches Tippen auf den Touchscreen kann jederzeit zwischen **4 vollwertigen Design-Themen** gewechselt werden:

```
+-----------------------------------+-----------------------------------+
| 1. CLASSIC QUAD (4-Kacheln)       | 2. SPORT CLUSTER (Rennsport)      |
| Große Digits für Ladedruck, EGT,  | F1/GT3 Shift-Light Drehzahlband,  |
| Ladeluft & Drehzahl/Wasser.       | riesiger Ladedruck, EGT-Balken.   |
+-----------------------------------+-----------------------------------+
| 3. OEM+ GOLF 4 / AUDI A3          | 4. DATA MATRIX (8 Telemetrie-Boxen|
| Original VW-Blaues Backlight mit  | High-Density Anzeige für Logging, |
| roten VDO-Zeigern & MFA-Display.  | Zündwinkel, MAF, Motorlast etc.   |
+-----------------------------------+-----------------------------------+
```

> [!TIP]
> **Interaktive Vorschau:** Eine interaktive Web-Simulation aller 4 Designs steht unter [`design_study/index.html`](design_study/index.html) bereit.

---

## 🚀 Features

- **Echter Ladedruck (Boost)**:
  - Da der 1.8T AGU ab Werk keinen Ladedrucksensor besitzt (reine Luftmassen-Steuerung), wird ein externer **3-Bar MAP-Sensor** (z. B. Bosch 0 281 002 437) über einen analogen Pin eingelesen.
  - **Auto-Zeroing:** Automatischer Nullpunktabgleich des Umgebungs-Barometerdrucks beim Einschalten der Zündung.
  - **Peak-Hold:** Höchster erreichter Ladedruck wird kontinuierlich gespeichert.
- **Ladelufttemperatur (Charge Air Temp / IAT)**:
  - Schneller NTC-Temperatursensor (z. B. Bosch M12) nach dem Ladeluftkühler / vor der Drosselklappe.
  - Steinhart-Hart / Beta-Berechnung mit Alarmierung bei Heat-Soak (> 60 °C).
  - Automatischer Fallback auf die native Steuergeräte-Ansauglufttemperatur via OBD.
- **Abgastemperatur (EGT)**:
  - Anschluss für ein **K-Typ Thermoelement** über den **Adafruit MAX31855** SPI-Verstärker.
  - Dynamische Warnstufen: Gelb ab 900 °C, Alarmrot ab 950 °C.
- **K-Line / KW1281 Diagnose-Schnittstelle**:
  - Vollständiger KW1281 Protokollstack mit 5-Baud Slow-Init (Adresse `0x01` Motor-ECU).
  - Zyklisches Auslesen der Messwertblöcke **MWB 001, MWB 002 und MWB 003**:
    - Motordrehzahl (RPM)
    - Kühlmitteltemperatur (°C)
    - Luftmasse (MAF g/s)
    - Motorlast (%)
    - Zündwinkel & Bordspannung (V)
- **Bench-Testumgebung & Simulator**:
  - Umschaltbarer Firmware-Simulator (`#define ENABLE_SIMULATION true`) für Tests ohne Auto.
  - Eigener Python KW1281 ECU-Simulator (`tools/ecu_simulator.py`) für Hardware-Tests am Schreibtisch.

---

## 📁 Projektstruktur

```
Motronic-OBD-Gauge/
├── AGU_ESP32_Gauge/
│   ├── AGU_ESP32_Gauge.ino       # Hauptsketch mit Setup- & Loop-Routinen
│   ├── Config.h                  # Pins, Kalibrierung, Alarmschwellen, Theme-Konfiguration
│   ├── Sensors.h & .cpp          # MAP-Sensor, MAX31855, Ladeluft-NTC & Filterung
│   ├── KW1281.h & .cpp           # K-Line KW1281 Diagnoseprotokoll-Implementierung
│   ├── Simulator.h & .cpp        # Realistischer Motor- und Fahrzyklus-Simulator
│   └── DisplayUI.h & .cpp        # Render-Engine für alle 4 Design-Themes & Touch
├── tools/
│   └── ecu_simulator.py          # Python PC-Simulator für KW1281 K-Line Tests
├── docs/
│   ├── SCHALTPLAN.md             # Vollständiger Verdrahtungsplan aller Sensoren
│   └── TESTUMGEBUNG.md           # Anleitung für Simulation & Bench-Testing
├── design_study/
│   └── index.html                # Interaktive Web-Designstudie & Sensor-Controller
├── .gitignore
├── LICENSE
└── README.md
```

---

## 🛠 Benötigte Hardware & Anschlüsse

| Komponente | Beschreibung | ESP32 CYD Pin |
| :--- | :--- | :--- |
| **ESP32 CYD** | ESP32-2432S028R mit 2.8" TFT & Touch | – |
| **MAP-Sensor** | 3-Bar Drucksensor (z. B. Bosch 0 281 002 437) | `GPIO 35` (über Spannungsteiler 5V -> 3.3V) |
| **Ladeluft-NTC** | Schneller NTC-Sensor nach Ladeluftkühler | `GPIO 34` (mit 2.2 kΩ Pullup zu 3.3V) |
| **MAX31855 Modul** | SPI-Verstärker für K-Typ Thermoelement | `CS: 21` / `DO: 22` / `CLK: 27` |
| **K-Line Transceiver** | L9637D oder Si9241A Board an OBD Pin 7 | `RX: GPIO 16` / `TX: GPIO 17` |
| **Spannungsversorgung** | 12V ➔ 5V Kfz Step-Down Wandler | Micro-USB / 5V Header am CYD |

> [!WARNING]
> Für Details zur Pegelanpassung (Spannungsteiler für MAP-Sensor) siehe den ausführlichen [Schaltplan](docs/SCHALTPLAN.md).

---

## 📦 Installation & Inbetriebnahme

1. **Repository klonen:**
   ```bash
   git clone https://github.com/Awsmqt/Motronic-OBD-Gauge.git
   ```
2. **Arduino IDE vorbereiten:**
   - Installiere das **ESP32 Board-Package** (`esp32` von Espressif Systems).
   - Installiere die Bibliotheken:
     - `TFT_eSPI` (Bodmer)
     - `Adafruit MAX31855 library` (Adafruit)
3. **`TFT_eSPI` Konfiguration:**
   - In der Datei `User_Setup.h` der TFT_eSPI Bibliothek sicherstellen, dass die CYD Pins (ILI9341 / ST7789) aktiviert sind.
4. **Flashen:**
   - Öffne [`AGU_ESP32_Gauge/AGU_ESP32_Gauge.ino`](AGU_ESP32_Gauge/AGU_ESP32_Gauge.ino) in der Arduino IDE.
   - Board: `ESP32 Dev Module`
   - Klicke auf **Upload**.

---

## 📄 Lizenz

Dieses Projekt ist unter der [MIT License](LICENSE) lizenziert.
