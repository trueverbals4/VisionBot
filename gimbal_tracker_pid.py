"""
VisionBot — PID Gimbal Tracker (Host-Side)
Hoodtronics LLC — Indianapolis, IN

Reads the ESP32-CAM MJPEG stream, runs YOLOv8 inference,
and drives 2-DOF pan/tilt servos via HTTP PID control.

Standalone object follower — works with any VisionBot MJPEG stream.
Not part of the ImaGenAI Cacadu/LoRa pipeline.

NSF SBIR Phase I — Hoodtronics LLC
PI: Tolib B. Sanni, M.S. | UEI: Z3TLHYQ1QMT3
"""

import cv2
import time
import requests
from ultralytics import YOLO

# ── ESP32-CAM Target ──────────────────────────────────────────────────────────
ESP32_IP     = "192.168.1.100"          # Update to your CVbot's DHCP address
STREAM_URL   = f"http://{ESP32_IP}:81/"
PAN_URL      = f"http://{ESP32_IP}/pan"   # Matches Hoodtronics_CVbot_LAN.ino /pan?val=X
TILT_URL     = f"http://{ESP32_IP}/tilt"  # Matches Hoodtronics_CVbot_LAN.ino /tilt?val=X

# ── Model ─────────────────────────────────────────────────────────────────────
MODEL_PATH      = "gimbal_tracker/custom_object_v1/weights/best.pt"
TARGET_CLASS_ID = 0          # Class index from your dataset

# ── PID Gains ─────────────────────────────────────────────────────────────────
KP_PAN,  KI_PAN,  KD_PAN  = 0.045, 0.001, 0.015
KP_TILT, KI_TILT, KD_TILT = 0.045, 0.001, 0.015

# ── Direction Polarity ────────────────────────────────────────────────────────
PAN_DIR  = -1   # Flip if pan tracks backwards
TILT_DIR =  1   # Flip if tilt tracks backwards

# ── Deadband — suppresses SG90 jitter below this pixel error ─────────────────
DEADBAND_PX = 12


class PIDController:
    def __init__(self, kp, ki, kd):
        self.kp = kp
        self.ki = ki
        self.kd = kd
        self.prev_error = 0.0
        self.integral   = 0.0
        self.last_time  = time.time()

    def compute(self, error):
        now        = time.time()
        dt         = max(now - self.last_time, 1e-3)
        self.integral   += error * dt
        derivative       = (error - self.prev_error) / dt
        output           = (self.kp * error) + (self.ki * self.integral) + (self.kd * derivative)
        self.prev_error  = error
        self.last_time   = now
        return output


def send_servo_angles(pan, tilt):
    pan  = int(max(0, min(180, round(pan))))
    tilt = int(max(0, min(180, round(tilt))))
    try:
        requests.get(PAN_URL,  params={"val": pan},  timeout=0.05)
        requests.get(TILT_URL, params={"val": tilt}, timeout=0.05)
    except requests.exceptions.RequestException:
        pass   # Drop non-critical servo timeouts silently


def main():
    model = YOLO(MODEL_PATH)
    cap   = cv2.VideoCapture(STREAM_URL)

    pan_angle  = 90.0
    tilt_angle = 90.0
    send_servo_angles(pan_angle, tilt_angle)

    pan_pid  = PIDController(KP_PAN,  KI_PAN,  KD_PAN)
    tilt_pid = PIDController(KP_TILT, KI_TILT, KD_TILT)

    while True:
        ret, frame = cap.read()
        if not ret:
            continue

        h, w, _  = frame.shape
        center_x = w // 2
        center_y = h // 2

        results = model.predict(frame, imgsz=320, conf=0.5, verbose=False)
        boxes   = results[0].boxes

        if len(boxes) > 0:
            best_box  = None
            best_conf = 0.0

            for box in boxes:
                cls_id = int(box.cls[0])
                conf   = float(box.conf[0])
                if cls_id == TARGET_CLASS_ID and conf > best_conf:
                    best_conf = conf
                    best_box  = box.xyxy[0].cpu().numpy()

            if best_box is not None:
                x1, y1, x2, y2 = best_box
                obj_x = int((x1 + x2) / 2)
                obj_y = int((y1 + y2) / 2)

                error_x = obj_x - center_x
                error_y = obj_y - center_y

                if abs(error_x) > DEADBAND_PX:
                    pan_angle  += PAN_DIR  * pan_pid.compute(error_x)
                if abs(error_y) > DEADBAND_PX:
                    tilt_angle += TILT_DIR * tilt_pid.compute(error_y)

                pan_angle  = max(0.0, min(180.0, pan_angle))
                tilt_angle = max(0.0, min(180.0, tilt_angle))

                send_servo_angles(pan_angle, tilt_angle)

                cv2.rectangle(frame,
                               (int(x1), int(y1)), (int(x2), int(y2)),
                               (0, 255, 0), 2)
                cv2.line(frame,
                          (center_x, center_y), (obj_x, obj_y),
                          (0, 0, 255), 2)

        cv2.imshow("VisionBot — 2-DOF Gimbal Tracker", frame)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
