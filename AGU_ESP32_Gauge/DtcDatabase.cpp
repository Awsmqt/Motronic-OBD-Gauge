#include "DtcDatabase.h"

#define VAG_P_OFFSET 16384u

// Hinweis: Texte bewusst ohne Umlaute und ohne Anfuehrungszeichen (TFT-Font + JSON).
static const DtcInfo DTC_TABLE[] = {
    // --- Luftmenge / Ansaugung / Ladedruck ---
    {  102, "Luftmassenmesser G70: Signal zu niedrig",  "Stecker/Kabel G70, Falschluft nach LMM, LMM verschmutzt oder defekt" },
    {  103, "Luftmassenmesser G70: Signal zu hoch",     "LMM defekt, Kabelbaum, Masseproblem" },
    {  105, "Saugrohrdruck: Signal unplausibel",        "Unterdruckschlauch, Ladedrucksensor G31, Leck im Ladeluftsystem" },
    {  106, "Saugrohrdruck: Bereich/Funktion",          "Ladedrucksensor G31, Schlauch abgefallen, Falschluft" },
    {  107, "Ladedrucksensor G31: Signal zu niedrig",   "Stecker G31, Kabelbruch, Sensor defekt" },
    {  108, "Ladedrucksensor G31: Signal zu hoch",      "Kurzschluss nach Plus, Sensor defekt" },
    {  110, "Ansauglufttemp G42: Fehlfunktion",         "Stecker G42, Sensor defekt" },
    {  112, "Ansauglufttemp G42: Signal zu niedrig",    "Kurzschluss nach Masse, Sensor defekt" },
    {  113, "Ansauglufttemp G42: Signal zu hoch",       "Unterbrechung Kabel/Stecker, Sensor defekt" },
    {  299, "Ladedruck: Regelabweichung zu niedrig",    "Leck Ladeluftschlauch/LLK, N75, Druckdose, Wastegate, Turbo, Schubumluftventil N249" },
    { 1092, "Ladedruckregelung: Unterschreitung",       "Ladeluft-Leck, N75-Ventil, Schlaeuche, Druckdose, Turbolader" },
    { 1093, "Ladedruckregelung: Ueberschreitung",       "N75 klemmt, Wastegate, Druckdose, Unterdruckschlauch" },
    { 1096, "Ladedruck-Magnetventil N75: Fehler",       "Stecker/Kabel N75, Ventil defekt" },

    // --- Kuehlmittel / Temperatur ---
    {  115, "Kuehlmitteltemp G62: Fehlfunktion",        "Geber G62, Stecker, Kabelbaum" },
    {  117, "Kuehlmitteltemp G62: Signal zu niedrig",   "Kurzschluss nach Masse, Geber G62 defekt" },
    {  118, "Kuehlmitteltemp G62: Signal zu hoch",      "Unterbrechung, Geber G62 defekt" },

    // --- Drosselklappe / Leerlauf / Gaspedal ---
    {  121, "Drosselklappenpoti G69: Bereich/Funktion", "Drosselklappe verschmutzt, Poti G69, Anpassung noetig" },
    {  122, "Drosselklappenpoti G69: Signal zu niedrig","Stecker, Kabel, Poti defekt" },
    {  123, "Drosselklappenpoti G69: Signal zu hoch",   "Kurzschluss nach Plus, Poti defekt" },
    {  505, "Leerlaufregelung: Fehler",                 "Drosselklappe verschmutzt, Falschluft, Leerlaufregler" },
    {  506, "Leerlaufdrehzahl zu niedrig",              "Falschluft, Drosselklappe verschmutzt, Anpassung" },
    {  507, "Leerlaufdrehzahl zu hoch",                 "Falschluft (Schlauche, Dichtungen), Drosselklappe" },
    { 1545, "Drosselklappen-Stellglied: Fehler",        "Drosselklappensteller, Stecker, Anpassung durchfuehren" },

    // --- Lambdasonden / Gemisch / Katalysator ---
    {  130, "Lambdasonde G39 (vor Kat): Fehlfunktion",  "Sonde G39, Kabel/Stecker, Abgasleck vor Sonde" },
    {  131, "Lambdasonde G39: Signal zu niedrig",       "Sonde verbraucht, Falschluft, Abgasleck" },
    {  132, "Lambdasonde G39: Signal zu hoch",          "Kurzschluss nach Plus, Sonde defekt" },
    {  133, "Lambdasonde G39: traege Reaktion",         "Sonde gealtert, Abgasleck, Falschluft" },
    {  134, "Lambdasonde G39: kein Signal",             "Unterbrechung Kabel, Sonde defekt" },
    {  135, "Sondenheizung G39: Fehler",                "Heizung/Sicherung/Kabel, Sonde defekt" },
    {  136, "Lambdasonde G130 (nach Kat): Fehlfunktion","Sonde G130, Kabel, Abgasleck" },
    {  141, "Lambdasonde G130: Heizung/Signal",         "Heizung, Kabelbaum, Sonde defekt" },
    {  171, "Gemisch zu mager (Bank 1)",                "Falschluft, LMM, Benzindruck, Einspritzventile, Abgasleck vor Sonde" },
    {  172, "Gemisch zu fett (Bank 1)",                 "Einspritzventile undicht, Benzindruck zu hoch, LMM, Aktivkohleventil" },
    {  420, "Katalysator: Wirkungsgrad zu gering",      "Katalysator gealtert/defekt, Lambdasonden pruefen" },
    { 1130, "Lambdaregelung: Grenze erreicht",          "Falschluft, Abgasleck, LMM, Einspritzung" },

    // --- Zuendung / Aussetzer / Klopfen ---
    {  300, "Zuendaussetzer erkannt",                   "Zuendspulen/Kerzen, Einspritzventile, Falschluft, Kompression, Benzindruck" },
    {  301, "Zuendaussetzer Zylinder 1",                "Zuendspule N70 / Kerze Zyl. 1, Einspritzventil, Kompression" },
    {  302, "Zuendaussetzer Zylinder 2",                "Zuendspule N127 / Kerze Zyl. 2, Einspritzventil, Kompression" },
    {  303, "Zuendaussetzer Zylinder 3",                "Zuendspule N291 / Kerze Zyl. 3, Einspritzventil, Kompression" },
    {  304, "Zuendaussetzer Zylinder 4",                "Zuendspule N292 / Kerze Zyl. 4, Einspritzventil, Kompression" },
    {  325, "Klopfsensor 1 G61: Fehlfunktion",          "Klopfsensor G61, Anzugsmoment, Kabel/Stecker" },
    {  327, "Klopfsensor 1 G61: Signal zu niedrig",     "Kabel/Stecker, Sensor lose oder defekt" },
    {  335, "Kurbelwellengeber G28: kein Signal",       "Geber G28, Kabel/Stecker, Geberrad, Abstand" },
    {  340, "Nockenwellengeber G40: kein Signal",       "Geber G40, Kabel/Stecker, Steuerzeiten pruefen" },
    {  341, "Nockenwellengeber G40: unplausibel",       "Steuerzeiten verstellt (Zahnriemen), Geber G40" },
    {  343, "Nockenwellengeber G40: Signal zu hoch",    "Kurzschluss, Geber defekt" },

    // --- Einspritzung / Kraftstoff ---
    {  201, "Einspritzventil Zyl. 1 N30: Fehler",       "Stecker, Kabel, Ventil defekt" },
    {  202, "Einspritzventil Zyl. 2 N31: Fehler",       "Stecker, Kabel, Ventil defekt" },
    {  203, "Einspritzventil Zyl. 3 N32: Fehler",       "Stecker, Kabel, Ventil defekt" },
    {  204, "Einspritzventil Zyl. 4 N33: Fehler",       "Stecker, Kabel, Ventil defekt" },

    // --- Tankentlueftung / Sekundaerluft / Abgas ---
    {  444, "Aktivkohlefilter-Ventil N80: Unterbrechung","Stecker/Kabel N80, Ventil defekt" },
    {  443, "Aktivkohlefilter-Ventil N80: Fehler",      "Ventil N80, Schlaeuche, Kabelbaum" },
    {  491, "Sekundaerluft: Bank 1 Durchfluss zu gering","Sekundaerluftpumpe/Ventil, Schlaeuche verstopft" },

    // --- Steuergeraet / Spannung / Signale ---
    {  560, "Bordspannung: Systemspannung unplausibel", "Batterie, Lichtmaschine, Masseverbindungen" },
    {  562, "Bordspannung zu niedrig",                  "Batterie schwach, Lichtmaschine, Masse" },
    {  563, "Bordspannung zu hoch",                     "Laderegler defekt, Lichtmaschine" },
    {  565, "Geschwindigkeitssignal: Fehler",           "Tachogeber G22, Kabel, Kombiinstrument" },
    {  601, "Steuergeraet: Speicherfehler",             "Motorsteuergeraet defekt oder Codierung falsch" },
    {  606, "Steuergeraet: Prozessorfehler",            "Motorsteuergeraet defekt, Spannungsversorgung pruefen" },
    {  650, "MIL-Lampe: Ansteuerung Fehler",            "Kombiinstrument, Kabel, Steuergeraet" },
    {  685, "Hauptrelais: Fehler",                      "Motorsteuerrelais J271, Kabel, Sicherung" },
    { 1600, "Wegfahrsperre: Fehler",                    "Wegfahrsperre/Schluessel, Kombiinstrument" },
};

static const size_t DTC_TABLE_SIZE = sizeof(DTC_TABLE) / sizeof(DTC_TABLE[0]);

void dtcFormatCode(uint16_t rawVag, char *out, size_t outSize) {
    // 0..16383 und >= 16384+10000 sind keine gueltigen P-Codes -> VAG-Code anzeigen
    if (rawVag >= VAG_P_OFFSET && (uint32_t)rawVag - VAG_P_OFFSET <= 9999u) {
        snprintf(out, outSize, "P%04u", (unsigned)(rawVag - VAG_P_OFFSET));
    } else {
        snprintf(out, outSize, "VAG %u", (unsigned)rawVag);
    }
}

bool dtcLookup(uint16_t rawVag, const char **text, const char **cause) {
    if (rawVag == 65535) {
        *text  = "Steuergeraet defekt";
        *cause = "ECU meldet internen Fehler: Spannungsversorgung/Masse pruefen, ggf. Steuergeraet tauschen";
        return true;
    }
    if (rawVag >= VAG_P_OFFSET) {
        uint16_t pNum = rawVag - VAG_P_OFFSET;
        for (size_t i = 0; i < DTC_TABLE_SIZE; i++) {
            if (DTC_TABLE[i].pNumber == pNum) {
                *text  = DTC_TABLE[i].text;
                *cause = DTC_TABLE[i].cause;
                return true;
            }
        }
    }
    *text  = "Unbekannter Fehlercode";
    *cause = "Code nicht in Datenbank - bitte mit Reparaturleitfaden/VCDS abgleichen";
    return false;
}

const char *dtcStatusText(uint8_t status) {
    // Projekt-Konvention: 0x20 = nur sporadisch, 0x40 (auch 0x60) = statisch.
    // 0x60 enthaelt das Bit 0x20, darf aber nicht als sporadisch gelten.
    if (status & 0x40) return "statisch";
    if (status & 0x20) return "sporadisch";
    return "statisch";
}
