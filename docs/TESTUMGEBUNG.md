# 🧪 Testumgebung & Simulation (Desktop & Bench-Testing)

Um das Zusatzinstrument ohne angeschlossenes Fahrzeug und ohne externe Sensoren auf der Werkbank oder am Schreibtisch vollständig testen zu können, stehen **drei mächtige Test- und Simulationswerkzeuge** zur Verfügung:

---

## 1. Integrierter Firmware-Simulator (`ENABLE_SIMULATION`)

Die ESP32-Firmware enthält eine vollständige physikalische Motor- und Fahrzyklus-Simulation.

### Aktivierung
In [Config.h](file:///d:/Claude/Motronic-OBD-Gauge/AGU_ESP32_Gauge/Config.h#L11):
```cpp
// Auf 'true' setzen für Bench-Testing ohne Sensoren / Fahrzeug
#define ENABLE_SIMULATION true
```

### Was wird simuliert?
Ein realistischer dynamischer 1.8T Fahrzyklus mit kontinuierlichen Übergängen:
1. **Standgas (Idle):** 840 U/min, -0.68 bar Saugrohrunterdruck, EGT kühlt auf 420 °C ab, Kühlwasser 90 °C.
2. **Cruisen (Teillast):** 2.200 U/min, leichter Teillast-Druck (-0.25 bar), EGT 580 °C.
3. **Ladedruckaufbau (Spool-Up):** 2.500 bis 4.500 U/min, Ladedruck steigt rasch auf 1.25 bar, EGT steigt auf 780 °C.
4. **Volllast (WOT / Highway Pull):** Drehzahl bis 6.500 U/min, Overboost 1.38 bar, EGT klettert auf 925 °C (**aktiviert die dynamische Rot-Warnung**), Ladelufttemperatur steigt auf 52 °C (Heat-Soak).
5. **Gangwechsel (Gear Shift):** Drehzahl fällt auf 4.300 U/min, Ladedruck bricht auf -0.30 bar ein (Blow-Off / Schubumluftventil).
6. **Schubabschaltung (Overrun):** Motorbremse, Einspritzzeit 0.0 ms, starker Unterdruck (-0.78 bar).

---

## 2. Python ECU Simulator (`tools/ecu_simulator.py`)

Wenn du die K-Line Hardware (Transceiver L9637D oder USB-K-Line-Kabel) und den echten KW1281-Protokollstack am PC testen willst:

### Voraussetzungen
```bash
pip install pyserial
```

### Ausführung
Verbinde einen USB-zu-Serial-Adapter (oder ein FTDI K-Line Kabel) mit deinem PC und dem ESP32 (RX an TX, TX an RX):
```bash
python tools/ecu_simulator.py --port COM3 --baud 9600
```

### Funktionsweise des Simulators:
- Erkennt den 5-Baud Slow-Init Impuls (`0x01`).
- Sendet Sync-Byte `0x55` und Keybytes `0x01, 0x8A`.
- Bestätigt das invertierte Handshake-Byte (`0x75`).
- Sendet den authentischen Bosch Motronic M3.8.3 ECU-Identifikationsstring:  
  `06A906018R  1.8L R4/5VT       MOTRONIC M3.8.3`
- Antwortet fortlaufend auf Messwertblock-Anfragen (MWB 001, 002, 003) mit live generierten Werten.

---

## 3. Interaktive Web-Designstudie (`design_study/index.html`)

Ein interaktives HTML5 Canvas Interface, das direkt im Webbrowser geöffnet werden kann.

### Starten
Öffne die Datei [design_study/index.html](file:///d:/Claude/Motronic-OBD-Gauge/design_study/index.html) in einem beliebigen Browser (Chrome, Firefox, Edge).

### Features:
- **Alle 4 Display-Themes** (Classic Quad, Sport Cluster, OEM+ Golf 4, Data Matrix) in voller 640x480 Auflösung (2x Skalierung des CYD 320x240 Displays).
- **Interaktive Regler:** Ladedruck (-1.0 bis +2.0 bar), EGT (300 bis 1050 °C), Ladeluft (15 bis 85 °C), Drehzahl (800 bis 7000 RPM) und Kühlwasser.
- **1-Klick Fahrzustands-Presets:** Standgas, Cruisen, Volllast, Alarmzustand (Overboost / EGT-Alarm).
- **Touch-Simulation:** Durch Klick auf das virtuelle Display wird direkt das nächste Layout durchgewechselt – genau wie beim echten Touchscreen des ESP32 CYD!
