# 🔌 Schaltplan & Verdrahtungsanleitung (ESP32 CYD Multi-Gauge)

Diese Dokumentation beschreibt die genaue elektrische Verschaltung des **ESP32 CYD ("Cheap Yellow Display" / ESP32-2432S028)** mit den externen Sensoren und dem Fahrzeug (VW 1.8T AGU / Bosch Motronic M3.8.3).

---

## ⚠️ Wichtige Sicherheits- & Pegelhinweise

1. **ESP32 Spannungspegel (3.3V Max!):**  
   Die GPIO-Pins und internen ADCs des ESP32 dürfen **niemals mit mehr als 3.3V** beaufschlagt werden. 5V-Sensoren (wie der Bosch MAP-Sensor) müssen über einen präzisen Spannungsteiler heruntergeteilt werden.
2. **Kfz-Bordnetz (12V - 14.4V):**  
   Das Kfz-Bordnetz hat Spannungsspitzen (Load Dumps). Verwende für die 5V-Versorgung des ESP32 CYD zwingend einen hochwertigen, gefilterten Step-Down-Wandler (z. B. LM2596 mit Schutzdiode oder Kfz-USB-Wandler) und sichere die Zuleitung mit einer 1A- bis 2A-Sicherung ab.
3. **Massebezug (Common Ground):**  
   Fahrzeugmasse (OBD Pin 4/5), Sensormasse und ESP32-GND müssen miteinander verbunden sein!

---

## 📌 Pinbelegung ESP32 CYD (CN1 / P3 Stecker)

| Signal / Sensor | ESP32 GPIO | Beschreibung / Pegel |
| :--- | :--- | :--- |
| **Ladedruck (MAP-Signal)** | `GPIO 35` | Analogeingang ADC1_CH7 (0 - 3.0V vom Spannungsteiler) |
| **Ladeluft-NTC (LLK IAT)** | `GPIO 34` | Analogeingang ADC1_CH6 (mit 2.2k Pullup zu 3.3V) |
| **MAX31855 CS** | `GPIO 21` | SPI Chip Select (Digital Out) |
| **MAX31855 DO / MISO** | `GPIO 22` | SPI Data Out (Digital In) |
| **MAX31855 CLK** | `GPIO 27` | SPI Clock (Digital Out) |
| **K-Line RX** | `GPIO 16` | Serial2 RX vom L9637D Transceiver (3.3V Logik) |
| **K-Line TX** | `GPIO 17` | Serial2 TX zum L9637D Transceiver (3.3V Logik) |
| **Display TFT & Touch** | *intern* | Integriert auf CYD Platine (ILI9341 & XPT2046) |

---

## 1. Ladedruck-Sensor (Bosch 3-Bar / 4-Bar MAP)

Ein externer Drucksensor (z. B. Bosch 0 281 002 437) liefert bei 5V Versorgung ein Ausgangssignal von 0.5V (Vakuum) bis 4.5V (3 Bar absolut).

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
                       +------------> GPIO 35 (ESP32 ADC, max. 2.7V)
                       |
                      [R2: 3.3 kΩ]
                       |
                      GND
```

*Spannungsteiler-Faktor:*  
$$V_{\text{out}} = V_{\text{in}} \cdot \frac{3.3\,\text{k}\Omega}{2.2\,\text{k}\Omega + 3.3\,\text{k}\Omega} = V_{\text{in}} \cdot 0.60$$  
Bei 4.5V Sensorspannung liegen am ESP32 genau $4.5\,\text{V} \cdot 0.60 = 2.70\,\text{V}$ an – perfekt im sicheren linearen Bereich des ESP32 ADCs!

---

## 2. Ladeluft-Temperatursensor (NTC nach LLK)

Für eine schnelle und exakte Erfassung der Ladelufttemperatur nach dem Ladeluftkühler wird ein schneller offener NTC (z. B. Bosch NTC M12 oder 2.2k / 10k NTC) in die Ladeluftverrohrung vor der Drosselklappe gesetzt.

```
                  +3.3V (ESP32 3.3V Pin)
                    |
                 [2.2 kΩ Vorwiderstand 1%]
                    |
                    +------------> GPIO 34 (ESP32 ADC)
                    |
              +-----+-----+
              |  NTC M12  |
              |  Sensor   |
              +-----+-----+
                    |
                   GND
```

---

## 3. Abgastemperatur EGT (Adafruit MAX31855 & K-Typ Thermoelement)

Das Thermoelement wird in den Abgaskrümmer (Sammler vor Turbolader) oder in die Downpipe direkt hinter die Turbine eingeschweißt (M10x1 Gewinde).

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

## 4. K-Line / KW1281 OBD-Anbindung (L9637D Transceiver)

Der L9637D ISO-Transceiver wandelt die 12V-Signalpegel der K-Line (OBD-Buchse Pin 7) in saubere 3.3V UART-Signale für den ESP32 um.

```
   OBD-2 Buchse (Fahrzeug)             L9637D Transceiver         ESP32 CYD
  +-----------------------+           +------------------+       +-----------+
  | Pin 16 (Dauerplus 12V)|---------->| VCC (12V)     TX |<------| GPIO 17   |
  | Pin 4/5 (Masse / GND) |---------->| GND           RX |------>| GPIO 16   |
  | Pin 7 (K-Line)        |<--------->| K             VS |<------| 3.3V      |
  +-----------------------+           +------------------+       +-----------+
```
