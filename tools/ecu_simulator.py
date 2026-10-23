#!/usr/bin/env python3
"""
==============================================================================
Bosch Motronic M3.8.3 (VW 1.8T AGU) - KW1281 ECU Simulator
==============================================================================
Dieser Simulator emuliert das Motorsteuergerät am PC über einen USB-Seriell-
oder K-Line-Adapter (z.B. FT232R, CH340). Damit lässt sich die ESP32 CYD
Firmware und Hardware-Transceiver vollständig auf der Werkbank testen!

Verwendung:
    pip install pyserial
    python ecu_simulator.py --port COM3 --baud 9600
==============================================================================
"""

import sys
import time
import argparse
import math

try:
    import serial
except ImportError:
    print("Fehler: pyserial ist nicht installiert. Bitte 'pip install pyserial' ausführen.")
    sys.exit(1)

class MotronicM383Simulator:
    def __init__(self, port, baud=9600):
        self.port = port
        self.baud = baud
        self.ser = None
        self.block_counter = 0
        self.connected = False
        self.rpm = 840
        self.coolant = 90.0
        self.iat = 28.0
        self.battery = 13.8
        self.maf = 3.2
        self.load = 18.0
        self.ignition = 12.0
        self.step = 0

    def open(self):
        print(f"[*] Öffne seriellen Port {self.port} mit {self.baud} Baud...")
        self.ser = serial.Serial(
            port=self.port,
            baudrate=self.baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=1.0
        )
        print("[+] Port geöffnet. Warte auf 5-Baud Slow-Init oder Sync...")

    def read_byte_with_echo(self, timeout=2.0):
        self.ser.timeout = timeout
        b = self.ser.read(1)
        if len(b) == 0:
            return None
        return b[0]

    def send_byte(self, val):
        self.ser.write(bytes([val & 0xFF]))
        self.ser.flush()

    def update_engine_dynamics(self):
        """Simuliert Beschleunigung und Drehzahl-Schwankungen"""
        self.step += 1
        t = self.step * 0.1
        # Sinus-basierte Drehzahlkurve für realistische Dynamik
        self.rpm = int(1800 + 1500 * math.sin(t * 0.5) + 1200 * math.sin(t * 0.2))
        if self.rpm < 840: self.rpm = 840
        if self.rpm > 6500: self.rpm = 6500

        # Abhängige Motordaten
        self.load = (self.rpm / 6500.0) * 120.0 + 15.0
        self.maf = (self.rpm / 6500.0) * 160.0 + 3.0
        self.coolant = 90.0 + math.sin(t * 0.05) * 2.0
        self.iat = 28.0 + (self.rpm / 6500.0) * 22.0
        self.battery = 13.8 + math.sin(t * 0.1) * 0.2

    def run_handshake(self):
        """Führt den KW1281 Handshake durch"""
        print("[*] Sende Sync-Byte 0x55...")
        time.sleep(0.1)
        self.send_byte(0x55)
        time.sleep(0.05)

        # Keybytes senden: 0x01 (Keybyte 1), 0x8A (Keybyte 2)
        print("[*] Sende Keybytes 0x01, 0x8A...")
        self.send_byte(0x01)
        time.sleep(0.02)
        self.send_byte(0x8A)

        # Warte auf invertiertes Keybyte 2 (~0x8A = 0x75)
        print("[*] Warte auf invertiertes Keybyte (~0x8A = 0x75)...")
        resp = self.read_byte_with_echo(timeout=2.0)
        if resp is not None and resp == (~0x8A & 0xFF):
            print(f"[+] Gültige Quittung empfangen: 0x{resp:02X}!")
            time.sleep(0.05)
            self.send_ecu_id()
            self.connected = True
            return True
        else:
            print(f"[-] Ungültige oder fehlende Antwort: {resp}")
            return False

    def send_ecu_id(self):
        """Sendet Motronic M3.8.3 Steuergeräte-Identifikation"""
        id_str = "06A906018R  1.8L R4/5VT       MOTRONIC M3.8.3  "
        print(f"[*] Sende ECU-ID: {id_str.strip()}")
        data = id_str.encode('ascii')
        self.send_block(data, 0xF6)

    def send_block(self, data, title):
        """Sendet einen KW1281 Block mit Komplement-Quittung für jedes Byte"""
        total_len = len(data) + 3
        self.block_counter = (self.block_counter + 1) & 0xFF

        # 1. Blocklänge
        self.send_byte(total_len)
        ack = self.read_byte_with_echo(1.0) # Tester quittiert mit ~total_len

        # 2. Block-Zähler
        self.send_byte(self.block_counter)
        ack = self.read_byte_with_echo(1.0)

        # 3. Titel
        self.send_byte(title)
        ack = self.read_byte_with_echo(1.0)

        # 4. Daten
        for b in data:
            self.send_byte(b)
            ack = self.read_byte_with_echo(1.0)

        # 5. Block-Ende (0x03)
        self.send_byte(0x03)

    def handle_group_request(self, group_num):
        """Generiert die 4 Messwert-Felder für den angeforderten Block"""
        self.update_engine_dynamics()
        print(f"[->] Messwertblock MWB {group_num:03d} angefordert (RPM={self.rpm}, Kühlwasser={self.coolant:.0f}°C, Last={self.load:.0f}%)")

        data = bytearray()
        if group_num == 1:
            # MWB 001:
            # Feld 1: Drehzahl (Formel 01: 0.2 * a * b)
            rpm_div = int(self.rpm / 0.2)
            a_rpm = int(rpm_div / 250)
            b_rpm = int(rpm_div / (a_rpm if a_rpm > 0 else 1))
            data.extend([0x01, a_rpm & 0xFF, b_rpm & 0xFF])

            # Feld 2: Kühlwasser (Formel 05: a - 50 °C)
            a_cool = int(self.coolant + 50.0)
            data.extend([0x05, a_cool & 0xFF, 0x00])

            # Feld 3: Lambdaspannung (Formel 21: 0.001 * a * b)
            data.extend([0x21, 0x1E, 0x14]) # ca. 0.6V

            # Feld 4: Betriebszustand (Formel 00)
            data.extend([0x00, 0x01, 0x00])

        elif group_num == 2:
            # MWB 002: Drehzahl, Last %, Einspritzzeit ms, Luftmasse MAF g/s
            rpm_div = int(self.rpm / 0.2)
            a_rpm = int(rpm_div / 250)
            b_rpm = int(rpm_div / (a_rpm if a_rpm > 0 else 1))
            data.extend([0x01, a_rpm & 0xFF, b_rpm & 0xFF])

            # Last (Formel 20: a * 0.392)
            a_load = int(self.load / 0.392)
            data.extend([0x20, a_load & 0xFF, 0x00])

            # Einspritzzeit (Formel 25: a * 0.05 ms)
            inj_ms = 2.0 + (self.load / 120.0) * 12.0
            a_inj = int(inj_ms / 0.05)
            data.extend([0x25, a_inj & 0xFF, 0x00])

            # Luftmasse (Formel 20: a * 0.78125 g/s)
            a_maf = int(self.maf / 0.78125)
            data.extend([0x20, a_maf & 0xFF, 0x00])

        elif group_num == 3:
            # MWB 003: Drehzahl, Bordspannung, Kühlwasser, Ansaugluft (IAT)
            rpm_div = int(self.rpm / 0.2)
            a_rpm = int(rpm_div / 250)
            b_rpm = int(rpm_div / (a_rpm if a_rpm > 0 else 1))
            data.extend([0x01, a_rpm & 0xFF, b_rpm & 0xFF])

            # Bordspannung (Formel 21: 0.001 * a * b => a=138, b=100 => 13.8V)
            a_volt = int(self.battery * 10.0)
            data.extend([0x21, a_volt & 0xFF, 100])

            # Kühlwasser (Formel 05: a - 50 °C)
            a_cool = int(self.coolant + 50.0)
            data.extend([0x05, a_cool & 0xFF, 0x00])

            # IAT Ansaugluft (Formel 05: a - 50 °C)
            a_iat = int(self.iat + 50.0)
            data.extend([0x05, a_iat & 0xFF, 0x00])

        elif group_num == 4:
            # MWB 004: Drehzahl, Lambda-Wert, Drosselklappe, Zündwinkel
            rpm_div = int(self.rpm / 0.2)
            data.extend([0x01, int(rpm_div / 250) & 0xFF, 250])
            # Lambda (Formel 31: b / 2560 * a => a=128, b=20 => ~1.000)
            data.extend([0x1F, 128, 20])
            # Drosselklappe (Formel 6: a * 0.001 * b * 100)
            data.extend([0x06, 25, 10])
            # Zündwinkel (Formel 3: a * 0.04 * b)
            data.extend([0x03, 10, 30])

        elif group_num == 5:
            # MWB 005: Drehzahl, Last, Geschwindigkeit, Einspritzzeit
            rpm_div = int(self.rpm / 0.2)
            data.extend([0x01, int(rpm_div / 250) & 0xFF, 250])
            data.extend([0x20, int(self.load / 0.392) & 0xFF, 0x00])
            data.extend([0x07, 60, 100]) # 60 km/h
            data.extend([0x0F, 40, 10])  # Einspritzzeit 4.0ms

        elif group_num == 6:
            # MWB 006: Drehzahl, Last, IAT, Klopfrücknahme
            rpm_div = int(self.rpm / 0.2)
            data.extend([0x01, int(rpm_div / 250) & 0xFF, 250])
            data.extend([0x20, int(self.load / 0.392) & 0xFF, 0x00])
            data.extend([0x05, int(self.iat + 50.0) & 0xFF, 0x00])
            # Klopfrücknahme (Formel 25: (b - 128) * 0.01 * a)
            data.extend([0x19, 100, 128]) # 0.0°

        else:
            # Fallback leere Gruppe
            data.extend([0x00, 0x00, 0x00] * 4)

        self.send_block(data, 0xE7) # 0xE7 = Group data response

    def handle_dtc_request(self):
        """Simuliert Antwort auf Fehlercode-Abfrage (Titel 0x07)"""
        print("[*] Tester fordert DTCs an (Titel 0x07)...")
        # Sende 2 simulierte DTCs als VAG-Dezimalcode (P-Nummer + 16384):
        # 17476 = 0x4444 = P1092 (sporadisch 0x20), 16949 = 0x4235 = P0565 (statisch 0x60)
        dtc_data = bytearray([
            0x44, 0x44, 0x20,  # DTC 1: P1092 sporadisch
            0x42, 0x35, 0x60   # DTC 2: P0565 statisch
        ])
        self.send_block(dtc_data, 0xFC) # 0xFC = DTC response

    def handle_dtc_clear(self):
        """Simuliert Bestätigung auf Fehlerspeicher löschen (Titel 0x05)"""
        print("[*] Tester löscht Fehlerspeicher (Titel 0x05)...")
        self.send_block(b'', 0x09) # 0x09 = ACK

    def loop(self):
        """Hauptschleife des Simulators"""
        while True:
            if not self.connected:
                # Prüfe auf eingehende Daten oder 5-Baud Pulse
                b = self.read_byte_with_echo(timeout=0.5)
                if b is not None:
                    print(f"[*] Byte empfangen: 0x{b:02X} - starte Handshake...")
                    self.run_handshake()
            else:
                # Warten auf Block vom ESP32 Tester
                len_byte = self.read_byte_with_echo(timeout=2.0)
                if len_byte is None:
                    print("[-] Timeout - keine Tester-Nachricht empfangen. Verbindung getrennt.")
                    self.connected = False
                    continue

                self.send_byte(~len_byte & 0xFF) # Quittung

                counter = self.read_byte_with_echo(0.5)
                if counter is None: self.connected = False; continue
                self.send_byte(~counter & 0xFF)

                title = self.read_byte_with_echo(0.5)
                if title is None: self.connected = False; continue
                self.send_byte(~title & 0xFF)

                data_len = max(0, len_byte - 3)
                data = bytearray()
                for _ in range(data_len):
                    d = self.read_byte_with_echo(0.5)
                    self.send_byte(~d & 0xFF)
                    data.append(d)

                end_byte = self.read_byte_with_echo(0.5)

                if title == 0x09:
                    # ACK empfangen
                    pass
                elif title == 0x29:
                    # Messwertblock-Anfrage
                    group = data[0] if len(data) > 0 else 1
                    self.handle_group_request(group)
                elif title == 0x07:
                    # DTCs auslesen
                    self.handle_dtc_request()
                elif title == 0x05:
                    # DTCs löschen
                    self.handle_dtc_clear()
                elif title == 0x06:
                    # End of communication
                    print("[*] Tester beendet Kommunikation.")
                    self.connected = False

def main():
    parser = argparse.ArgumentParser(description="KW1281 Motronic M3.8.3 ECU Simulator")
    parser.add_argument("--port", default="COM3", help="Serieller Port (z.B. COM3 oder /dev/ttyUSB0)")
    parser.add_argument("--baud", type=int, default=9600, help="Baudrate (Standard: 9600)")
    args = parser.parse_args()

    sim = MotronicM383Simulator(port=args.port, baud=args.baud)
    try:
        sim.open()
        sim.loop()
    except KeyboardInterrupt:
        print("\n[!] Simulator durch Benutzer beendet.")
    except Exception as e:
        print(f"[-] Fehler: {e}")

if __name__ == "__main__":
    main()
