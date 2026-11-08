/**
 * CVisionBot Cacadu Layer 1 — LAN Stream + Edge Impulse Data Capture
 * ============================================================
 * Hardware : AI Thinker ESP32-CAM
 * Library  : EloquentEsp32cam (eloquentarduino) — install via
 *            Arduino Library Manager
 *            Search: "EloquentEsp32cam"
 *
 * What it does
 * ────────────
 *  • Streams live MJPEG video on your local LAN (port 81).
 *  • On Serial command 'c', switches to 96×96, captures one
 *    JPEG frame, and outputs raw bytes for Edge Impulse CLI
 *    data forwarder.
 *  • On Serial command 's', prints WiFi RSSI + frame count.
 *
 * Open Serial Monitor @ 115200 baud.
 * Visit   http://<IP>:81   in a browser to view the stream.
 *
 * Flash settings (Arduino IDE):
 *   Board     : AI Thinker ESP32-CAM
 *   Partition : Huge APP (3MB No OTA)
 *   Port      : select your USB-to-UART  com port
 *
 *   [https://github.com/trueverbals4](https://github.com/trueverbals4)Hoodtronics.com
 * ============================================================
 */

// ── Library & Model ─────────────────────────────────────────
#define CAMERA_MODEL_AI_THINKER
#include <WiFi.h>
#include <eloquent_esp32cam.h>
#include <eloquent_esp32cam/viz/mjpeg.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

using eloq::camera;
using eloq::viz::mjpeg;

// ── WiFi Credentials ────────────────────────────────────────
// Replace with your LAN SSID and password.
//const char* SSID     = "YOUR_SSID";
//const char* PASSWORD = "YOUR_PASSWORD";

// ── State ───────────────────────────────────────────────────
uint32_t frameCount    = 0;
bool     eiCaptureMode = false;   // true while capturing for EI

// ============================================================
void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println();
    Serial.println("╔══════════════════════════════════╗");
    Serial.println("║  CVisionBot Layer 1 — LAN Test   ║");
    Serial.println("╚══════════════════════════════════╝");

    // ── Camera init ─────────────────────────────────────────
    camera.pinout.aithinker();
    camera.brownout.disable();
    camera.resolution.vga();          // 640×480 for LAN stream
    camera.quality.at(12);            // JPEG quality 1–63 (lower = larger file)

    while (!camera.begin().isOk()) {
        Serial.println("[CAM ERR] " + camera.exception.toString());
        delay(1000);
    }
    Serial.println("[CAM]  Initialized OK");

    // ── WiFi ────────────────────────────────────────────────
  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wm;
bool res;
    // reset settings - wipe stored credentials for testing
    // these are stored by the esp library
    // wm.resetSettings(); uncomment ( remove // from this line to reset wifi credentials the place back and reboot)
    res = wm.autoConnect("2DoFCVBOT","computervision"); // password protected phone app
    
    uint8_t tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 40) {
        delay(500);
        Serial.print(".");
        tries++;
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WiFi] FAILED — check SSID/password. Halting.");
        while (true) delay(1000);
    }

    Serial.println("[WiFi] Connected");
    Serial.println("[WiFi] LAN IP  : " + WiFi.localIP().toString());
    Serial.println("[WiFi] RSSI    : " + String(WiFi.RSSI()) + " dBm");

    // ── MJPEG stream server ──────────────────────────────────
    while (!mjpeg.begin().isOk()) {
        Serial.println("[MJPEG ERR] " + mjpeg.exception.toString());
        delay(1000);
    }

    Serial.println();
    Serial.println("┌────────────────────────────────────┐");
    Serial.println("│  Stream :  http://" + WiFi.localIP().toString() + ":" + String(mjpeg.port()) + " │");
    Serial.println("└────────────────────────────────────┘");
    Serial.println();
    Serial.println("Commands (send over Serial):");
    Serial.println("  c  — capture 96×96 frame for Edge Impulse");
    Serial.println("  s  — status (RSSI, frame count)");
}

// ============================================================
void loop() {

    // ── Serial command handler ───────────────────────────────
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 'c') captureForEdgeImpulse();
        if (cmd == 's') printStatus();
    }

    // ── Stream one MJPEG frame ───────────────────────────────
    if (!eiCaptureMode) {
        if (!camera.capture().isOk()) return;
        mjpeg.send(camera.frame);
        frameCount++;
    }
}

// ── Capture one 96×96 frame for Edge Impulse data forwarder ─
void captureForEdgeImpulse() {
    eiCaptureMode = true;

    // Switch to Edge Impulse-compatible resolution
    camera.resolution.face();          // 96×96
    delay(200);                        // let sensor settle

    if (!camera.capture().isOk()) {
        Serial.println("[EI ERR] " + camera.exception.toString());
        camera.resolution.vga();
        eiCaptureMode = false;
        return;
    }

    // Output header + raw JPEG bytes to Serial.
    // Edge Impulse CLI data forwarder reads this stream.
    Serial.printf("\n[EI] Frame #%u  %u bytes\n", frameCount, camera.frame->len);
    Serial.write(camera.frame->buf, camera.frame->len);
    Serial.println("\n[EI] Done — label this sample in Edge Impulse Studio.");

    // Restore streaming resolution
    camera.resolution.vga();
    eiCaptureMode = false;
}

// ── Status print ─────────────────────────────────────────────
void printStatus() {
    Serial.println("\n── Status ──────────────────────");
    Serial.println("  IP         : " + WiFi.localIP().toString());
    Serial.println("  RSSI       : " + String(WiFi.RSSI()) + " dBm");
    Serial.println("  Frames     : " + String(frameCount));
    Serial.println("  Free heap  : " + String(ESP.getFreeHeap()) + " B");
    Serial.println("────────────────────────────────\n");
}
