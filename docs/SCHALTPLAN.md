# 🔌 Schaltplan & Sensor-Anschlussplan (ESP32 CYD Motor-Monitor)

Dieses Zusatzinstrument überwacht die **kritischen Vitalwerte des VW 1.8T AGU** (Öldruck, Öltemperatur, Kühlwasser, Ladelufttemperatur, Ladedruck, Abgastemperatur EGT und Bordspannung).

---

## 📌 Übersicht aller Sensor-Pins (ESP32 CYD)

Alle 4 analogen Sensoren nutzen die rauscharmen **ADC1-Eingänge** des ESP32:

| Messwert / Sensor | Signalart / Geber | ESP32 GPIO | Schnittstelle / Pegelanpassung |
| :--- | :--- | :--- | :--- |
| **Ladedruck** | 0.5V - 4.5V (3-Bar MAP Bosch) | `GPIO 35` | ADC1_CH7 über Spannungsteiler 5V -> 3.0V |
| **Öldruck** | 0.5V - 4.5V (0-10 bar Geber) | `GPIO 36` | ADC1_CH0 (Sensor VP) über Spannungsteiler 5V -> 3.0V |
| **Öltemperatur** | NTC/VDO Sensor (M10x1 am Ölflansch) | `GPIO 39` | ADC1_CH3 (Sensor VN) mit 2.2 kΩ Pullup zu 3.3V |
| **Ladeluft (LLK)** | Schneller NTC (z. B. Bosch M12) | `GPIO 34` | ADC1_CH6 mit 2.2 kΩ Pullup zu 3.3V |
| **Abgastemperatur** | K-Typ Thermoelement (-50 bis 1100°C)| `GPIO 21, 22, 27`| SPI Digital (Adafruit MAX31855 Modul) |
| **Kühlwasser & Batt**| Digital via K-Line (KW1281 OBD) | `GPIO 16, 17` | Hardware Serial2 über L9637D Transceiver |

---

## 1. Ladedruck-Sensor (Bosch 3-Bar / 4-Bar MAP)
```
                      +5V (vom Step-Down Wandler)
                       |
               +-------+-------+
               |  MAP-Sensor   |
               | (Bosch 3-Bar) |
               +-------+-------+
                       | Signal (0.5V - 4.5V)
                       |
                      [R1: 2.2 kΩ]
                       |
                       +------------> GPIO 35 (ESP32 ADC1, max. 2.7V)
                       |
                      [R2: 3.3 kΩ]
                       |
                      GND
```

---

## 2. Öldruck-Sensor (0-10 Bar Druckwandler am Ölfilterhalter)
Ein 0-10 Bar Druckgeber mit 0.5V - 4.5V Ausgang (oder VDO Geber mit Vorwiderstand):
```
                      +5V (Versorgung)
                       |
               +-------+-------+
               | Öldruckgeber  | (z. B. M10x1 am Ölfilterflansch)
               +-------+-------+
                       | Signal (0.5V - 4.5V)
                       |
                      [R1: 2.2 kΩ]
                       |
                       +------------> GPIO 36 (ESP32 Sensor VP, max. 2.7V)
                       |
                      [R2: 3.3 kΩ]
                       |
                      GND
```

---

## 3. Öltemperatur & Ladelufttemperatur (NTC Sensoren)
Beide NTC-Sensoren arbeiten nach dem gleichen präzisen Messprinzip:
```
                  +3.3V (ESP32 3.3V Pin)
                    |
                 [2.2 kΩ Vorwiderstand 1%]
                    |
                    +------------> GPIO 39 (Öltemperatur) bzw. GPIO 34 (Ladeluft)
                    |
              +-----+-----+
              |    NTC    | (Öl: M10x1 Ölflansch / LLK: Druckrohr vor Drosselklappe)
              |  Sensor   |
              +-----+-----+
                    |
                   GND
```

---

## 4. Abgastemperatur EGT (Adafruit MAX31855 SPI)
```
  K-Typ Thermoelement          MAX31855 Modul             ESP32 CYD
  +------------------+         +--------------+          +-----------+
  | Gelb (+) --------|-------->| T+       VCC |<---------| 3.3V      |
  | Rot  (-) --------|-------->| T-       GND |<---------| GND       |
  +------------------+         |           DO |--------->| GPIO 22   |
                               |           CS |<---------| GPIO 21   |
                               |          CLK |<---------| GPIO 27   |
                               +--------------+          +-----------+
```

---

## 5. K-Line / KW1281 OBD-Anbindung (L9637D Transceiver)
Kühlwassertemperatur und Bordspannung werden direkt digital aus dem Motronic M3.8.3 Steuergerät gelesen:
```
   OBD-2 Buchse (Fahrzeug)             L9637D Transceiver         ESP32 CYD
  +-----------------------+           +------------------+       +-----------+
  | Pin 16 (Dauerplus 12V)|---------->| VCC (12V)     TX |<------| GPIO 17   |
  | Pin 4/5 (Masse / GND) |---------->| GND           RX |------>| GPIO 16   |
  | Pin 7 (K-Line)        |<--------->| K             VS |<------| 3.3V      |
  +-----------------------+           +------------------+       +-----------+
```
