/**
 * Hoodtronics-CVbot Firmware
 * - MJPEG video stream on :81, pan/tilt control API on :80.
 * - Serves the full HTML control interface at http://Hoodtronics-CV.local/
 * - Reachable by raw IP and by mDNS name (http://Hoodtronics-CV.local)
 * - No outbound push to a second host.
 *   See disabled block at the bottom for future remote push (Pi, server, etc.)
 */

#define HOSTNAME "Hoodtronics-CV"

#include <WiFiManager.h>
#include <eloquent_esp32cam.h>
#include <eloquent_esp32cam/viz/mjpeg.h>
#include <ESP32Servo.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WebServer.h>
#include <ESPmDNS.h>

using namespace eloq;
using namespace eloq::viz;

// ── Pin definitions ──────────────────────────────────────────
// GPIO 12/13 share SD card data lines on AI-Thinker.
// If an SD card is ever added, move PAN_PIN/TILT_PIN first.
#define PAN_PIN  12
#define TILT_PIN 13
#define OLED_SDA 15
#define OLED_SCL 14

// ── HTML control page (served at GET /) ──────────────────────
// Loaded into flash (PROGMEM). Students visit http://Hoodtronics-CV.local
// in any browser on the same LAN — no separate file needed.
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Hoodtronics CVbot</title>
<style>
  :root {
    --bg: #0d1117; --surface: #161b22; --border: #30363d;
    --accent: #f78166; --fg: #e6edf3; --muted: #8b949e;
    --btn: #21262d; --btn-hover: #30363d;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    background: var(--bg); color: var(--fg);
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    min-height: 100vh; display: flex; flex-direction: column; align-items: center;
    padding: 16px;
  }
  header {
    width: 100%; max-width: 680px;
    display: flex; justify-content: space-between; align-items: center;
    padding-bottom: 12px; border-bottom: 1px solid var(--border); margin-bottom: 16px;
  }
  header h1 { font-size: 1rem; font-weight: 600; letter-spacing: .04em; }
  header h1 span { color: var(--accent); }
  #status-dot {
    width: 10px; height: 10px; border-radius: 50%;
    background: #3fb950; box-shadow: 0 0 6px #3fb950;
    transition: background .4s;
  }
  #status-dot.off { background: var(--muted); box-shadow: none; }

  /* Stream */
  #stream-wrap {
    width: 100%; max-width: 640px;
    background: #000; border: 1px solid var(--border); border-radius: 8px;
    overflow: hidden; aspect-ratio: 4/3; position: relative;
  }
  #stream-wrap img {
    width: 100%; height: 100%; object-fit: contain; display: block;
  }
  #stream-error {
    display: none; position: absolute; inset: 0;
    align-items: center; justify-content: center;
    color: var(--muted); font-size: .85rem; text-align: center; padding: 16px;
  }

  /* Controls */
  .controls { width: 100%; max-width: 640px; margin-top: 18px; }
  .ctrl-row {
    display: flex; gap: 12px; align-items: center; margin-bottom: 14px;
  }
  .ctrl-row label {
    width: 46px; font-size: .78rem; color: var(--muted);
    text-transform: uppercase; letter-spacing: .06em; flex-shrink: 0;
  }
  input[type=range] {
    flex: 1; accent-color: var(--accent); height: 6px; cursor: pointer;
  }
  .val-badge {
    width: 38px; text-align: right; font-size: .82rem;
    font-variant-numeric: tabular-nums; color: var(--fg);
  }
  .arrow-group { display: flex; gap: 6px; }
  button {
    background: var(--btn); border: 1px solid var(--border);
    color: var(--fg); border-radius: 6px; padding: 6px 12px;
    font-size: .82rem; cursor: pointer; transition: background .15s;
  }
  button:hover { background: var(--btn-hover); }
  button:active { background: var(--accent); color: #fff; border-color: var(--accent); }
  #btn-center {
    width: 100%; margin-top: 4px; padding: 9px;
    font-size: .85rem; letter-spacing: .03em;
  }

  /* Footer */
  footer {
    margin-top: 20px; font-size: .75rem; color: var(--muted); text-align: center;
  }
</style>
</head>
<body>

<header>
  <h1>Hoodtronics <span>CVbot</span></h1>
  <div id="status-dot"></div>
</header>

<!-- MJPEG stream (:81 same host, different port — works in all browsers) -->
<div id="stream-wrap">
  <img id="stream" src="" alt="stream">
  <div id="stream-error">
    Stream not available.<br>Check that the bot is connected<br>and port 81 is reachable.
  </div>
</div>

<div class="controls">

  <!-- Pan -->
  <div class="ctrl-row">
    <label>Pan</label>
    <input type="range" id="pan-slider" min="0" max="180" value="90"
           oninput="onSlider('pan', this.value)">
    <span class="val-badge" id="pan-val">90°</span>
    <div class="arrow-group">
      <button onclick="step('pan', -10)">◀</button>
      <button onclick="step('pan', +10)">▶</button>
    </div>
  </div>

  <!-- Tilt -->
  <div class="ctrl-row">
    <label>Tilt</label>
    <input type="range" id="tilt-slider" min="0" max="180" value="90"
           oninput="onSlider('tilt', this.value)">
    <span class="val-badge" id="tilt-val">90°</span>
    <div class="arrow-group">
      <button onclick="step('tilt', -10)">▲</button>
      <button onclick="step('tilt', +10)">▼</button>
    </div>
  </div>

  <button id="btn-center" onclick="center()">⊕ Center</button>
</div>

<footer>
  http://Hoodtronics-CV.local &nbsp;·&nbsp; stream :81 &nbsp;·&nbsp; api :80
</footer>

<script>
  // Resolve the host so the same HTML works via IP or mDNS hostname.
  const HOST = window.location.hostname;   // e.g. "192.168.1.45" or "Hoodtronics-CV.local"
  const STREAM_URL = `http://${HOST}:81`;

  const streamImg   = document.getElementById('stream');
  const streamError = document.getElementById('stream-error');
  const dot         = document.getElementById('status-dot');

  // Start stream
  streamImg.src = STREAM_URL;
  streamImg.onload  = () => { dot.classList.remove('off'); streamError.style.display = 'none'; };
  streamImg.onerror = () => { dot.classList.add('off');    streamError.style.display = 'flex'; };

  // Debounce — avoid flooding the bot while slider is dragged
  let timers = { pan: null, tilt: null };

  function send(axis, val) {
    val = Math.max(0, Math.min(180, Math.round(val)));
    document.getElementById(`${axis}-slider`).value = val;
    document.getElementById(`${axis}-val`).textContent = val + '°';
    clearTimeout(timers[axis]);
    timers[axis] = setTimeout(() => {
      fetch(`/${axis}?val=${val}`).catch(() => dot.classList.add('off'));
    }, 40);   // 40 ms debounce — fast enough, not spammy
  }

  function onSlider(axis, val) { send(axis, parseInt(val)); }

  function step(axis, delta) {
    const cur = parseInt(document.getElementById(`${axis}-slider`).value);
    send(axis, cur + delta);
  }

  function center() { send('pan', 90); send('tilt', 90); }
</script>
</body>
</html>
)rawliteral";

// ── Objects ──────────────────────────────────────────────────
WiFiManager Hoodtronet;
Servo panServo;
Servo tiltServo;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
WebServer server(80);

bool servicesStarted = false;
int  panPos  = 90;
int  tiltPos = 90;

// ── OLED helper ──────────────────────────────────────────────
void updateOLED(String status, String ip) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);  display.println("Hoodtronics-CVbot");
  display.println("---------------------");
  display.setCursor(10, 3); display.print("Status: "); display.println(status);
  display.setCursor(0, 5);  display.print("IP: ");     display.println(ip);
  display.print("Pan: "); display.print(panPos);
  display.print("  Tilt: "); display.println(tiltPos);
  display.display();
}

// ── HTTP handlers ────────────────────────────────────────────
void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache");
  server.send_P(200, "text/html", INDEX_HTML);
}

void handlePan() {
  if (server.hasArg("val")) {
    panPos = constrain(server.arg("val").toInt(), 0, 180);
    panServo.write(panPos);
    updateOLED("Pan moving", WiFi.localIP().toString());
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
}

void handleTilt() {
  if (server.hasArg("val")) {
    tiltPos = constrain(server.arg("val").toInt(), 0, 180);
    tiltServo.write(tiltPos);
    updateOLED("Tilt moving", WiFi.localIP().toString());
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
}

// ── Setup ────────────────────────────────────────────────────
void setup() {
  delay(3000);
  pinMode(4, OUTPUT);
  Serial.begin(115200);
  Serial.println("___HOODTRONICS-CVBOT: STREAM + SERVO SERVER___");

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    Serial.println(F("SSD1306 allocation failed"));

  updateOLED("Initializing...", "0.0.0.0");

  panServo.attach(PAN_PIN);
  tiltServo.attach(TILT_PIN);
  panServo.write(panPos);
  tiltServo.write(tiltPos);

  camera.pinout.aithinker();
  camera.brownout.disable();
  camera.resolution.face();
  camera.quality.high();

  while (!camera.begin().isOk()) {
    Serial.println(camera.exception.toString());
    updateOLED("Cam Init Fail", "0.0.0.0");
    delay(1000);
  }
  Serial.println("Camera OK");

  WiFi.mode(WIFI_STA);
  Hoodtronet.setConfigPortalBlocking(true);
  Hoodtronet.setConfigPortalTimeout(180);
  Hoodtronet.setHostname(HOSTNAME);
  updateOLED("Checking WiFi", "0.0.0.0");

  if (Hoodtronet.autoConnect("HOODVISION")) {
    Serial.println("Wi-Fi connected successfully!");
    updateOLED("WiFi OK", WiFi.localIP().toString());
  } else {
    Serial.println("Portal timeout reached.");
    updateOLED("Portal Timeout", "0.0.0.0");
  }

  Hoodtronet.setConfigPortalBlocking(false);

  // mDNS — setHostname() alone sets the DHCP name only.
  // MDNS.begin() is required for .local resolution.
  if (WiFi.status() == WL_CONNECTED) {
    if (MDNS.begin(HOSTNAME)) {
      Serial.printf("mDNS started: http://%s.local\n", HOSTNAME);
    } else {
      Serial.println("mDNS failed to start");
    }
  }

  server.on("/",     HTTP_GET, handleRoot);   // ← control page
  server.on("/pan",  HTTP_GET, handlePan);
  server.on("/tilt", HTTP_GET, handleTilt);
}

// ── Loop ─────────────────────────────────────────────────────
void loop() {
  delay(1000);
  digitalWrite(4, LOW);   // suppress noise signaling

  Hoodtronet.process();

  if (WiFi.status() == WL_CONNECTED) {
    if (!servicesStarted) {
      Serial.println("WiFi Link Active. Launching servers...");

      if (mjpeg.begin().isOk()) {
        Serial.println("MJPEG stream: " + String(mjpeg.address()));
        Serial.printf("Also at: http://%s.local:81\n", HOSTNAME);
      } else {
        Serial.println(mjpeg.exception.toString());
      }

      server.begin();
      Serial.println("Control page: http://" + WiFi.localIP().toString());
      Serial.printf("Also at: http://%s.local\n", HOSTNAME);
      updateOLED("Ready", WiFi.localIP().toString());
      servicesStarted = true;
    }

    server.handleClient();
  }

  static unsigned long lastDisplayUpdate = 0;
  if (millis() - lastDisplayUpdate > 5000) {
    lastDisplayUpdate = millis();
    updateOLED(WiFi.status() == WL_CONNECTED ? "Online" : "No Network",
               WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "0.0.0.0");
    if (WiFi.status() != WL_CONNECTED) servicesStarted = false;
  }
}

/* ---------------------------------------------------------------
 * OPTIONAL: push frames to a SEPARATE physical machine.
 * Only enable if a real second device (Pi, recording server,
 * cloud endpoint) will receive frames. Do NOT point this at
 * Hoodtronics-CV itself.
 *
 * #include <HTTPClient.h>
 * #define REMOTE_UPLOAD_URL "http://<other-machine>/upload"
 * #define REMOTE_UPLOAD_INTERVAL_MS 1000
 *
 * void pushFrameToRemote() {
 *   if (!camera.capture().isOk()) { Serial.println(camera.exception.toString()); return; }
 *   HTTPClient http;
 *   http.begin(REMOTE_UPLOAD_URL);
 *   http.addHeader("Content-Type", "image/jpeg");
 *   int code = http.POST(camera.frame->buf, camera.frame->len);
 *   Serial.printf("Remote push: %d\n", code);
 *   http.end();
 * }
 * // In loop(), inside WL_CONNECTED block:
 * // static unsigned long lastPush = 0;
 * // if (millis() - lastPush > REMOTE_UPLOAD_INTERVAL_MS) { lastPush = millis(); pushFrameToRemote(); }
 * // NOTE: mjpeg and this push both call camera.capture(). Watch for
 * // frame-buffer contention when both a stream client and push are active.
 * --------------------------------------------------------------- */
