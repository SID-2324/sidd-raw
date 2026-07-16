"""
Spearhead Alignment System — Configuration
============================================
All tunable parameters live here. Adjust these to match your physical setup
(camera resolution, distance-to-target, STM32 motor speed, etc.)
"""

# ──────────────────────────────────────────────
# SERIAL / STM32
# ──────────────────────────────────────────────
SERIAL_PORT  = 'COM3'       # Change to your STM32 port (e.g. '/dev/ttyUSB0' on Linux)
BAUD_RATE    = 115200
SERIAL_TIMEOUT = 1          # seconds

# ──────────────────────────────────────────────
# YOLO MODEL
# ──────────────────────────────────────────────
MODEL_PATH      = "runs/detect/train/weights/best.pt"   # path to your trained weights
TARGET_CLASS_ID = 0          # class index for 'spearhead' in your data.yaml
CONFIDENCE_THRESHOLD = 0.5   # ignore detections below this confidence

# ──────────────────────────────────────────────
# CAMERA
# ──────────────────────────────────────────────
CAMERA_INDEX = 0             # 0 = default webcam, 1 = external USB camera

# ──────────────────────────────────────────────
# REFERENCE GRID  (fraction of frame dimensions)
# ──────────────────────────────────────────────
# The reference grid divides the frame into zones:
#
#   ┌──────┬──────────────┬──────┐
#   │      │              │      │
#   │  L   │   CENTER (S) │  R   │
#   │      │              │      │
#   └──────┴──────────────┴──────┘
#
# CENTER_ZONE_WIDTH_RATIO: width of center zone as a fraction of frame width
#   - 0.15 means the center zone is 15% of the frame width
#   - Smaller = tighter alignment required
#   - Larger  = more forgiving alignment
#
# DEAD_ZONE_RATIO: a small inner band inside the center zone
#   - If the spearhead center is within the dead zone, we consider it "locked"
#   - Prevents command oscillation at zone boundaries

CENTER_ZONE_WIDTH_RATIO  = 0.15   # center "S" zone width (fraction of frame width)
CENTER_ZONE_HEIGHT_RATIO = 0.25   # center "S" zone height (fraction of frame height)
DEAD_ZONE_RATIO          = 0.05   # dead zone width inside the center zone (fraction of frame width)

# ──────────────────────────────────────────────
# ALIGNMENT ALGORITHM (Continuous Polling)
# ──────────────────────────────────────────────
# Debounce: how many consecutive identical readings before we act
DEBOUNCE_COUNT = 3          # require N identical readings before sending a new command

# Command send interval: minimum time (seconds) between serial writes
COMMAND_INTERVAL_SEC = 0.05  # 50ms between serial commands (20 Hz max)

# ──────────────────────────────────────────────
# DISPLAY
# ──────────────────────────────────────────────
WINDOW_NAME = "Spearhead Alignment System"
SHOW_FPS    = True