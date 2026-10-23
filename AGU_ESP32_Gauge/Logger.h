// ==============================================================================
// Logger.h — SD-Karten Datenlogger für VW 1.8T AGU Telemetrie
// ==============================================================================
// Schreibt Sensor- und ECU-Daten als CSV auf die SD-Karte (HSPI).
// Gepufferte Schreibweise (RAM-Puffer) zur Schonung der SD-Karte.
// Automatische Session-Verwaltung mit Dateigrössen-Limit und Rotation.
// ==============================================================================

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <SD.h>
#include <FS.h>
#include <SPI.h>
#include "Config.h"

class DataLogger {
public:
    DataLogger();

    // Initialisiert die SD-Karte (HSPI) und erstellt die erste Log-Datei
    bool begin();

    // Wird zyklisch aufgerufen — schreibt CSV-Zeile wenn Intervall erreicht
    void update(const SensorData &data);

    // Erzwingt sofortiges Schreiben des RAM-Puffers auf die SD-Karte
    void flush();

    // Gibt true zurück wenn SD-Karte gemountet und Datei offen ist
    bool isReady() const;

    // Gibt den aktuellen Dateinamen zurück (z.B. "/logs/AGU_session_0001.csv")
    String getFilename() const;

    // Gibt die Anzahl geschriebener Zeilen in der aktuellen Session zurück
    uint32_t getLogCount() const;

    // Schliesst aktuelle Datei und öffnet eine neue Session
    bool startNewSession();

private:
    // HSPI-Instanz für SD-Karte (CYD verwendet HSPI, nicht VSPI)
    SPIClass _hspi;

    // Datei-Handle für aktuelle CSV-Datei
    File _logFile;

    // Status-Flags
    bool _sdMounted;        // SD-Karte erfolgreich initialisiert
    bool _fileOpen;         // Aktuelle Datei ist geöffnet

    // Session-Verwaltung
    uint16_t _sessionNumber;  // Laufende Session-Nummer (0001 - 9999)
    String   _currentFilename;

    // Schreib-Steuerung
    uint32_t _lastWriteMs;    // Timestamp des letzten Schreibvorgangs
    uint32_t _lineCount;      // Zeilen in aktueller Session
    uint8_t  _bufferCount;    // Zeilen im RAM-Puffer (noch nicht geflusht)

    // RAM-Puffer: sammelt mehrere CSV-Zeilen vor dem Flush
    String _writeBuffer;

    // --- Private Hilfsmethoden ---

    // Erstellt das Verzeichnis /logs/ falls nicht vorhanden
    bool _ensureLogDirectory();

    // Findet die nächste freie Session-Nummer
    uint16_t _findNextSessionNumber();

    // Öffnet eine neue CSV-Datei und schreibt den Header
    bool _openNewFile();

    // Schreibt die CSV-Kopfzeile
    void _writeHeader();

    // Formatiert eine Datenzeile als CSV-String
    String _formatDataLine(const SensorData &data);

    // Erstellt die Alarm-Bitmaske aus den einzelnen Alarm-Flags
    uint8_t _buildAlarmFlags(const SensorData &data);

    // Prüft ob Dateigrösse das Limit überschritten hat
    bool _isFileSizeLimitReached();

    // Löscht die ältesten Session-Dateien wenn Maximum überschritten
    void _enforceSessionLimit();
};

#endif // LOGGER_H
