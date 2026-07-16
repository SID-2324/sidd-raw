"""
Spearhead Alignment System — Main Loop
========================================
Runs the full detection → classification → command → serial pipeline.
Continuously polls — never locks. Always tracks the CLOSEST spearhead.

Usage
-----
    python main.py                    # run with defaults from config.py
    python main.py --no-serial        # run without STM32 (testing / demo mode)
    python main.py --camera 1         # use camera index 1
    python main.py --model path.pt    # override model path
"""

import argparse
import time
import sys

import cv2
from ultralytics import YOLO

from config import (
    SERIAL_PORT, BAUD_RATE, SERIAL_TIMEOUT,
    MODEL_PATH, TARGET_CLASS_ID, CONFIDENCE_THRESHOLD,
    CAMERA_INDEX,
    CENTER_ZONE_WIDTH_RATIO, CENTER_ZONE_HEIGHT_RATIO, DEAD_ZONE_RATIO,
    DEBOUNCE_COUNT, COMMAND_INTERVAL_SEC,
    WINDOW_NAME, SHOW_FPS,
)
from grid import ReferenceGrid
from controller import AlignmentController
from serial_bridge import SerialBridge


def parse_args():
    p = argparse.ArgumentParser(description="Spearhead Alignment System")
    p.add_argument("--no-serial", action="store_true",
                   help="Run without serial connection (demo/testing mode)")
    p.add_argument("--camera", type=int, default=CAMERA_INDEX,
                   help=f"Camera index (default: {CAMERA_INDEX})")
    p.add_argument("--model", type=str, default=MODEL_PATH,
                   help=f"YOLO model path (default: {MODEL_PATH})")
    return p.parse_args()


def main():
    args = parse_args()

    # ── 1. Load YOLO model ──
    print(f"[INIT] Loading YOLO model: {args.model}")
    try:
        model = YOLO(args.model)
    except Exception as e:
        print(f"[ERROR] Failed to load model: {e}")
        sys.exit(1)

    # ── 2. Open camera ──
    print(f"[INIT] Opening camera index {args.camera}")
    cap = cv2.VideoCapture(args.camera)
    if not cap.isOpened():
        print("[ERROR] Could not open webcam.")
        sys.exit(1)

    # Read one frame to get dimensions
    ret, frame = cap.read()
    if not ret:
        print("[ERROR] Could not read from webcam.")
        sys.exit(1)
    frame_h, frame_w = frame.shape[:2]
    print(f"[INIT] Frame size: {frame_w}x{frame_h}")

    # ── 3. Build reference grid ──
    grid = ReferenceGrid(
        frame_width=frame_w,
        frame_height=frame_h,
        center_w_ratio=CENTER_ZONE_WIDTH_RATIO,
        center_h_ratio=CENTER_ZONE_HEIGHT_RATIO,
        dead_zone_ratio=DEAD_ZONE_RATIO,
    )
    print(f"[INIT] Reference grid created:")
    print(f"       Center zone: ({grid.center_x1},{grid.center_y1}) → ({grid.center_x2},{grid.center_y2})")
    print(f"       Dead zone:   ({grid.dead_x1},{grid.dead_y1}) → ({grid.dead_x2},{grid.dead_y2})")

    # ── 4. Build alignment controller (continuous, no lock) ──
    controller = AlignmentController(
        debounce_count=DEBOUNCE_COUNT,
        command_interval=COMMAND_INTERVAL_SEC,
    )

    # ── 5. Connect to STM32 ──
    bridge = None
    if not args.no_serial:
        bridge = SerialBridge(SERIAL_PORT, BAUD_RATE, SERIAL_TIMEOUT)
        if not bridge.connect():
            print("[WARN] Could not connect to STM32. Continuing in display-only mode.")
            bridge = None
    else:
        print("[INIT] Running in --no-serial mode (no STM32 commands).")

    # ── 6. Main loop ──
    print(f"\n[RUN] Starting continuous alignment loop. Press 'q' to quit, 'r' to reset.\n")
    fps_time = time.time()
    fps_count = 0
    fps_display = 0.0

    while cap.isOpened():
        ret, frame = cap.read()
        if not ret:
            print("[ERROR] Frame grab failed.")
            break

        # ── FPS counter ──
        fps_count += 1
        elapsed = time.time() - fps_time
        if elapsed >= 1.0:
            fps_display = fps_count / elapsed
            fps_count = 0
            fps_time = time.time()

        # ── Run YOLO detection ──
        results = model(frame, verbose=False, conf=CONFIDENCE_THRESHOLD)

        raw_cmd = None
        in_dead = False
        closest_detection = None
        all_detections = []

        for result in results:
            for box in result.boxes:
                class_id = int(box.cls[0])
                if class_id != TARGET_CLASS_ID:
                    continue

                conf = float(box.conf[0])
                x1, y1, x2, y2 = map(int, box.xyxy[0])
                obj_cx = (x1 + x2) // 2
                obj_cy = (y1 + y2) // 2
                area = (x2 - x1) * (y2 - y1)  # bbox area = proxy for closeness

                detection = {
                    'x1': x1, 'y1': y1, 'x2': x2, 'y2': y2,
                    'cx': obj_cx, 'cy': obj_cy, 'conf': conf,
                    'area': area,
                }
                all_detections.append(detection)

                # Pick the CLOSEST spearhead = LARGEST bounding box area
                if closest_detection is None or area > closest_detection['area']:
                    closest_detection = detection

        # ── Classify position of the CLOSEST spearhead against reference grid ──
        if closest_detection:
            cx, cy = closest_detection['cx'], closest_detection['cy']
            raw_cmd = grid.classify(cx, cy)
            in_dead = grid.is_in_dead_zone(cx, cy)

        # ── Feed into alignment controller (continuous, never locks) ──
        cmd_to_send = controller.update(raw_cmd, in_dead)

        # ── Send command to STM32 ──
        if bridge and cmd_to_send:
            success = bridge.send(cmd_to_send)
            status = "OK" if success else "FAIL"
            print(f"  → STM32: {cmd_to_send}  [{status}]")

        # ── Draw overlays ──
        grid.draw(frame, controller.active_command, locked=False)

        # Draw ALL detected spearheads (dimmed) so you can see what's being ignored
        for det in all_detections:
            if det is not closest_detection:
                # Non-target spearheads: thin gray box
                cv2.rectangle(frame, (det['x1'], det['y1']), (det['x2'], det['y2']),
                              (100, 100, 100), 1)
                cv2.putText(frame, f"{det['conf']:.0%}",
                            (det['x1'], det['y1'] - 5),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.4, (100, 100, 100), 1, cv2.LINE_AA)

        # Draw the CLOSEST (target) spearhead with full highlighting
        if closest_detection:
            d = closest_detection
            # Bounding box color based on command
            if controller.active_command == 'S':
                box_color = (0, 255, 0)     # green = aligned
            elif controller.active_command == 'L':
                box_color = (0, 100, 255)   # orange = left
            elif controller.active_command == 'R':
                box_color = (255, 100, 0)   # blue-ish = right
            else:
                box_color = (200, 200, 200) # gray = debouncing

            cv2.rectangle(frame, (d['x1'], d['y1']), (d['x2'], d['y2']),
                          box_color, 3)  # thicker box for the target
            cv2.circle(frame, (d['cx'], d['cy']), 6, (0, 0, 255), -1)  # red center dot
            cv2.circle(frame, (d['cx'], d['cy']), 6, (255, 255, 255), 1)  # white ring

            # Labels
            cv2.putText(frame, f"{d['conf']:.0%} [CLOSEST]",
                        (d['x1'], d['y1'] - 8),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, box_color, 1, cv2.LINE_AA)

        # ── HUD info ──
        hud_y = frame_h - 15
        hud_items = []
        if SHOW_FPS:
            hud_items.append(f"FPS: {fps_display:.0f}")
        hud_items.append(f"CMD: {controller.active_command or '---'}")
        hud_items.append(f"Detected: {len(all_detections)}")
        if bridge:
            hud_items.append(f"STM32: {'ON' if bridge.connected else 'OFF'}")
        else:
            hud_items.append("STM32: N/A")
        hud_items.append("MODE: CONTINUOUS")

        hud_text = "  |  ".join(hud_items)
        # Background bar
        cv2.rectangle(frame, (0, frame_h - 30), (frame_w, frame_h), (0, 0, 0), cv2.FILLED)
        cv2.putText(frame, hud_text, (10, hud_y),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (220, 220, 220), 1, cv2.LINE_AA)

        # ── Show frame ──
        cv2.imshow(WINDOW_NAME, frame)

        # ── Keyboard input ──
        key = cv2.waitKey(1) & 0xFF
        if key == ord('q'):
            break
        elif key == ord('r'):
            controller.reset()
            print("[RESET] Alignment state reset.")

    # ── Cleanup ──
    print("\n[SHUTDOWN] Cleaning up...")
    cap.release()
    if bridge:
        bridge.close()
    cv2.destroyAllWindows()
    print("[DONE]")


if __name__ == '__main__':
    main()