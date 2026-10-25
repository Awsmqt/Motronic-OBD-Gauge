# Testumgebung und Simulation

## Firmware-Simulator

Für einen Display- und Logger-Test ohne angeschlossene Sensoren oder Fahrzeug kann `ENABLE_SIMULATION` in `AGU_ESP32_Gauge/Config.h` auf `true` gesetzt werden. Die Simulation durchläuft Kaltstart, Teillast, Ladedruck, hohe Last und heißen Leerlauf. In den Lastphasen werden zusätzlich zwei DTCs erzeugt, damit die Diagnosefunktionen des Web-Dashboards geprüft werden können.

Nach dem Test `ENABLE_SIMULATION` wieder auf `false` setzen, bevor die Firmware im Fahrzeug verwendet wird.

## Python-K-Line-Simulator

Der Simulator unter `tools/ecu_simulator.py` testet den KW1281-Stack über eine serielle Verbindung. Er benötigt Python und `pyserial`:

```sh
python -m pip install pyserial
python tools/ecu_simulator.py --port COM3 --baud 9600
```

Unter Linux/macOS kann `--port` beispielsweise `/dev/ttyUSB0` sein. Verbinde den seriellen Adapter entsprechend der Testhardware mit dem ESP32-Transceiver; der Simulator emuliert die ECU-Seite.

Der Simulator verarbeitet den 5-Baud-Start, sendet Sync- und Keybytes sowie eine ECU-ID und beantwortet Anfragen für Messwertblöcke 1 bis 6 und DTCs.

## Display-Bedienung

Das Gerät startet in der Ansicht **Messwerte**. Kurzes Antippen wechselt zwischen **Messwerte** und **Motorzustand**; langes Drücken setzt die Peak- und Minimalwerte zurück. Die gleiche Auswahl ist im Web-Dashboard unter **Display** verfügbar.
