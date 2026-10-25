#ifndef KW1281_H
#define KW1281_H

#include <Arduino.h>
#include "Config.h"

enum KlineState {
    KLINE_DISCONNECTED = 0,
    KLINE_WAIT_SYNC,
    KLINE_WAIT_KEYBYTES,
    KLINE_ACK_ECU_INFO,
    KLINE_CONNECTED,
    KLINE_ERROR_WAIT
};

class KW1281Protocol {
public:
    KW1281Protocol();
    void begin();
    void update(SensorData &data);
    bool isSlowInitActive() const { return state == KLINE_SENDING_5BAUD; }

private:
    static const uint8_t MAX_BLOCK_DATA = 48;

    KlineState state;
    uint8_t blockCounter;
    uint8_t currentGroup;
    uint8_t initBitIndex;
    uint8_t keyByteCount;
    uint8_t keyBytes[2];
    unsigned long stateTimer;
    unsigned long lastDtcCheckTime;     // Zeitpunkt der letzten DTC-Prüfung

    void start5BaudInit();
    bool readBlock(uint8_t *buffer, uint8_t &length, uint8_t &title);
    void sendBlock(const uint8_t *data, uint8_t length, uint8_t title);
    void sendAck();
    void requestGroup(uint8_t groupNumber);
    void parseGroupData(const uint8_t *data, uint8_t length, SensorData &sensorData);
    void decodeField(uint8_t formula, uint8_t a, uint8_t b, int fieldIndex, SensorData &sensorData);
    uint8_t readByteWithTimeout(uint16_t timeoutMs, bool *ok = nullptr);
    void sendByteWithEchoCheck(uint8_t b);

    // DTC-Funktionen
    bool requestDTCs(SensorData &data);
    bool clearDTCs();
};

#endif // KW1281_H
