#include "KW1281.h"

KW1281Protocol::KW1281Protocol()
    : state(KLINE_DISCONNECTED),
      blockCounter(0),
      currentGroup(1),
      initBitIndex(0),
      keyByteCount(0),
      keyBytes{0, 0},
      stateTimer(0),
      lastDtcCheckTime(0) {
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
    return Serial2.read();
}

void KW1281Protocol::start5BaudInit() {
    Serial.println("[KW1281] Starte 5-Baud Slow-Init für Motorsteuergerät (0x01)...");
    Serial2.end();
    pinMode(PIN_KLINE_TX, OUTPUT);
    digitalWrite(PIN_KLINE_TX, HIGH);
    initBitIndex = 0;
    state = KLINE_SENDING_5BAUD;
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

    if (len < 3) return false;
    uint8_t dataLen = len - 3;
    bool bufferOverflow = dataLen > MAX_BLOCK_DATA;
    for (uint8_t i = 0; i < dataLen; i++) {
        uint8_t b = readByteWithTimeout(500, &ok);
        if (!ok) return false;
        sendByteWithEchoCheck(~b);
        if (i < MAX_BLOCK_DATA) {
            buffer[i] = b;
        }
    }

    uint8_t endByte = readByteWithTimeout(500, &ok);
    if (!ok || endByte != 0x03) return false;

    length = bufferOverflow ? 0 : dataLen;
    return !bufferOverflow;
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
    float fa = (float)a;
    float fb = (float)b;
    switch (formula) {
        case 1: // Drehzahl: 0.2 * a * b
            sensorData.ecuRpm = (int)(0.2f * fa * fb);
            break;
        case 2: // Einspritzdauer/diverse: a * 0.002 * b (ms)
            sensorData.ecuInjectionMs = fa * 0.002f * fb;
            break;
        case 3: // Zündwinkel: a * 0.04 * b
            sensorData.ecuIgnitionTiming = fa * 0.04f * fb;
            break;
        case 4: // Abweichung: abs(b - 127) * 0.01 * a
            sensorData.ecuKnockRetard = (float)abs((int)b - 127) * 0.01f * fa;
            break;
        case 5: // Kühlmittel / IAT: a - 50 (°C)
            if (fieldIndex == 1) {
                sensorData.coolantTemp = fa - 50.0f;
            } else if (fieldIndex == 3) {
                sensorData.ecuIntakeAirTemp = fa - 50.0f;
            }
            break;
        case 6: // Diverse: a * 0.001 * b
            // Kontextabhängig, z.B. Drosselklappe oder weitere Sensoren
            sensorData.ecuThrottlePct = fa * 0.001f * fb * 100.0f;
            break;
        case 7: // Geschwindigkeit: 0.01 * a * b
            break;
        case 10: // Status-Bits (Rohwerte, Bit-Feld)
            sensorData.ecuStatusBits = b;
            break;
        case 15: // Einspritzzeit: 0.01 * a * b (ms)
            sensorData.ecuInjectionMs = 0.01f * fa * fb;
            break;
        case 18: // Einspritzzeit alternativ: 0.04 * a * b (ms)
            sensorData.ecuInjectionMs = 0.04f * fa * fb;
            break;
        case 20: // Last in % ODER Luftmasse MAF
            if (fieldIndex == 1) {
                sensorData.ecuEngineLoadPct = fa * 0.392f;
            } else if (fieldIndex == 3) {
                sensorData.ecuMafGs = fa * 0.78125f; // Luftmasse in g/s
            }
            break;
        case 21: // Spannung: 0.001 * a * b
            sensorData.batteryVolt = 0.001f * fa * fb;
            break;
        case 25: // Klopfregelung Rücknahme: (b - 128) * 0.01 * a (°)
            sensorData.ecuKnockRetard = (fb - 128.0f) * 0.01f * fa;
            break;
        case 31: // Lambda-Wert: b / 2560 * a (0 - 2.0)
            sensorData.ecuLambda = fb / 2560.0f * fa;
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

        case KLINE_SENDING_5BAUD: {
            static const uint8_t bits[10] = { 0, 1, 0, 0, 0, 0, 0, 0, 0, 1 };
            const unsigned long bitDurationMs = 200;

            if (initBitIndex < 10 && now - stateTimer >= bitDurationMs) {
                digitalWrite(PIN_KLINE_TX, bits[initBitIndex] ? HIGH : LOW);
                initBitIndex++;
                stateTimer = now;
            } else if (initBitIndex == 10 && now - stateTimer >= bitDurationMs) {
                digitalWrite(PIN_KLINE_TX, HIGH);
                initBitIndex++;
                stateTimer = now;
            } else if (initBitIndex == 11 && now - stateTimer >= 100) {
                Serial2.begin(9600, SERIAL_8N1, PIN_KLINE_RX, PIN_KLINE_TX);
                while (Serial2.available()) Serial2.read();
                state = KLINE_WAIT_SYNC;
                stateTimer = now;
            }
            break;
        }

        case KLINE_WAIT_SYNC: {
            if (Serial2.available() && Serial2.read() == 0x55) {
                Serial.println("[KW1281] Sync Byte 0x55 empfangen!");
                state = KLINE_WAIT_KEYBYTES;
                keyByteCount = 0;
                stateTimer = now;
            } else if (now - stateTimer > 3500) {
                Serial.println("[KW1281] Timeout beim Warten auf Sync Byte. Wiederhole...");
                state = KLINE_ERROR_WAIT;
                stateTimer = now;
            }
            break;
        }

        case KLINE_WAIT_KEYBYTES: {
            while (Serial2.available() && keyByteCount < 2) {
                keyBytes[keyByteCount++] = Serial2.read();
                stateTimer = now;
            }

            if (keyByteCount == 2) {
                Serial.printf("[KW1281] Keybytes empfangen: 0x%02X 0x%02X\n",
                              keyBytes[0], keyBytes[1]);
                delay(30);
                sendByteWithEchoCheck(~keyBytes[1]);
                state = KLINE_ACK_ECU_INFO;
                stateTimer = millis();
            } else if (now - stateTimer > 1000) {
                state = KLINE_ERROR_WAIT;
                stateTimer = now;
            }
            break;
        }

        case KLINE_ACK_ECU_INFO: {
            // Steuergerät sendet Identifikations-Strings (Titel 0xF6)
            if (!Serial2.available()) {
                if (now - stateTimer > 4000) {
                    state = KLINE_ERROR_WAIT;
                    stateTimer = now;
                }
                break;
            }

            uint8_t buf[MAX_BLOCK_DATA];
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
            unsigned long now2 = millis();

            // --- DTC-Löschung hat Priorität (wenn angefordert) ---
            if (data.dtcClearPending) {
                Serial.println("[KW1281] Sende DTC-Löschanforderung (Titel 0x05)...");
                data.dtcClearPending = false;
                if (clearDTCs()) {
                    Serial.println("[KW1281] DTCs erfolgreich gelöscht.");
                    data.dtcCount = 0;
                    memset(data.dtcCodes, 0, sizeof(data.dtcCodes));
                    memset(data.dtcStatus, 0, sizeof(data.dtcStatus));
                    data.milSeen = false;
                } else {
                    Serial.println("[KW1281] DTC-Löschung fehlgeschlagen!");
                }
                break;
            }

            // --- DTC-Auslesen (wenn angefordert oder periodisch alle 10s) ---
            if (data.dtcReadPending ||
                (now2 - lastDtcCheckTime >= 10000 && lastDtcCheckTime > 0)) {
                Serial.println("[KW1281] Lese Fehlercodes (Titel 0x07)...");
                data.dtcReadPending = false;
                lastDtcCheckTime = now2;
                if (requestDTCs(data)) {
                    Serial.printf("[KW1281] %d DTC(s) gelesen.\n", data.dtcCount);
                    data.milSeen = (data.dtcCount > 0);
                    data.dtcLastReadMs = millis();
                } else {
                    Serial.println("[KW1281] DTC-Lesung fehlgeschlagen.");
                }
                break;
            }

            // Initiale DTC-Prüfung nach Verbindungsaufbau
            if (lastDtcCheckTime == 0) {
                lastDtcCheckTime = now2;
            }

            // --- Normaler Messwertblock-Zyklus (Gruppen 1-6) ---
            requestGroup(currentGroup);
            
            uint8_t buf[MAX_BLOCK_DATA];
            uint8_t len = 0;
            uint8_t title = 0;
            if (readBlock(buf, len, title)) {
                if (title == 0xE7) { // 0xE7 = Group data response
                    parseGroupData(buf, len, data);
                    data.klinePackets++;
                    data.klineConnected = true;
                }
                sendAck();

                // Nächsten Block rotieren (1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 1)
                currentGroup++;
                if (currentGroup > 6) currentGroup = 1;
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

// ==============================================================================
// DTC-FUNKTIONEN (Fehlercodes lesen / löschen)
// ==============================================================================

bool KW1281Protocol::requestDTCs(SensorData &data) {
    // DTC-Leseanforderung senden: Titel 0x07 (Read DTC)
    sendBlock(nullptr, 0, 0x07);

    uint8_t buf[MAX_BLOCK_DATA];
    uint8_t len = 0;
    uint8_t title = 0;

    if (!readBlock(buf, len, title)) {
        Serial.println("[KW1281] Timeout bei DTC-Antwort.");
        return false;
    }

    if (title == 0xFC) {
        // DTC-Antwort: Jeweils 3 Bytes pro DTC (2 Bytes Nummer big-endian + 1 Byte Status)
        data.dtcCount = 0;
        uint8_t numDtcs = len / 3;
        if (numDtcs > 16) numDtcs = 16;

        for (uint8_t i = 0; i < numDtcs; i++) {
            uint16_t dtcCode = ((uint16_t)buf[i * 3] << 8) | buf[i * 3 + 1];
            uint8_t  dtcStat = buf[i * 3 + 2];

            // DTC-Code 0x0000 überspringen (kein gültiger Fehler)
            if (dtcCode == 0x0000) continue;

            data.dtcCodes[data.dtcCount]  = dtcCode;
            data.dtcStatus[data.dtcCount] = dtcStat;
            data.dtcCount++;

            Serial.printf("[KW1281] DTC #%d: Code 0x%04X, Status 0x%02X\n",
                          data.dtcCount, dtcCode, dtcStat);
        }

        sendAck();
        return true;
    } else if (title == 0x09) {
        // ACK ohne DTCs -> keine Fehlercodes gespeichert
        data.dtcCount = 0;
        Serial.println("[KW1281] Keine DTCs vorhanden (ACK erhalten).");
        return true;
    }

    // Unerwarteter Titel -> trotzdem ACK senden
    Serial.printf("[KW1281] Unerwarteter DTC-Antworttitel: 0x%02X\n", title);
    sendAck();
    return false;
}

bool KW1281Protocol::clearDTCs() {
    // DTC-Löschanforderung senden: Titel 0x05 (Clear DTC / Fehlerspeicher löschen)
    sendBlock(nullptr, 0, 0x05);

    uint8_t buf[MAX_BLOCK_DATA];
    uint8_t len = 0;
    uint8_t title = 0;

    if (!readBlock(buf, len, title)) {
        Serial.println("[KW1281] Timeout bei DTC-Löschantwort.");
        return false;
    }

    // ECU bestätigt mit ACK (0x09) oder manchmal mit 0xFC (leere Liste)
    if (title == 0x09 || title == 0xFC) {
        sendAck();
        Serial.println("[KW1281] Fehlerspeicher gelöscht (ECU hat bestätigt).");
        return true;
    }

    Serial.printf("[KW1281] Unerwartete Antwort auf DTC-Löschung: 0x%02X\n", title);
    sendAck();
    return false;
}
