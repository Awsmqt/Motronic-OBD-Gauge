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
| **Drücke** | **Öldruck (bar)** | 0-10 bar Geber an `GPIO 36` | Warnung < 1.2 bar / **kritisch < 0.8 bar** |
| | **Ladedruck (bar)** | Bosch 3-Bar MAP an `GPIO 35` | Overboost-Warnung > 1.45 bar |
| **Temperaturen** | **Öltemperatur (°C)** | NTC Sensor (Ölflansch) an `GPIO 39` | Blau < 75 °C (Kaltlauf) / Warnung > 120 °C / **kritisch > 135 °C** |
| | **Kühlwasser (°C)** | Digital via K-Line (KW1281 MWB 001) | Soll: 90 °C / Warnung > 103 °C / **kritisch > 112 °C** |
| | **Ladeluft LLK (°C)** | Schneller NTC (Druckrohr) an `GPIO 34` | Heat-Soak Warnung > 55 °C |
| | **Abgastemp EGT (°C)**| K-Typ Thermoelement & MAX31855 SPI | Warnung > 900 °C / **kritisch > 950 °C** |
| **Elektrik** | **Bordspannung (V)** | Digital via K-Line (KW1281 MWB 003) | Gelb/Rot < 12.0 V oder > 15.0 V |

---

## 🎨 Zwei minimalistische Display-Ansichten

Ein kurzes Tippen wechselt zwischen genau zwei Ansichten; langes Drücken setzt die Peak- und Minimalwerte zurück:

1. **Messwerte:** Acht kompakte Werte für Drücke, Temperaturen, Lambda und Drehzahl; Bordspannung und ECU-Verbindung stehen im Footer.
2. **Motorzustand:** Sechs farbcodierte Statusbalken und eine Gesamtanzeige für Warnung oder kritischen Zustand.

---

## 📱 iPhone & Smartphone Web-Dashboard (WiFi)

Das Display erzeugt einen eigenen automotiven **WiFi Access-Point**, sodass du während der Fahrt oder bei der Abstimmung alle Daten direkt auf deinem iPhone in Safari abrufen kannst:

- **Netzwerk:** `VW-1.8T-AGU-Gauge` (Passwort: `motronic18t`)
- **URL in Safari:** `http://192.168.4.1/`
- **Features:**
  - Vier Tabs: **Live**, **Graphen**, **Fehler**, **Display**
  - 4 Hz Live-Telemetrie (Ladedruck, Öldruck, Temperaturen, EGT, Lambda, Zündung, Klopfen)
  - **Graphen:** Verlauf von Temperaturen, EGT, Drücken und Bordspannung (1/5/15 min, Wischen zeigt Werte), Export als CSV
  - **Tuner-Aufzeichnung:** Start/Stopp-Logging mit 23 Spalten (Drehzahl, Last, Lambda, Zündwinkel, Klopfen, Ladedruck u.v.m.) als CSV für Chiptuner, wahlweise Standard (Komma/Punkt) oder Excel DE
  - Fehlercodes (DTCs) direkt vom Handy auslesen und mit einem Klick löschen – **mit Klartext-Beschreibung und Prüfhinweisen** (z.B. `P0300 – Zündaussetzer erkannt`)
  - Peak-Werte (Max Boost, Max EGT, Min Öldruck) per Button zurücksetzen
  - Zwischen den zwei Display-Ansichten wählen
  - Unterstützt Apples **"Zum Home-Bildschirm hinzufügen"** (startet im Vollbild ohne Safari-Adressleiste)
  - Funktioniert überall ohne Internet / ohne App-Installation

---

## 💾 SD-Karten Telemetrie-Logger (CSV)

Der eingebaute SD-Kartenslot des CYD zeichnet alle Motordaten vollautomatisch auf:

- **Format:** Standard-CSV (`/logs/AGU_session_NNNN.csv`) mit 18 Spalten
- **Aufgezeichnete Daten:** Zeitstempel (ms), Ladedruck, Öldruck, Öltemp, Kühlwasser, Ladeluft, EGT, Bordspannung, Ansaugluft ECU, Last %, Luftmasse (MAF g/s), Drehzahl, Lambda, Einspritzzeit (ms), Zündwinkel, Klopfrücknahme, Drosselklappe %, Alarm-Bitmaske
- **SD-Schonung:** 10 Zeilen RAM-Pufferung vor jedem Schreibvorgang
- **Sicherheit:** Automatische Session-Rotation bei 10 MB Dateigröße, Ringspeicher bis 50 Sessions

---

## 🚀 Features & Sensor-Management

- **Öldrucküberwachung mit Min-Hold:**
  - Erfasst den minimalen Öldruck bei betriebswarmem Motor (wichtig zur Erkennung von verschlissenen Pleuellagern oder verstopftem Ölsieb).
- **Öltemperatur mit Kaltlauf-Erkennung:**
  - Solange das Öl unter 75 °C hat, wird der Wert zur Schonung des Motors blau dargestellt. Ab 120 °C warnt das Display; ab 135 °C ist der Zustand kritisch.
- **Ladedruck mit Auto-Zeroing:**
  - Beim Einschalten der Zündung wird der barometrische Umgebungsdruck automatisch genullt, um unabhängig von Wetter und Meereshöhe exakt 0,00 bar Relativdruck anzuzeigen.
- **Ladelufttemperatur (Charge Air Temp nach LLK):**
  - Zeigt exakt an, wie effizient der Ladeluftkühler arbeitet und warnt vor Leistungsverlust durch Heat-Soak im Sommer oder Stau.
- **K-Line / KW1281 Protokoll:**
  - Fragt ohne zusätzliche Geber Kühlwassertemperatur, Bordspannung, Lambda, Zündwinkel, Einspritzung und Fehlerspeicher (DTC) direkt digital aus dem Motronic M3.8.3 Steuergerät ab.

---

## 📁 Projektstruktur

```
Motronic-OBD-Gauge/
├── AGU_ESP32_Gauge/
│   ├── AGU_ESP32_Gauge.ino       # Hauptsketch mit zeitgesteuerter Loop
│   ├── Config.h                  # Pins, Alarmschwellen, WiFi- & Logger-Parameter
│   ├── Sensors.h & .cpp          # Ladedruck, Öldruck, Öltemp, Ladeluft, EGT & Filter
│   ├── KW1281.h & .cpp           # K-Line Diagnoseprotokoll (MWB 1-6 + DTCs lesen/löschen)
│   ├── DtcDatabase.h & .cpp      # Fehlercode-Datenbank (VAG-Code → P-Code, Klartext, Ursachen)
│   ├── Simulator.h & .cpp        # Realistischer Motor- und Fahrzyklus-Simulator
│   ├── DisplayUI.h & .cpp        # Zwei minimalistische Display-Ansichten
│   ├── Logger.h & .cpp           # SD-Karten CSV Datenlogger (HSPI, 18 Parameter)
│   └── WebDashboard.h & .cpp     # WiFi Access-Point & iPhone Web-Dashboard
├── tools/
│   └── ecu_simulator.py          # Python Simulator für K-Line Tests am Schreibtisch
├── docs/
│   ├── SCHALTPLAN.md             # Vollständiger Anschlussplan (alle 4 ADC1 Pins)
│   └── TESTUMGEBUNG.md           # Anleitung für Simulation & Bench-Testing
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
