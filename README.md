# VisionBot — Computer Vision Bot

**Hoodtronics LLC — Indianapolis, IN**
`Open Source | Edge-ML Camera Platform`

![Platform](https://img.shields.io/badge/Platform-ESP32--CAM-blue)
![License](https://img.shields.io/badge/License-MIT-green)
![Status](https://img.shields.io/badge/Status-Active-brightgreen)

---

## Overview

VisionBot is a standalone 2-DOF pan/tilt camera robot built on the ESP32-CAM.
It streams live MJPEG video over Wi-Fi to any browser or ML host.
No pipeline. No cloud dependency. No RTOS required.

It is designed as a plug-and-play edge-ML camera reference platform.
Anyone can deploy it for robotics, surveillance, or machine learning research.

---

## Features

- Two-axis pan/tilt control via servo PWM
- Live MJPEG video stream over Wi-Fi (LAN)
- Browser-based web control interface — no app required
- Onboard image capture on the ESP32-CAM
- Lightweight C++ firmware — no RTOS dependency
- OLED status display
- Custom 3D-printed PETG/TPU enclosure (STL files included)
- Compatible with any ML host that accepts an MJPEG stream

---

## Hardware

| Component | Part | Spec |
|---|---|---|
| MCU / Camera | ESP32-CAM (AI-Thinker) | OV2640 2MP, 802.11 b/g/n |
| Tilt Servo | SG90 / MG90S | 5V, 0.25A |
| Pan Servo | SG90 / MG90S | 5V, 0.25A |
| Servo filter cap | 100µF electrolytic + 100nF ceramic | Per servo rail |
| Enclosure | Custom 3D-printed | PETG shell, TPU bumper |
| Power | 5V USB or 18650 Li-ion bank | 2A recommended |
| Display (optional) | 0.96″ SSD1306 OLED | I²C |

> **⚠️ Power note:** Each servo can draw up to 0.25A at stall.
> Use a dedicated 5V rail for servos — do not share with the ESP32-CAM 3.3V regulator.

---

## Platform Architecture

```
┌──────────────────────────────────────────────────────┐
│                   VisionBot (Standalone)              │
│                                                       │
│  OV2640 Camera                                        │
│      │                                                │
│      ▼                                                │
│  ESP32-CAM ──► MJPEG Stream ──► Wi-Fi (LAN)          │
│      │                              │                 │
│      ▼                              ▼                 │
│  Servo PWM                   Browser / ML Host        │
│  (Pan + Tilt)                (OpenCV, Python, etc.)   │
│                                                       │
│  OLED Display ◄─── Status feedback                   │
└──────────────────────────────────────────────────────┘
```

**Data flow:**

1. OV2640 captures frames at the configured resolution and frame rate.
2. ESP32-CAM encodes frames as MJPEG and serves them over Wi-Fi.
3. Any browser or Python/OpenCV client connects to the stream endpoint.
4. Pan/tilt commands arrive via the web interface or serial.
5. PWM signals drive the servos to the commanded angle.

**Stream endpoint:**
```
http://<ESP32-CAM-IP>:80/stream
```

**Control endpoint:**
```
http://<ESP32-CAM-IP>:80/control?pan=<0-180>&tilt=<0-180>
```

---

## Software Components

```
VisionBot/
├── Embedded Firmware/         # ESP32-CAM Arduino firmware
├── Computer_vision_interface/ # Python/OpenCV host-side scripts
├── STEM Education/            # Lesson plans and project guides
├── gimbal_case_1.stl          # 3D print — gimbal body
├── gimba_case_2.stl           # 3D print — gimbal cover
└── README.md
```

---

## Quickstart

### 1. Flash the firmware

Open the firmware sketch in Arduino IDE.
Set your Wi-Fi credentials in the sketch header.
Select **AI-Thinker ESP32-CAM** as the board.
Flash at 115200 baud.

### 2. Connect

Open a browser and navigate to the IP address printed on the OLED.
The MJPEG stream and pan/tilt controls load automatically.

### 3. Run with Python / OpenCV

```python
import cv2
cap = cv2.VideoCapture("http://<ESP32-IP>/stream")
while True:
    ret, frame = cap.read()
    if ret:
        cv2.imshow("VisionBot", frame)
    if cv2.waitKey(1) == ord('q'):
        break
```

---

## 3D Print Files

| File | Part | Material |
|---|---|---|
| `gimbal_case_1.stl` | Main gimbal body | PETG |
| `gimba_case_2.stl` | Top cover / camera mount | PETG or TPU |

Print at 20% infill, 0.2mm layer height.
TPU for the cover adds vibration damping — recommended for mobile use.

---

## Edge-ML Use Cases

| Domain | Application |
|---|---|
| **STEM / Education** | Robotics class camera platform |
| **Home Automation** | Pan/tilt security camera |
| **Object Detection** | YOLO or MobileNet inference on host |
| **Data Collection** | Labeled image dataset capture |
| **Precision Agriculture** | Field scouting camera on a fixed mount |
| **Research** | Embedded vision baseline for publications |

---

## License

Released under the **MIT License**.
See `LICENSE` for full terms.

Free to use, modify, and distribute.
Attribution to Hoodtronics LLC — Indianapolis, IN is appreciated but not required.

---

## Attribution

**Hoodtronics LLC — Indianapolis, IN**
Research & Development | Embedded AI Systems

> *"Built to measure. Improved to perform."*

---

*Open Source Edge-ML Camera Platform — Active Development*
