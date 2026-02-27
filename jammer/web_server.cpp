#include "web_server.h"
#include "battery_manager.h"
#include "nrf_monitor.h"
#include "packet_injection.h"
#include "display_manager.h"

static ESP8266WebServer server(80);

void handleRoot() {
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Jammer Dashboard</title>";
  html += "<style>body { font-family: sans-serif; background: #121212; color: #ffffff; text-align: center; margin: 0; padding: 20px; }";
  html += "h1 { color: #f39c12; }";
  html += "div.stat { background: #1e1e1e; padding: 15px; margin: 10px auto; max-width: 400px; border-radius: 8px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); }";
  html += "span.ok { color: #2ecc71; font-weight: bold; } span.fail { color: #e74c3c; font-weight: bold; }";
  html += "</style></head><body>";
  html += "<h1>Jammer Dashboard</h1>";
  html += "<div class='stat'><h2>System Info</h2>";
  html += "Battery: <span id='batt'>...</span><br/>";
  html += "Uptime: <span id='uptime'>...</span>s</div>";
  html += "<div class='stat'><h2>Hardware Checks</h2>";
  html += "OLED: <span id='oled'>...</span><br/>";
    html += "NRF1 (Scan): <span id='nrf1'>...</span><br/></div>";
  html += "<script>";
  html += "setInterval(() => {";
  html += "  fetch('/api/status').then(res => res.json()).then(data => {";
  html += "    document.getElementById('batt').innerText = data.battery_pct + '% (' + data.battery_v + 'V)';";
  html += "    document.getElementById('uptime').innerText = data.uptime;";
  html += "    document.getElementById('oled').innerText = data.oled_ok ? 'OK' : 'FAIL';";
  html += "    document.getElementById('oled').className = data.oled_ok ? 'ok' : 'fail';";
  html += "    document.getElementById('nrf1').innerText = data.nrf1_ok ? 'OK' : 'FAIL';";
  html += "    document.getElementById('nrf1').className = data.nrf1_ok ? 'ok' : 'fail';";

  html += "  });";
  html += "}, 2000);";
  html += "</script></body></html>";
  server.send(200, "text/html", html);
}

void handleStatus() {
  String json = "{";
  json += "\"battery_pct\": " + String(battery_getPercent()) + ",";
  json += "\"battery_v\": " + String(battery_getVoltage(), 2) + ",";
  json += "\"uptime\": " + String(millis() / 1000) + ",";
  json += "\"oled_ok\": " + String(display_isOk() ? "true" : "false") + ",";
  json += "\"nrf1_ok\": " + String(nrf_monitor_isConnected() ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void web_server_init() {
  WiFi.softAP("Jammer_Dashboard", "");
  DBGF("[WEB] AP Started. IP: %s\n", WiFi.softAPIP().toString().c_str());

  server.on("/", handleRoot);
  server.on("/api/status", handleStatus);
  server.begin();
  DBGLN(F("[WEB] Server Started"));
}

void web_server_update() {
  server.handleClient();
}
