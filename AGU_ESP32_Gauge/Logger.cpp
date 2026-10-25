// ==============================================================================
// Logger.cpp — SD-Karten Datenlogger Implementierung
// ==============================================================================
// CSV-Logging auf SD-Karte via HSPI (ESP32 CYD built-in SD-Slot).
// Gepuffert (LOG_BUFFER_LINES Zeilen im RAM), automatische Session-Rotation.
// ==============================================================================

#include "Logger.h"

// CSV-Kopfzeile — muss mit _formatDataLine() übereinstimmen
static const char CSV_HEADER[] PROGMEM =
    "millis,boost_bar,oil_press_bar,oil_temp_c,coolant_c,charge_air_c,"
    "egt_c,batt_v,iat_ecu_c,load_pct,maf_gs,rpm,"
    "lambda,inj_ms,ign_deg,knock_deg,throttle_pct,alarm_flags";

// Log-Verzeichnis auf der SD-Karte
static const char LOG_DIR[] = "/logs";

// Dateinamen-Präfix und Format
static const char FILE_PREFIX[] = "/logs/AGU_session_";
static const char FILE_SUFFIX[] = ".csv";

// ==============================================================================
// Konstruktor
// ==============================================================================
DataLogger::DataLogger()
    : _hspi(HSPI)
    , _sdMounted(false)
    , _fileOpen(false)
    , _sessionNumber(0)
    , _lastWriteMs(0)
    , _lineCount(0)
    , _bufferCount(0)
{
}

// ==============================================================================
// begin() — SD-Karte initialisieren und erste Log-Datei anlegen
// ==============================================================================
bool DataLogger::begin() {
    #if !ENABLE_SD_LOGGING
        Serial.println(F("[Logger] SD-Logging deaktiviert (ENABLE_SD_LOGGING=false)"));
        return false;
    #endif

    Serial.println(F("[Logger] Initialisiere SD-Karte (HSPI)..."));

    // HSPI für SD-Karte konfigurieren (CYD Standard-Pins)
    _hspi.begin(18, 19, 23, PIN_SD_CS);  // CLK, MISO, MOSI, CS

    // SD-Karte mit HSPI starten
    if (!SD.begin(PIN_SD_CS, _hspi)) {
        Serial.println(F("[Logger] FEHLER: SD-Karte nicht erkannt! Logging deaktiviert."));
        _sdMounted = false;
        return false;
    }

    _sdMounted = true;

    // Kartentyp und Kapazität ausgeben
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println(F("[Logger] FEHLER: Keine SD-Karte eingesteckt!"));
        _sdMounted = false;
        return false;
    }

    const char* typeStr = "Unbekannt";
    if (cardType == CARD_MMC)       typeStr = "MMC";
    else if (cardType == CARD_SD)   typeStr = "SD";
    else if (cardType == CARD_SDHC) typeStr = "SDHC";

    uint64_t totalMB = SD.totalBytes() / (1024 * 1024);
    uint64_t usedMB  = SD.usedBytes()  / (1024 * 1024);
    Serial.printf("[Logger] SD-Karte: %s, %llu MB total, %llu MB belegt\n",
                  typeStr, totalMB, usedMB);

    // Verzeichnis anlegen und erste Datei öffnen
    if (!_ensureLogDirectory()) {
        Serial.println(F("[Logger] FEHLER: Konnte /logs/ Verzeichnis nicht anlegen!"));
        _sdMounted = false;
        return false;
    }

    // Session-Limit prüfen und ggf. alte Dateien löschen
    _enforceSessionLimit();

    // Nächste freie Session-Nummer ermitteln und Datei öffnen
    _sessionNumber = _findNextSessionNumber();
    if (!_openNewFile()) {
        Serial.println(F("[Logger] FEHLER: Konnte Log-Datei nicht erstellen!"));
        return false;
    }

    Serial.printf("[Logger] Bereit. Logge in: %s (Intervall: %d ms)\n",
                  _currentFilename.c_str(), LOG_INTERVAL_MS);
    return true;
}

// ==============================================================================
// update() — Wird zyklisch aus loop() aufgerufen
// ==============================================================================
void DataLogger::update(const SensorData &data) {
    uint32_t now = millis();
    if ((now - _lastWriteMs) < LOG_INTERVAL_MS) return;

    if (!_sdMounted) return;
    if (!_fileOpen) {
        _logFile = SD.open(_currentFilename, FILE_APPEND);
        if (!_logFile) {
            Serial.println(F("[Logger] FEHLER: Log-Datei konnte nicht erneut geöffnet werden."));
            _lastWriteMs = now;
            return;
        }
        _fileOpen = true;
        flush();
        if (!_fileOpen) {
            _lastWriteMs = now;
            return;
        }
    }

    _lastWriteMs = now;

    // CSV-Zeile formatieren und in RAM-Puffer schreiben
    _writeBuffer += _formatDataLine(data);
    _writeBuffer += '\n';
    _bufferCount++;
    _lineCount++;

    // Puffer voll → auf SD-Karte flushen
    if (_bufferCount >= LOG_BUFFER_LINES) {
        flush();
    }

    // Dateigrössen-Limit prüfen → ggf. neue Session starten
    if (_isFileSizeLimitReached()) {
        Serial.printf("[Logger] Dateigroesse-Limit (%d MB) erreicht. Starte neue Session.\n",
                      LOG_MAX_FILE_SIZE_MB);
        startNewSession();
    }
}

// ==============================================================================
// flush() — RAM-Puffer auf SD-Karte schreiben
// ==============================================================================
void DataLogger::flush() {
    if (!_fileOpen || _bufferCount == 0) return;

    const size_t bufferLength = _writeBuffer.length();
    const size_t written = _logFile.write(
        (const uint8_t *)_writeBuffer.c_str(), bufferLength);
    uint8_t completedLines = 0;
    for (size_t i = 0; i < written; ++i) {
        if (_writeBuffer[i] == '\n') completedLines++;
    }
    if (completedLines > _bufferCount) completedLines = _bufferCount;
    _bufferCount -= completedLines;
    if (written > 0) {
        _writeBuffer.remove(0, written);
    }

    if (_writeBuffer.length() > 0) {
        Serial.printf("[Logger] WARNUNG: SD-Schreibvorgang unvollständig (%u/%u Bytes); Puffer bleibt erhalten.\n",
                      (unsigned)written, (unsigned)bufferLength);
        _logFile.close();
        _fileOpen = false;
        return;
    }

    _logFile.flush();
    _writeBuffer = "";
    _bufferCount = 0;
}

// ==============================================================================
// isReady() — SD-Karte gemountet und Datei offen?
// ==============================================================================
bool DataLogger::isReady() const {
    return _sdMounted && _fileOpen;
}

// ==============================================================================
// getFilename() — Aktuellen Dateinamen zurückgeben
// ==============================================================================
String DataLogger::getFilename() const {
    return _currentFilename;
}

// ==============================================================================
// getLogCount() — Anzahl geschriebener Zeilen
// ==============================================================================
uint32_t DataLogger::getLogCount() const {
    return _lineCount;
}

// ==============================================================================
// startNewSession() — Aktuelle Datei schliessen, neue Session beginnen
// ==============================================================================
bool DataLogger::startNewSession() {
    // Restlichen Puffer noch schreiben
    flush();
    if (_bufferCount > 0 || _writeBuffer.length() > 0) {
        Serial.println(F("[Logger] FEHLER: Session bleibt offen, bis der Log-Puffer geschrieben wurde."));
        return false;
    }

    // Aktuelle Datei schliessen
    if (_fileOpen) {
        _logFile.close();
        _fileOpen = false;
        Serial.printf("[Logger] Session %04d beendet (%lu Zeilen)\n",
                      _sessionNumber, _lineCount);
    }

    // Session-Limit durchsetzen (alte Dateien löschen)
    _enforceSessionLimit();

    // Neue Session starten
    _sessionNumber++;
    _lineCount = 0;
    _bufferCount = 0;
    _writeBuffer = "";

    if (!_openNewFile()) {
        Serial.println(F("[Logger] FEHLER: Neue Session-Datei konnte nicht erstellt werden!"));
        return false;
    }

    Serial.printf("[Logger] Neue Session %04d gestartet: %s\n",
                  _sessionNumber, _currentFilename.c_str());
    return true;
}

// ==============================================================================
// PRIVATE: _ensureLogDirectory() — /logs/ Verzeichnis anlegen
// ==============================================================================
bool DataLogger::_ensureLogDirectory() {
    if (SD.exists(LOG_DIR)) return true;

    Serial.println(F("[Logger] Erstelle /logs/ Verzeichnis..."));
    return SD.mkdir(LOG_DIR);
}

// ==============================================================================
// PRIVATE: _findNextSessionNumber() — Nächste freie Session-Nummer finden
// ==============================================================================
uint16_t DataLogger::_findNextSessionNumber() {
    uint16_t maxFound = 0;

    File dir = SD.open(LOG_DIR);
    if (!dir || !dir.isDirectory()) {
        return 1;
    }

    // Alle Dateien im Verzeichnis durchgehen
    File entry;
    while ((entry = dir.openNextFile())) {
        String name = entry.name();
        entry.close();

        // Dateiname parsen: "AGU_session_NNNN.csv"
        // entry.name() gibt je nach SD-Bibliothek nur den Dateinamen zurück
        int idx = name.lastIndexOf("AGU_session_");
        if (idx < 0) continue;

        // Nummer extrahieren (4 Ziffern nach "AGU_session_")
        int numStart = idx + 12; // Länge von "AGU_session_"
        String numStr = name.substring(numStart, numStart + 4);
        uint16_t num = (uint16_t)numStr.toInt();
        if (num > maxFound) {
            maxFound = num;
        }
    }
    dir.close();

    return maxFound + 1;
}

// ==============================================================================
// PRIVATE: _openNewFile() — Neue CSV-Datei öffnen und Header schreiben
// ==============================================================================
bool DataLogger::_openNewFile() {
    // Dateinamen zusammenbauen: /logs/AGU_session_NNNN.csv
    char filename[40];
    snprintf(filename, sizeof(filename), "%s%04d%s",
             FILE_PREFIX, _sessionNumber, FILE_SUFFIX);
    _currentFilename = String(filename);

    // Falls Datei schon existiert, Nummer hochzählen (Sicherheit)
    while (SD.exists(_currentFilename)) {
        _sessionNumber++;
        snprintf(filename, sizeof(filename), "%s%04d%s",
                 FILE_PREFIX, _sessionNumber, FILE_SUFFIX);
        _currentFilename = String(filename);
    }

    // Neue Datei erstellen
    _logFile = SD.open(_currentFilename, FILE_WRITE);
    if (!_logFile) {
        Serial.printf("[Logger] FEHLER: Kann %s nicht öffnen!\n",
                      _currentFilename.c_str());
        _fileOpen = false;
        return false;
    }

    _fileOpen = true;
    _lineCount = 0;
    _bufferCount = 0;
    _writeBuffer = "";

    // CSV-Header schreiben
    _writeHeader();
    return true;
}

// ==============================================================================
// PRIVATE: _writeHeader() — CSV-Kopfzeile in Datei schreiben
// ==============================================================================
void DataLogger::_writeHeader() {
    if (!_fileOpen) return;
    _logFile.println(FPSTR(CSV_HEADER));
    _logFile.flush();
}

// ==============================================================================
// PRIVATE: _formatDataLine() — Sensordaten als CSV-Zeile formatieren
// ==============================================================================
String DataLogger::_formatDataLine(const SensorData &data) {
    // Alarm-Bitmaske bauen
    uint8_t alarms = _buildAlarmFlags(data);

    // CSV-Zeile zusammenbauen (feste Dezimalstellen für Konsistenz)
    char line[256];
    snprintf(line, sizeof(line),
        "%lu,"       // millis
        "%.2f,"      // boost_bar
        "%.2f,"      // oil_press_bar
        "%.1f,"      // oil_temp_c
        "%.1f,"      // coolant_c
        "%.1f,"      // charge_air_c
        "%.1f,"      // egt_c
        "%.2f,"      // batt_v
        "%.1f,"      // iat_ecu_c
        "%.1f,"      // load_pct
        "%.2f,"      // maf_gs
        "%d,"        // rpm
        "%.3f,"      // lambda
        "%.2f,"      // inj_ms
        "%.1f,"      // ign_deg
        "%.1f,"      // knock_deg
        "%.1f,"      // throttle_pct
        "%u",        // alarm_flags
        (unsigned long)millis(),
        data.boostBar,
        data.oilPressureBar,
        data.oilTemp,
        data.coolantTemp,
        data.chargeAirTemp,
        data.exhaustTemp,
        data.batteryVolt,
        data.ecuIntakeAirTemp,
        data.ecuEngineLoadPct,
        data.ecuMafGs,
        data.ecuRpm,
        data.ecuLambda,
        data.ecuInjectionMs,
        data.ecuIgnitionTiming,
        data.ecuKnockRetard,
        data.ecuThrottlePct,
        alarms
    );

    return String(line);
}

// ==============================================================================
// PRIVATE: _buildAlarmFlags() — Alarm-Bitmaske zusammenbauen
// ==============================================================================
//   Bit 0 = Öldruck         Bit 4 = EGT
//   Bit 1 = Öltemperatur    Bit 5 = Ladedruck (Overboost)
//   Bit 2 = Kühlwasser      Bit 6 = Batteriespannung
//   Bit 3 = Ladeluft        Bit 7 = MIL (Motorkontrollleuchte)
// ==============================================================================
uint8_t DataLogger::_buildAlarmFlags(const SensorData &data) {
    uint8_t flags = 0;
    if (data.alarmOilPressure) flags |= (1 << 0);
    if (data.alarmOilTemp)     flags |= (1 << 1);
    if (data.alarmCoolant)     flags |= (1 << 2);
    if (data.alarmChargeAir)   flags |= (1 << 3);
    if (data.alarmEgt)         flags |= (1 << 4);
    if (data.alarmBoost)       flags |= (1 << 5);
    if (data.alarmBattery)     flags |= (1 << 6);
    if (data.milSeen)          flags |= (1 << 7);
    return flags;
}

// ==============================================================================
// PRIVATE: _isFileSizeLimitReached() — Dateigrösse prüfen
// ==============================================================================
bool DataLogger::_isFileSizeLimitReached() {
    if (!_fileOpen) return false;
    size_t currentSize = _logFile.size();
    size_t limitBytes  = (size_t)LOG_MAX_FILE_SIZE_MB * 1024UL * 1024UL;
    return (currentSize >= limitBytes);
}

// ==============================================================================
// PRIVATE: _enforceSessionLimit() — Alte Dateien löschen wenn Limit erreicht
// ==============================================================================
void DataLogger::_enforceSessionLimit() {
    // Alle Session-Dateien zählen und die älteste (niedrigste Nummer) finden
    File dir = SD.open(LOG_DIR);
    if (!dir || !dir.isDirectory()) return;

    uint16_t fileCount = 0;
    uint16_t minSession = 9999;

    // Erster Durchlauf: zählen und Minimum finden
    File entry;
    while ((entry = dir.openNextFile())) {
        String name = entry.name();
        entry.close();

        int idx = name.lastIndexOf("AGU_session_");
        if (idx < 0) continue;

        fileCount++;
        int numStart = idx + 12;
        String numStr = name.substring(numStart, numStart + 4);
        uint16_t num = (uint16_t)numStr.toInt();
        if (num < minSession) {
            minSession = num;
        }
    }
    dir.close();

    // Älteste Dateien löschen bis unter dem Limit
    while (fileCount >= LOG_MAX_SESSIONS && minSession < 9999) {
        char delName[40];
        snprintf(delName, sizeof(delName), "%s%04d%s",
                 FILE_PREFIX, minSession, FILE_SUFFIX);

        if (SD.exists(delName)) {
            SD.remove(delName);
            Serial.printf("[Logger] Alte Session gelöscht: %s\n", delName);
            fileCount--;
        }

        minSession++;

        // Sicherheit: Endlosschleife vermeiden
        if (minSession > 9999) break;
    }
}
