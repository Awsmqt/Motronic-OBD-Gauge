# VW 1.8T (AGU) Motronic M3.8.3 – ESP32 CYD Motor-Zustand Monitor 🚗💨

[![ESP32](https://img.shields.io/badge/Hardware-ESP32%20CYD-yellow.svg)](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)
[![Engine](https://img.shields.io/badge/Engine-1.8T%2020V%20AGU-blue.svg)](#)
[![ECU](https://img.shields.io/badge/ECU-Bosch%20Motronic%20M3.8.3-orange.svg)](#)
[![Protocol](https://img.shields.io/badge/OBD-KW1281%20%2F%20K--Line-red.svg)](#)
[![IDE](https://img.shields.io/badge/IDE-Arduino%20IDE-00979D.svg)](https://www.arduino.cc/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

Ein multifunktionales digitales Zusatzinstrument für Fahrzeuge der VAG-Gruppe mit dem beliebten **1.8T 20V (AGU)** Motor und **Bosch Motronic M3.8.3** Steuergerät (z. B. VW Golf 4, Audi A3 8L, Škoda Octavia 1U). 

**Fokus auf den echten Betriebszustand & Vitalwerte des Motors:**  
Anstatt Platz für eine redundante Drehzahlanzeige zu verschwenden, überwacht dieses Display die **lebenswichtigen Drücke, Temperaturen und Spannungen** des Turbo-Triebwerks in Echtzeit.

---

## 🩺 Überwachte Motor-Betriebswerte

| Kategorie | Parameter | Geber / Sensor | Warnschwelle / Alarm |
| :--- | :--- | :--- | :--- |
| **Drücke** | **Öldruck (bar)** | 0-10 bar Geber an `GPIO 36` | Gelb < 2.0 bar / **Alarmrot < 1.2 bar** |
| | **Ladedruck (bar)** | Bosch 3-Bar MAP an `GPIO 35` | Overboost-Warnung > 1.45 bar |
| **Temperaturen** | **Öltemperatur (°C)** | NTC Sensor (Ölflansch) an `GPIO 39` | Blau < 75 °C (Kaltlauf) / **Rot > 125 °C** |
| | **Kühlwasser (°C)** | Digital via K-Line (KW1281 MWB 001) | Soll: 90 °C / **Rot > 103 °C** |
| | **Ladeluft LLK (°C)** | Schneller NTC (Druckrohr) an `GPIO 34` | Heat-Soak Warnung > 55 °C |
| | **Abgastemp EGT (°C)**| K-Typ Thermoelement & MAX31855 SPI | Gelb > 900 °C / **Alarmrot > 950 °C** |
| **Elektrik** | **Bordspannung (V)** | Digital via K-Line (KW1281 MWB 003) | Gelb/Rot < 12.0 V oder > 15.0 V |

---

## 🎨 4 Umschaltbare Display-Design Studien

Durch einfaches Antippen des Touchscreens wechselst du jederzeit nahtlos zwischen **4 spezialisierten Anzeige-Layouts**:

```
+-----------------------------------+-----------------------------------+
| 1. HEALTH GRID (6 Kacheln)        | 2. PRESSURE & TEMP (Druck/Temp)   |
| 2 Drücke (Ladedruck / Öldruck),   | 2 riesige Digitalanzeigen oben    |
| 4 Temperaturen (Öl/H2O/LLK/EGT),  | (Ladedruck/Öldruck), 4 Bargraphen |
| Status-Footer mit Bordspannung.   | für Temperaturen unten.           |
+-----------------------------------+-----------------------------------+
| 3. OEM+ GOLF 4 VDO LOOK           | 4. MONITOR MATRIX (Statusleisten) |
| Blaues Backlight mit roten VDO-   | 6 horizontale Parameterleisten    |
| Runduhren (Ladedruck & Öldruck),  | mit farbigen Alarmschwellen       |
| zentrales rotes MFA-Display.      | und Min/Max-Wertspeichern.        |
+-----------------------------------+-----------------------------------+
```

> [!TIP]
> **Interaktive Live-Vorschau:** Öffne [`design_study/index.html`](design_study/index.html) im Webbrowser, um alle 4 Themes mit Schiebereglern und Fahrzustands-Presets (Kaltstart, Leerlauf, Volllast, Überhitzung) auszuprobieren!

---

## 🚀 Features & Sensor-Management

- **Öldrucküberwachung mit Min-Hold:**
  - Erfasst den minimalen Öldruck bei betriebswarmem Motor (wichtig zur Erkennung von verschlissenen Pleuellagern oder verstopftem Ölsieb).
- **Öltemperatur mit Kaltlauf-Erkennung:**
  - Solange das Öl unter 75 °C hat, wird der Wert zur Schonung des Motors blau dargestellt. Ab 125 °C warnt das Display vor Ölfilmabriss.
- **Ladedruck mit Auto-Zeroing:**
  - Beim Einschalten der Zündung wird der barometrische Umgebungsdruck automatisch genullt, um unabhängig von Wetter und Meereshöhe exakt 0,00 bar Relativdruck anzuzeigen.
- **Ladelufttemperatur (Charge Air Temp nach LLK):**
  - Zeigt exakt an, wie effizient der Ladeluftkühler arbeitet und warnt vor Leistungsverlust durch Heat-Soak im Sommer oder Stau.
- **K-Line / KW1281 Protokoll:**
  - Fragt ohne zusätzliche Geber Kühlwassertemperatur und Bordspannung direkt digital aus dem Motronic M3.8.3 Steuergerät ab.

---

## 📁 Projektstruktur

```
Motronic-OBD-Gauge/
├── AGU_ESP32_Gauge/
│   ├── AGU_ESP32_Gauge.ino       # Hauptsketch (FreeRTOS / Non-blocking)
│   ├── Config.h                  # Pins, Alarmschwellen & Sensor-Kalibrierung
│   ├── Sensors.h & .cpp          # Ladedruck, Öldruck, Öltemp, Ladeluft, EGT & Filter
│   ├── KW1281.h & .cpp           # K-Line Diagnoseprotokoll für Kühlwasser & Batt V
│   ├── Simulator.h & .cpp        # Realistischer Motor- und Fahrzyklus-Simulator
│   └── DisplayUI.h & .cpp        # 4 Themes fokussiert auf Motor-Betriebszustand
├── tools/
│   └── ecu_simulator.py          # Python Simulator für K-Line Tests am Schreibtisch
├── docs/
│   ├── SCHALTPLAN.md             # Vollständiger Anschlussplan (alle 4 ADC1 Pins)
│   └── TESTUMGEBUNG.md           # Anleitung für Simulation & Bench-Testing
├── design_study/
│   └── index.html                # Interaktive Web-Designstudie & Sensor-Controller
├── .gitignore
├── LICENSE
└── README.md
```

---

## 🛠 Anschlussbelegung (ESP32 CYD)

Alle analogen Geber nutzen rauschfreie **ADC1-Pins** des ESP32:

| Sensor / Geber | ESP32 Pin | Beschreibung |
| :--- | :--- | :--- |
| **Ladedruck (MAP)** | `GPIO 35` | 0.5V - 4.5V über Spannungsteiler 5V -> 3.0V |
| **Öldruck (0-10 bar)** | `GPIO 36` | 0.5V - 4.5V über Spannungsteiler 5V -> 3.0V |
| **Öltemperatur** | `GPIO 39` | NTC Geber am Ölfilterflansch (2.2 kΩ Pullup an 3.3V) |
| **Ladeluft (LLK)** | `GPIO 34` | Schneller NTC vor Drosselklappe (2.2 kΩ Pullup an 3.3V) |
| **Abgastemperatur (EGT)** | `GPIO 21, 22, 27`| SPI Digital (MAX31855 Modul mit K-Typ Thermoelement) |
| **Kühlwasser & Spannung** | `GPIO 16, 17` | L9637D Transceiver an K-Line Pin 7 der OBD-2 Buchse |

> [!NOTE]
> Für Details siehe den [Schaltplan & Verdrahtungsplan](docs/SCHALTPLAN.md).

---

## 📄 Lizenz

Dieses Projekt ist unter der [MIT License](LICENSE) lizenziert.
