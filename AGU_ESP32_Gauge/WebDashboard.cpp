#include "WebDashboard.h"

#if ENABLE_WIFI_DASHBOARD

static const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
    <meta name="apple-mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
    <meta name="apple-mobile-web-app-title" content="1.8T AGU">
    <title>VW 1.8T AGU Live Telemetrie</title>
    <style>
        :root {
            --bg-main: #0a0e14;
            --bg-card: #151b23;
            --bg-card-border: #262c36;
            --accent-cyan: #00e5ff;
            --accent-green: #00e676;
            --accent-orange: #ff9100;
            --accent-red: #ff1744;
            --accent-blue: #2979ff;
            --text-dim: #78909c;
            --text-light: #eceff1;
        }
        * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; margin: 0; padding: 0; }
        body {
            background-color: var(--bg-main);
            color: var(--text-light);
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            padding: env(safe-area-inset-top, 16px) env(safe-area-inset-right, 12px) env(safe-area-inset-bottom, 20px) env(safe-area-inset-left, 12px);
            user-select: none;
            overflow-x: hidden;
        }
        .header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 12px 6px;
            margin-bottom: 8px;
        }
        .title-group h1 {
            font-size: 1.15rem;
            font-weight: 700;
            letter-spacing: 0.5px;
            color: #fff;
        }
        .title-group .sub {
            font-size: 0.72rem;
            color: var(--text-dim);
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        .badge {
            font-size: 0.75rem;
            font-weight: 600;
            padding: 4px 10px;
            border-radius: 12px;
            background: #1c2530;
            color: var(--text-dim);
            display: flex;
            align-items: center;
            gap: 6px;
        }
        .badge.online { color: var(--accent-green); background: rgba(0, 230, 118, 0.12); border: 1px solid rgba(0, 230, 118, 0.3); }
        .badge.offline { color: var(--accent-red); background: rgba(255, 23, 68, 0.12); }
        .badge .dot {
            width: 8px; height: 8px; border-radius: 50%; background: currentColor;
            box-shadow: 0 0 8px currentColor;
        }

        .alert-banner {
            display: none;
            background: linear-gradient(90deg, #b71c1c, #d32f2f);
            color: #fff;
            padding: 10px 14px;
            border-radius: 10px;
            margin-bottom: 12px;
            font-weight: 700;
            font-size: 0.9rem;
            text-align: center;
            animation: pulse 1s infinite alternate;
        }
        @keyframes pulse { from { opacity: 0.85; } to { opacity: 1; transform: scale(1.01); } }

        .grid {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 10px;
        }
        @media (min-width: 600px) {
            .grid { grid-template-columns: repeat(4, 1fr); gap: 12px; }
        }

        .card {
            background: var(--bg-card);
            border: 1px solid var(--bg-card-border);
            border-radius: 14px;
            padding: 12px 14px;
            display: flex;
            flex-direction: column;
            justify-content: space-between;
            min-height: 95px;
            transition: border-color 0.2s;
        }
        .card.alarm {
            border-color: var(--accent-red);
            background: rgba(255, 23, 68, 0.08);
        }
        .card-top {
            display: flex;
            justify-content: space-between;
            align-items: baseline;
        }
        .card-label {
            font-size: 0.72rem;
            font-weight: 600;
            text-transform: uppercase;
            letter-spacing: 0.8px;
            color: var(--text-dim);
        }
        .card-unit {
            font-size: 0.72rem;
            font-weight: 500;
            color: var(--text-dim);
        }
        .card-val {
            font-size: 1.85rem;
            font-weight: 800;
            font-feature-settings: "tnum";
            margin: 4px 0 2px 0;
            color: #fff;
        }
        .card-sub {
            font-size: 0.7rem;
            color: var(--text-dim);
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis;
        }

        .val-cyan { color: var(--accent-cyan); }
        .val-green { color: var(--accent-green); }
        .val-orange { color: var(--accent-orange); }
        .val-red { color: var(--accent-red); }
        .val-blue { color: var(--accent-blue); }

        .progress-bar {
            height: 4px;
            background: #202731;
            border-radius: 2px;
            margin-top: 6px;
            overflow: hidden;
        }
        .progress-fill {
            height: 100%;
            background: var(--accent-cyan);
            border-radius: 2px;
            transition: width 0.2s ease-out, background-color 0.2s;
        }

        .section-title {
            font-size: 0.8rem;
            font-weight: 700;
            text-transform: uppercase;
            letter-spacing: 1px;
            color: var(--text-dim);
            margin: 18px 6px 8px 6px;
            display: flex;
            justify-content: space-between;
        }

        .panel {
            background: var(--bg-card);
            border: 1px solid var(--bg-card-border);
            border-radius: 14px;
            padding: 14px;
            margin-bottom: 12px;
        }

        .ecu-row {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 7px 0;
            border-bottom: 1px solid rgba(255, 255, 255, 0.05);
            font-size: 0.85rem;
        }
        .ecu-row:last-child { border-bottom: none; }
        .ecu-label { color: var(--text-dim); font-weight: 500; }
        .ecu-val { font-weight: 700; font-feature-settings: "tnum"; }

        .btn-group {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 8px;
            margin-top: 10px;
        }
        button {
            background: #212b36;
            border: 1px solid #343d49;
            color: #fff;
            padding: 11px 14px;
            border-radius: 10px;
            font-size: 0.85rem;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.15s;
        }
        button:active { transform: scale(0.97); background: #2c3846; }
        button.btn-danger { background: rgba(255, 23, 68, 0.15); border-color: rgba(255, 23, 68, 0.4); color: #ff5252; }
        button.btn-danger:active { background: rgba(255, 23, 68, 0.3); }

        .dtc-list {
            margin-top: 8px;
            font-size: 0.8rem;
        }
        .dtc-item {
            padding: 6px 8px;
            margin-bottom: 4px;
            background: rgba(255, 23, 68, 0.1);
            border-left: 3px solid var(--accent-red);
            border-radius: 4px;
            display: flex;
            justify-content: space-between;
        }
        .dtc-clean {
            color: var(--accent-green);
            text-align: center;
            padding: 12px;
            font-weight: 600;
        }
    </style>
</head>
<body>
    <div class="header">
        <div class="title-group">
            <h1>VW 1.8T TELEMETRIE</h1>
            <div class="sub">Bosch Motronic M3.8.3</div>
        </div>
        <div id="statusBadge" class="badge online">
            <div class="dot"></div>
            <span id="statusText">LIVE</span>
        </div>
    </div>

    <div id="alertBanner" class="alert-banner">ACHTUNG: MOTOR-ALARM AKTIV!</div>

    <!-- 6 HAUPT-KACHELN MOTORBETRIEBSZUSTAND -->
    <div class="grid">
        <!-- 1. Ladedruck -->
        <div class="card" id="cardBoost">
            <div class="card-top">
                <span class="card-label">Ladedruck</span>
                <span class="card-unit">bar</span>
            </div>
            <div class="card-val val-cyan" id="valBoost">--</div>
            <div class="card-sub" id="subBoost">Peak: -- bar</div>
            <div class="progress-bar"><div class="progress-fill" id="barBoost" style="width: 0%;"></div></div>
        </div>

        <!-- 2. Öldruck -->
        <div class="card" id="cardOilP">
            <div class="card-top">
                <span class="card-label">Öldruck</span>
                <span class="card-unit">bar</span>
            </div>
            <div class="card-val val-green" id="valOilP">--</div>
            <div class="card-sub" id="subOilP">Min: -- bar</div>
            <div class="progress-bar"><div class="progress-fill" id="barOilP" style="width: 0%;"></div></div>
        </div>

        <!-- 3. Öltemperatur -->
        <div class="card" id="cardOilT">
            <div class="card-top">
                <span class="card-label">Öltemp</span>
                <span class="card-unit">°C</span>
            </div>
            <div class="card-val" id="valOilT">--</div>
            <div class="card-sub" id="subOilT">Max: -- °C</div>
            <div class="progress-bar"><div class="progress-fill" id="barOilT" style="width: 0%;"></div></div>
        </div>

        <!-- 4. Kühlwasser -->
        <div class="card" id="cardCoolant">
            <div class="card-top">
                <span class="card-label">Kühlwasser</span>
                <span class="card-unit">°C</span>
            </div>
            <div class="card-val val-cyan" id="valCoolant">--</div>
            <div class="card-sub">Soll: 90 °C</div>
            <div class="progress-bar"><div class="progress-fill" id="barCoolant" style="width: 0%;"></div></div>
        </div>

        <!-- 5. Ladeluft (LLK) -->
        <div class="card" id="cardIat">
            <div class="card-top">
                <span class="card-label">Ladeluft (LLK)</span>
                <span class="card-unit">°C</span>
            </div>
            <div class="card-val val-green" id="valIat">--</div>
            <div class="card-sub" id="subIat">ECU Ansaug: -- °C</div>
            <div class="progress-bar"><div class="progress-fill" id="barIat" style="width: 0%;"></div></div>
        </div>

        <!-- 6. Abgastemperatur EGT -->
        <div class="card" id="cardEgt">
            <div class="card-top">
                <span class="card-label">Abgas (EGT)</span>
                <span class="card-unit">°C</span>
            </div>
            <div class="card-val val-orange" id="valEgt">--</div>
            <div class="card-sub" id="subEgt">Max: -- °C</div>
            <div class="progress-bar"><div class="progress-fill" id="barEgt" style="width: 0%;"></div></div>
        </div>
    </div>

    <!-- ELEKTRIK & K-LINE SCHNELLÜBERSICHT -->
    <div class="section-title">ECU Live & Gemisch</div>
    <div class="panel">
        <div class="ecu-row">
            <span class="ecu-label">Lambda-Wert (&lambda;)</span>
            <span class="ecu-val val-green" id="valLambda">--</span>
        </div>
        <div class="ecu-row">
            <span class="ecu-label">Einspritzdauer</span>
            <span class="ecu-val" id="valInjection">-- ms</span>
        </div>
        <div class="ecu-row">
            <span class="ecu-label">Zündwinkel</span>
            <span class="ecu-val" id="valIgnition">-- °KW</span>
        </div>
        <div class="ecu-row">
            <span class="ecu-label">Klopfregelung (Rücknahme)</span>
            <span class="ecu-val" id="valKnock">-- °</span>
        </div>
        <div class="ecu-row">
            <span class="ecu-label">Drosselklappe</span>
            <span class="ecu-val" id="valThrottle">-- %</span>
        </div>
        <div class="ecu-row">
            <span class="ecu-label">Luftmasse (MAF)</span>
            <span class="ecu-val" id="valMaf">-- g/s</span>
        </div>
        <div class="ecu-row">
            <span class="ecu-label">Bordspannung</span>
            <span class="ecu-val" id="valVolt">-- V</span>
        </div>
    </div>

    <!-- FEHLERSPEICHER (DTC) KONSOLE -->
    <div class="section-title">
        <span>Fehlerspeicher (DTC)</span>
        <span id="dtcCountBadge" style="color: var(--accent-green);">0 FEHLER</span>
    </div>
    <div class="panel">
        <div id="dtcContainer" class="dtc-list">
            <div class="dtc-clean">Keine Fehlercodes im Speicher</div>
        </div>
        <div class="btn-group">
            <button onclick="readDtc()">DTC Auslesen</button>
            <button class="btn-danger" onclick="clearDtc()">Speicher Löschen</button>
        </div>
    </div>

    <!-- STEUERUNG GAUGE-DISPLAY -->
    <div class="section-title">Display & Peaks</div>
    <div class="panel">
        <div class="btn-group">
            <button onclick="nextDisplayTheme()">Nächstes Theme</button>
            <button onclick="resetPeaks()">Peak-Werte Reset</button>
        </div>
        <div style="margin-top: 10px; font-size: 0.72rem; color: var(--text-dim); text-align: center;" id="logInfo">
            SD-Karte: -- | Zeilen: --
        </div>
    </div>

    <script>
        let failCount = 0;

        function fetchTelemetry() {
            fetch('/api/data')
                .then(r => r.json())
                .then(data => {
                    failCount = 0;
                    document.getElementById('statusBadge').className = 'badge online';
                    document.getElementById('statusText').innerText = data.kline ? 'KW1281 OK' : 'SIMULATION';

                    // 1. Ladedruck
                    document.getElementById('valBoost').innerText = data.boost.toFixed(2);
                    document.getElementById('subBoost').innerText = 'Peak: ' + data.boostPeak.toFixed(2) + ' bar';
                    let bPct = ((data.boost + 1.0) / 3.0) * 100;
                    document.getElementById('barBoost').style.width = Math.min(Math.max(bPct, 0), 100) + '%';
                    document.getElementById('cardBoost').className = data.alarmBoost ? 'card alarm' : 'card';

                    // 2. Öldruck
                    document.getElementById('valOilP').innerText = data.oilP.toFixed(1);
                    document.getElementById('subOilP').innerText = 'Min: ' + data.oilPMin.toFixed(1) + ' bar';
                    let oPct = (data.oilP / 7.0) * 100;
                    document.getElementById('barOilP').style.width = Math.min(Math.max(oPct, 0), 100) + '%';
                    document.getElementById('cardOilP').className = data.alarmOilP ? 'card alarm' : 'card';

                    // 3. Öltemperatur
                    let otElem = document.getElementById('valOilT');
                    otElem.innerText = Math.round(data.oilT);
                    document.getElementById('subOilT').innerText = 'Max: ' + Math.round(data.oilTPeak) + ' °C';
                    let otPct = ((data.oilT - 40) / 100) * 100;
                    document.getElementById('barOilT').style.width = Math.min(Math.max(otPct, 0), 100) + '%';
                    if (data.oilT < 75) { otElem.className = 'card-val val-blue'; }
                    else if (data.alarmOilT) { otElem.className = 'card-val val-red'; }
                    else { otElem.className = 'card-val val-green'; }
                    document.getElementById('cardOilT').className = data.alarmOilT ? 'card alarm' : 'card';

                    // 4. Kühlwasser
                    document.getElementById('valCoolant').innerText = Math.round(data.coolant);
                    let cPct = ((data.coolant - 40) / 80) * 100;
                    document.getElementById('barCoolant').style.width = Math.min(Math.max(cPct, 0), 100) + '%';
                    document.getElementById('cardCoolant').className = data.alarmCool ? 'card alarm' : 'card';

                    // 5. Ladeluft (LLK)
                    document.getElementById('valIat').innerText = data.iat.toFixed(1);
                    document.getElementById('subIat').innerText = 'ECU Ansaug: ' + Math.round(data.ecuIat) + ' °C';
                    let iPct = ((data.iat - 10) / 60) * 100;
                    document.getElementById('barIat').style.width = Math.min(Math.max(iPct, 0), 100) + '%';
                    document.getElementById('cardIat').className = data.alarmIat ? 'card alarm' : 'card';

                    // 6. EGT
                    document.getElementById('valEgt').innerText = Math.round(data.egt);
                    document.getElementById('subEgt').innerText = 'Max: ' + Math.round(data.egtPeak) + ' °C';
                    let ePct = ((data.egt - 300) / 750) * 100;
                    document.getElementById('barEgt').style.width = Math.min(Math.max(ePct, 0), 100) + '%';
                    document.getElementById('cardEgt').className = data.alarmEgt ? 'card alarm' : 'card';

                    // Alarme Banner
                    let anyAlarm = data.alarmBoost || data.alarmOilP || data.alarmOilT || data.alarmCool || data.alarmIat || data.alarmEgt || data.alarmBatt;
                    document.getElementById('alertBanner').style.display = anyAlarm ? 'block' : 'none';

                    // ECU Live
                    let lamElem = document.getElementById('valLambda');
                    lamElem.innerText = data.lambda.toFixed(3);
                    lamElem.className = (data.lambda >= 0.95 && data.lambda <= 1.05) ? 'ecu-val val-green' : (data.lambda < 0.90 ? 'ecu-val val-orange' : 'ecu-val val-red');

                    document.getElementById('valInjection').innerText = data.injMs.toFixed(2) + ' ms';
                    document.getElementById('valIgnition').innerText = data.ignDeg.toFixed(1) + ' °KW';
                    document.getElementById('valKnock').innerText = data.knock.toFixed(1) + ' °';
                    document.getElementById('valThrottle').innerText = data.throttle.toFixed(1) + ' %';
                    document.getElementById('valMaf').innerText = data.maf.toFixed(1) + ' g/s';
                    document.getElementById('valVolt').innerText = data.batt.toFixed(1) + ' V';

                    // DTCs
                    let countBadge = document.getElementById('dtcCountBadge');
                    let container = document.getElementById('dtcContainer');
                    if (data.dtcCount > 0) {
                        countBadge.innerText = data.dtcCount + ' FEHLER';
                        countBadge.style.color = 'var(--accent-red)';
                        let html = '';
                        data.dtcs.forEach((d, idx) => {
                            html += '<div class="dtc-item"><span>#' + (idx+1) + ' ' + d.code + '</span><span>' + d.status + '</span></div>';
                        });
                        container.innerHTML = html;
                    } else {
                        countBadge.innerText = '0 FEHLER';
                        countBadge.style.color = 'var(--accent-green)';
                        container.innerHTML = '<div class="dtc-clean">Keine Fehlercodes im Speicher</div>';
                    }

                    // SD-Log Info
                    document.getElementById('logInfo').innerText = 'SD-Karte: ' + (data.sdReady ? 'Aktiv' : 'Keine Karte') + ' | ' + data.logLines + ' Zeilen geloggt';
                })
                .catch(() => {
                    failCount++;
                    if (failCount > 3) {
                        document.getElementById('statusBadge').className = 'badge offline';
                        document.getElementById('statusText').innerText = 'OFFLINE';
                    }
                });
        }

        function readDtc() {
            fetch('/api/dtc/read', { method: 'POST' }).then(() => fetchTelemetry());
        }
        function clearDtc() {
            if (confirm('Fehlerspeicher wirklich löschen?')) {
                fetch('/api/dtc/clear', { method: 'POST' }).then(() => fetchTelemetry());
            }
        }
        function resetPeaks() {
            fetch('/api/peaks/reset', { method: 'POST' }).then(() => fetchTelemetry());
        }
        function nextDisplayTheme() {
            fetch('/api/theme', { method: 'POST' });
        }

        // 4 Hz Aktualisierungsrate (alle 250ms)
        setInterval(fetchTelemetry, 250);
        fetchTelemetry();
    </script>
</body>
</html>
)rawliteral";

WebDashboard::WebDashboard()
    : server(WEB_SERVER_PORT),
      currentData(nullptr),
      lastClientCheck(0) {
}

void WebDashboard::begin() {
    Serial.println(F("[WebDashboard] Starte WiFi Access-Point für iPhone / Smartphone..."));

    // WiFi Access-Point starten
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS, WIFI_AP_CHANNEL);

    IPAddress ip = WiFi.softAPIP();
    Serial.printf("[WebDashboard] Access-Point gestartet! SSID: '%s' | PW: '%s'\n",
                  WIFI_AP_SSID, WIFI_AP_PASS);
    Serial.printf("[WebDashboard] iPhone URL: http://%s/\n", ip.toString().c_str());

    setupRoutes();
    server.begin();
    Serial.println(F("[WebDashboard] Webserver auf Port 80 gestartet."));
}

void WebDashboard::setupRoutes() {
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/api/data", HTTP_GET, [this]() { handleApiData(); });
    server.on("/api/dtc/read", HTTP_POST, [this]() { handleDtcRead(); });
    server.on("/api/dtc/clear", HTTP_POST, [this]() { handleDtcClear(); });
    server.on("/api/peaks/reset", HTTP_POST, [this]() { handlePeakReset(); });
    server.on("/api/theme", HTTP_POST, [this]() { handleThemeSwitch(); });

    server.onNotFound([this]() {
        server.send(404, "text/plain", "Not Found");
    });
}

void WebDashboard::handleRoot() {
    server.send_P(200, "text/html; charset=UTF-8", DASHBOARD_HTML);
}

void WebDashboard::handleApiData() {
    if (!currentData) {
        server.send(500, "application/json", "{\"error\":\"no data\"}");
        return;
    }

    // JSON String erzeugen
    char json[1024];
    char dtcJson[384] = "[]";

    if (currentData->dtcCount > 0) {
        int pos = 0;
        pos += snprintf(dtcJson + pos, sizeof(dtcJson) - pos, "[");
        for (int i = 0; i < currentData->dtcCount && i < 8; i++) {
            const char prefixes[] = {'P', 'C', 'B', 'U'};
            char prefix = prefixes[(currentData->dtcCodes[i] >> 14) & 0x03];
            uint8_t d1 = (currentData->dtcCodes[i] >> 12) & 0x03;
            uint8_t d2 = (currentData->dtcCodes[i] >> 8) & 0x0F;
            uint8_t d3 = (currentData->dtcCodes[i] >> 4) & 0x0F;
            uint8_t d4 = currentData->dtcCodes[i] & 0x0F;

            const char *st = (currentData->dtcStatus[i] & 0x20) ? "sporadisch" : "statisch";
            pos += snprintf(dtcJson + pos, sizeof(dtcJson) - pos,
                            "%s{\"code\":\"%c%d%X%X%X\",\"raw\":\"0x%04X\",\"status\":\"%s\"}",
                            (i > 0 ? "," : ""), prefix, d1, d2, d3, d4, currentData->dtcCodes[i], st);
        }
        snprintf(dtcJson + pos, sizeof(dtcJson) - pos, "]");
    }

    snprintf(json, sizeof(json),
        "{"
        "\"boost\":%.2f,\"boostPeak\":%.2f,"
        "\"oilP\":%.2f,\"oilPMin\":%.2f,"
        "\"oilT\":%.1f,\"oilTPeak\":%.1f,"
        "\"coolant\":%.1f,"
        "\"iat\":%.1f,\"ecuIat\":%.1f,"
        "\"egt\":%.1f,\"egtPeak\":%.1f,"
        "\"batt\":%.2f,"
        "\"lambda\":%.3f,\"injMs\":%.2f,\"ignDeg\":%.1f,\"knock\":%.1f,\"throttle\":%.1f,\"maf\":%.1f,"
        "\"kline\":%s,\"dtcCount\":%u,\"dtcs\":%s,"
        "\"alarmBoost\":%s,\"alarmOilP\":%s,\"alarmOilT\":%s,\"alarmCool\":%s,\"alarmIat\":%s,\"alarmEgt\":%s,\"alarmBatt\":%s,"
        "\"sdReady\":%s,\"logLines\":%lu"
        "}",
        currentData->boostBar, currentData->boostPeakBar,
        currentData->oilPressureBar, currentData->oilPressureMinBar,
        currentData->oilTemp, currentData->oilTempPeak,
        currentData->coolantTemp,
        currentData->chargeAirTemp, currentData->ecuIntakeAirTemp,
        currentData->exhaustTemp, currentData->exhaustTempPeak,
        currentData->batteryVolt,
        currentData->ecuLambda, currentData->ecuInjectionMs, currentData->ecuIgnitionTiming,
        currentData->ecuKnockRetard, currentData->ecuThrottlePct, currentData->ecuMafGs,
        currentData->klineConnected ? "true" : "false",
        currentData->dtcCount,
        dtcJson,
        currentData->alarmBoost ? "true" : "false",
        currentData->alarmOilPressure ? "true" : "false",
        currentData->alarmOilTemp ? "true" : "false",
        currentData->alarmCoolant ? "true" : "false",
        currentData->alarmChargeAir ? "true" : "false",
        currentData->alarmEgt ? "true" : "false",
        currentData->alarmBattery ? "true" : "false",
        currentData->sdCardReady ? "true" : "false",
        (unsigned long)currentData->logLinesWritten
    );

    server.send(200, "application/json", json);
}

void WebDashboard::handleDtcRead() {
    if (currentData) {
        currentData->dtcReadPending = true;
    }
    server.send(200, "application/json", "{\"status\":\"dtc_read_requested\"}");
}

void WebDashboard::handleDtcClear() {
    if (currentData) {
        currentData->dtcClearPending = true;
    }
    server.send(200, "application/json", "{\"status\":\"dtc_clear_requested\"}");
}

void WebDashboard::handlePeakReset() {
    if (currentData) {
        currentData->boostPeakBar = currentData->boostBar;
        currentData->exhaustTempPeak = currentData->exhaustTemp;
        currentData->oilTempPeak = currentData->oilTemp;
        currentData->oilPressureMinBar = currentData->oilPressureBar;
    }
    server.send(200, "application/json", "{\"status\":\"peaks_reset\"}");
}

void WebDashboard::handleThemeSwitch() {
    if (currentData) {
        int next = (int)currentData->currentTheme + 1;
        if (next >= THEME_COUNT) next = 0;
        currentData->currentTheme = (DisplayTheme)next;
    }
    server.send(200, "application/json", "{\"status\":\"theme_switched\"}");
}

void WebDashboard::update(SensorData &data) {
    currentData = &data;
    server.handleClient();
}

#endif // ENABLE_WIFI_DASHBOARD
