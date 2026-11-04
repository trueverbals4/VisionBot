# Computer_Vision_Bot

![Platform](https://img.shields.io/badge/platform-ESP32--CAM-blue)
![License](https://img.shields.io/badge/license-Proprietary-red)
![Status](https://img.shields.io/badge/status-Active-green)

**Edge-computing pan/tilt camera robot** — Hoodtronics LLC

VisionBot is a 2-DOF autonomous camera platform built on ESP32-CAM.
It streams live video locally over LAN and serves as  a standalone edge devise or Layer I
of the Hoodtronics ImageGenAI platform pipeline.

---

## Features

- 2-axis pan/tilt control via servo PWM
- Live MJPEG video stream over Wi-fi LAN WEB control interface
- ESP32-CAM onboard image capture
- Lightweight C++ firmware — no RTOS dependency
- Designed for edge-AI integration (Layer I → Cacadu Codec)
- OLED display
---

## Hardware

| Component | Spec | Support component |
|---|---|
| MCU | ESP32-CAM (AI-Thinker) |
| Camera | OV2640 2MP |
| Tilt_servo | SVG90 / MG90S |5v|.25A |100uF_Ceram_Cap & 100nF_elect_Cap |
| Tan_servo | SVG90 / MG90S |5v|.25A |100uF_Ceram_Cap & 100nF_elect_Cap |
| Enclosure | Custom 3D PETG & TPU | design_ame.stl |
| Power_supply| 5V_USB 2(18650_Li_Batt_bank |
| Hoodtronics_contorl_board Not Neccessary|  
---

## Platform Architecture
