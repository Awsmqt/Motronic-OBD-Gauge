#ifndef DTC_DATABASE_H
#define DTC_DATABASE_H

#include <Arduino.h>

// ==============================================================================
// DTC-DATENBANK (Klartext-Fehlerbeschreibungen fuer 1.8T AGU / Motronic M3.8.3)
// ==============================================================================
// KW1281 liefert Fehlercodes als 16-Bit "VAG-Dezimalcode" (z.B. 16684).
// Umrechnung in den OBD-II P-Code:  P-Nummer = VAG-Code - 16384
//   16684 - 16384 = 300  ->  P0300
//   17476 - 16384 = 1092 ->  P1092
// Codes ausserhalb dieses Bereichs (z.B. 65535) werden als Sonderfaelle behandelt.

struct DtcInfo {
    uint16_t    pNumber;   // P-Nummer dezimal (300 = P0300)
    const char *text;      // Kurzbeschreibung (ASCII, passt auf CYD-Display)
    const char *cause;     // Moegliche Ursachen / Pruefhinweis
};

// Wandelt den rohen KW1281-Code in einen lesbaren Code-String um.
// out muss mindestens 8 Bytes gross sein. Ergebnis z.B. "P0300" oder "VAG 65535".
void dtcFormatCode(uint16_t rawVag, char *out, size_t outSize);

// Sucht den Fehlercode in der Datenbank.
// Liefert true bei Treffer; text/cause werden dann gesetzt. Bei keinem Treffer
// werden generische Texte zurueckgegeben (Funktion liefert dann false).
bool dtcLookup(uint16_t rawVag, const char **text, const char **cause);

// Status-Byte der ECU als Text ("sporadisch", "statisch", ...)
const char *dtcStatusText(uint8_t status);

#endif // DTC_DATABASE_H
