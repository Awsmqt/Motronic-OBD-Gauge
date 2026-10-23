#include "KW1281.h"

KW1281Protocol::KW1281Protocol()
    : state(KLINE_DISCONNECTED),
      blockCounter(0),
      currentGroup(1),
      stateTimer(0),
      lastByteReceivedTime(0),
      rxIndex(0) {
}

void KW1281Protocol::begin() {
    state = KLINE_DISCONNECTED;
    stateTimer = millis();
}

void KW1281Protocol::sendByteWithEchoCheck(uint8_t b) {
    Serial2.write(b);
    // Da K-Line ein Eindrahtbus ist, liest der Transceiver das eigene gesendete Byte als Echo.
    // Wir lesen das Echo aus dem Puffer, damit es nachfolgende Leseoperationen nicht stört:
    unsigned long start = millis();
    while (!Serial2.available() && (millis() - start < 100)) {
        delayMicroseconds(50);
    }
    if (Serial2.available()) {
        Serial2.read(); // Echo verwerfen
    }
}

uint8_t KW1281Protocol::readByteWithTimeout(uint16_t timeoutMs, bool *ok) {
    unsigned long start = millis();
    while (!Serial2.available()) {
        if (millis() - start >= timeoutMs) {
            if (ok) *ok = false;
            return 0;
        }
        delayMicroseconds(100);
    }
    if (ok) *ok = true;
    lastByteReceivedTime = millis();
    return Serial2.read();
}

void KW1281Protocol::start5BaudInit() {
    Serial.println("[KW1281] Starte 5-Baud Slow-Init für Motorsteuergerät (0x01)...");
    
    // Serial2 vorübergehend freigeben für GPIO Bit-Banging
    Serial2.end();
    pinMode(PIN_KLINE_TX, OUTPUT);
    digitalWrite(PIN_KLINE_TX, HIGH);
    delay(200);

    // Adresse 0x01: Startbit(0), 7 Datenbits LSB, Odd Parity, Stopbit(1)
    // 5 Baud = 200 ms pro Bit!
    // Bits für 0x01:
    // Startbit: 0
    // Bit 0: 1
    // Bit 1: 0
    // Bit 2: 0
    // Bit 3: 0
    // Bit 4: 0
    // Bit 5: 0
    // Bit 6: 0
    // Parity (ungerade Summe): Da nur Bit0 gesetzt ist (1 Eins), Parity = 0
    // Stopbit: 1

    uint8_t bits[10] = { 0, 1, 0, 0, 0, 0, 0, 0, 0, 1 };
    for (int i = 0; i < 10; i++) {
        digitalWrite(PIN_KLINE_TX, bits[i] ? HIGH : LOW);
        delay(200);
    }
    digitalWrite(PIN_KLINE_TX, HIGH);
    delay(100);

    // K-Line Serial2 wieder auf 9600 Baud initialisieren (Standard KW1281 Init)
    // Manche Bosch M3.8.3 nutzen 9600, manche 10400.
    Serial2.begin(9600, SERIAL_8N1, PIN_KLINE_RX, PIN_KLINE_TX);
    
    // Empfangspuffer leeren
    while (Serial2.available()) Serial2.read();

    state = KLINE_WAIT_SYNC;
    stateTimer = millis();
}

bool KW1281Protocol::readBlock(uint8_t *buffer, uint8_t &length, uint8_t &title) {
    bool ok = false;
    uint8_t len = readByteWithTimeout(1200, &ok);
    if (!ok) return false;

    // Komplement-Handshake für jedes Byte im KW1281 Protokoll:
    // Nach Empfang jedes Bytes muss das Komplement (~b) als Quittung gesendet werden
    sendByteWithEchoCheck(~len);

    uint8_t counter = readByteWithTimeout(500, &ok);
    if (!ok) return false;
    sendByteWithEchoCheck(~counter);
    blockCounter = counter;

    title = readByteWithTimeout(500, &ok);
    if (!ok) return false;
    sendByteWithEchoCheck(~title);

    uint8_t dataLen = (len >= 3) ? (len - 3) : 0;
    for (uint8_t i = 0; i < dataLen; i++) {
        uint8_t b = readByteWithTimeout(500, &ok);
        if (!ok) return false;
        sendByteWithEchoCheck(~b);
        buffer[i] = b;
    }

    uint8_t endByte = readByteWithTimeout(500, &ok);
    if (!ok || endByte != 0x03) return false;

    length = dataLen;
    return true;
}

void KW1281Protocol::sendBlock(const uint8_t *data, uint8_t length, uint8_t title) {
    uint8_t totalLen = length + 3;
    blockCounter++;

    // Sende Länge & warte auf Quittung
    sendByteWithEchoCheck(totalLen);
    bool ok;
    readByteWithTimeout(500, &ok); // ECU quittiert mit Komplement

    // Sende Block-Counter
    sendByteWithEchoCheck(blockCounter);
    readByteWithTimeout(500, &ok);

    // Sende Titel
    sendByteWithEchoCheck(title);
    readByteWithTimeout(500, &ok);

    // Sende Daten
    for (uint8_t i = 0; i < length; i++) {
        sendByteWithEchoCheck(data[i]);
        readByteWithTimeout(500, &ok);
    }

    // Sende Block-Ende (0x03)
    sendByteWithEchoCheck(0x03);
}

void KW1281Protocol::sendAck() {
    sendBlock(nullptr, 0, 0x09); // 0x09 = ACK
}

void KW1281Protocol::requestGroup(uint8_t groupNumber) {
    uint8_t data[1] = { groupNumber };
    sendBlock(data, 1, 0x29); // 0x29 = Read Group / Messwertblock lesen
}

void KW1281Protocol::decodeField(uint8_t formula, uint8_t a, uint8_t b, int fieldIndex, SensorData &sensorData) {
    // VAG KW1281 Formel-Dekodierung:
    switch (formula) {
        case 1: // Drehzahl: 0.2 * a * b
            sensorData.ecuRpm = (int)(0.2f * (float)a * (float)b);
            break;
        case 5: // Kühlmittel / IAT: 0.1 * (a - 100) oder a * 1 - 50
            if (fieldIndex == 1) {
                sensorData.ecuCoolantTemp = (float)a - 50.0f;
            } else if (fieldIndex == 3) {
                sensorData.ecuIntakeAirTemp = (float)a - 50.0f;
            }
            break;
        case 7: // Geschwindigkeit: 0.01 * a * b
            break;
        case 20: // Last in %: a * 0.392 ODER Luftmasse MAF
            if (fieldIndex == 1) {
                sensorData.ecuEngineLoadPct = (float)a * 0.392f;
            } else if (fieldIndex == 3) {
                sensorData.ecuMafGs = (float)a * 0.78125f; // Luftmasse in g/s
            }
            break;
        case 21: // Spannung: 0.001 * a * b
            sensorData.ecuBatteryVolt = 0.001f * (float)a * (float)b;
            break;
        case 25: // Einspritzzeit: a * 0.05 ms
            sensorData.ecuInjectionMs = (float)a * 0.05f;
            break;
        case 36: // Zündwinkel: a * 0.4 °KW
            sensorData.ecuIgnitionAngle = (float)a * 0.4f;
            break;
        case 37: // Klopfregelung Zündwinkelrücknahme: a * 0.75 °KW
            if ((float)a * 0.75f > sensorData.ecuKnockRetardMax) {
                sensorData.ecuKnockRetardMax = (float)a * 0.75f;
            }
            break;
        default:
            break;
    }
}

void KW1281Protocol::parseGroupData(const uint8_t *data, uint8_t length, SensorData &sensorData) {
    // Ein Messwertblock enthält 4 Felder. Jedes Feld besteht aus 3 Bytes: (Formel, Byte A, Byte B)
    if (length < 12) return;
    for (int i = 0; i < 4; i++) {
        uint8_t formula = data[i * 3];
        uint8_t a = data[i * 3 + 1];
        uint8_t b = data[i * 3 + 2];
        decodeField(formula, a, b, i, sensorData);
    }
}

void KW1281Protocol::update(SensorData &data) {
    unsigned long now = millis();

    switch (state) {
        case KLINE_DISCONNECTED:
            // Nach 3 Sekunden Trennung neuen Verbindungsaufbau versuchen
            if (now - stateTimer > 3000) {
                start5BaudInit();
            }
            break;

        case KLINE_WAIT_SYNC: {
            bool ok;
            uint8_t sync = readByteWithTimeout(3000, &ok);
            if (ok && sync == 0x55) {
                Serial.println("[KW1281] Sync Byte 0x55 empfangen!");
                state = KLINE_WAIT_KEYBYTES;
                stateTimer = now;
            } else if (now - stateTimer > 3500) {
                Serial.println("[KW1281] Timeout beim Warten auf Sync Byte. Wiederhole...");
                state = KLINE_ERROR_WAIT;
                stateTimer = now;
            }
            break;
        }

        case KLINE_WAIT_KEYBYTES: {
            bool ok;
            uint8_t kb1 = readByteWithTimeout(1000, &ok);
            if (!ok) { state = KLINE_ERROR_WAIT; stateTimer = now; break; }
            uint8_t kb2 = readByteWithTimeout(1000, &ok);
            if (!ok) { state = KLINE_ERROR_WAIT; stateTimer = now; break; }

            Serial.printf("[KW1281] Keybytes empfangen: 0x%02X 0x%02X\n", kb1, kb2);
            // Sende invertiertes Keybyte 2 als Handshake
            delay(30);
            sendByteWithEchoCheck(~kb2);

            state = KLINE_ACK_ECU_INFO;
            stateTimer = now;
            break;
        }

        case KLINE_ACK_ECU_INFO: {
            // Steuergerät sendet Identifikations-Strings (Titel 0xF6)
            uint8_t buf[48];
            uint8_t len = 0;
            uint8_t title = 0;
            if (readBlock(buf, len, title)) {
                if (title == 0xF6) {
                    char ecuStr[49];
                    memcpy(ecuStr, buf, len);
                    ecuStr[len] = '\0';
                    Serial.printf("[KW1281] ECU ID: %s\n", ecuStr);
                    sendAck();
                } else if (title == 0x09) {
                    // ACK empfangen -> Verbunden!
                    Serial.println("[KW1281] Handshake abgeschlossen! Verbunden mit Bosch Motronic.");
                    state = KLINE_CONNECTED;
                    data.klineConnected = true;
                    currentGroup = 1;
                }
            } else if (now - stateTimer > 4000) {
                state = KLINE_ERROR_WAIT;
                stateTimer = now;
            }
            break;
        }

        case KLINE_CONNECTED: {
            // Reihum Messwertblöcke MWB 001, MWB 002, MWB 003 abfragen
            requestGroup(currentGroup);
            
            uint8_t buf[48];
            uint8_t len = 0;
            uint8_t title = 0;
            if (readBlock(buf, len, title)) {
                if (title == 0xE7) { // 0xE7 = Group data response
                    parseGroupData(buf, len, data);
                    data.klinePackets++;
                    data.klineConnected = true;
                }
                sendAck();

                // Nächsten Block rotieren (1 -> 2 -> 3 -> 1)
                currentGroup++;
                if (currentGroup > 3) currentGroup = 1;
            } else {
                // Timeout bei Blockempfang
                Serial.println("[KW1281] Verbindungsverlust zur ECU. Reconnect...");
                data.klineConnected = false;
                state = KLINE_ERROR_WAIT;
                stateTimer = now;
            }
            break;
        }

        case KLINE_ERROR_WAIT:
            data.klineConnected = false;
            if (now - stateTimer > 2000) {
                state = KLINE_DISCONNECTED;
                stateTimer = now;
            }
            break;
    }
}
