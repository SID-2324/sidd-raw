
# camera.py
# Vision input and path decision for the robot.

import time
import hardware2 as hardware

try:
    import cv2
    from ultralytics import YOLO
    AI_AVAILABLE = True
except ImportError:
    AI_AVAILABLE = False

# =========================================================
# --- CONFIGURATION ---
# =========================================================
POLL_INTERVAL = 0.2          # Seconds between YOLO polls
POLL_TIMEOUT  = 15.0         # Max wait before falling back
FALLBACK_PATH = 'A'          # Used if YOLO times out or fails
MODEL_PATH    = r"C:\Users\LENOVO\OneDrive\Documents\STM32\f4\NEW\yolo-pose-model\runs\pose\train\weights\best.pt"
CAMERA_INDEX  = 1            # 0 = built-in webcam, 1 = external camera

# Collect several frames and use majority voting to reduce false positives.
N_FRAMES = 5

# X-coordinate thresholds for path classification.
COL_LEFT_THRESHOLD  = 0.33
COL_RIGHT_THRESHOLD = 0.67

# Use relative Y-sorting within each path to assign rows.


# =========================================================
# --- YOLO INFERENCE & GRID POPULATION ---
# =========================================================
def get_yolo_arrays(model, cap):
    """
    Captures one frame, runs YOLO inference, and builds a 3x3 grid
    using relative Y-sorting for row assignment.

    Grid layout (returned as three flat lists):
      grid['A'] = [col0_closest, col1_middle, col2_farthest]
      grid['B'] = [col0_closest, col1_middle, col2_farthest]
      grid['C'] = [col0_closest, col1_middle, col2_farthest]

    Value 1 = R2 picks this block ('real' class detected here).
    Value 0 = R1 handles it / block absent.

    Returns a tuple (array_A, array_B, array_C) or None if the
    camera frame could not be read.
    """

    ret, frame = cap.read()
    if not ret:
        hardware._log(" Camera read failed (ret=False). Check CAMERA_INDEX.")
        return None

    # Run YOLO inference. verbose=False silences per-frame console spam.
    results = model(frame, conf=0.5, verbose=False)

    # -------------------------------------------------------
    # Live display window.
    # Shows bounding boxes, class labels, and keypoints so you
    # can visually confirm detections during testing.
    # -------------------------------------------------------
    for result in results:
        annotated_frame = result.plot()
    cv2.imshow("YOLO - Live Detection", annotated_frame)
    cv2.waitKey(1)   # Must be called to actually render the window.

    # -------------------------------------------------------
    # STEP 1: Collect all detections into per-path buckets.
    #
    # Each bucket holds a list of (y_center, val) tuples.
    # We do NOT assign a row index yet — that happens after
    # sorting in Step 2.
    # -------------------------------------------------------
    buckets = {'A': [], 'B': [], 'C': []}

    for result in results:
        for box in result.boxes:
            class_name = model.names[int(box.cls[0])]

            # Determine real (1) or fake (0) from the class name.
            # 'real' in the name → R2 handles this block → value 1.
            # 'fake' in the name → R1 handles it         → value 0.
            val = 1 if 'real' in class_name else 0

            # Retrieve normalised box coordinates.
            # box.xywhn → [x_centre, y_centre, width, height], all 0.0-1.0.
            x_center = float(box.xywhn[0][0])
            y_center = float(box.xywhn[0][1])

            # Debug log: print raw values so calibration is easy.
            hardware._log(
                f"  [{class_name}] val={val} "
                f"x={x_center:.2f}  y={y_center:.2f}"
            )

            # Assign to a path bucket using the X-coordinate strip rule.
            if x_center < COL_LEFT_THRESHOLD:
                path = 'A'
            elif x_center < COL_RIGHT_THRESHOLD:
                path = 'B'
            else:
                path = 'C'

            buckets[path].append((y_center, val, class_name))

    # -------------------------------------------------------
    # STEP 2: Sort each path bucket by Y descending and
    #         assign column indices by rank.
    #
    # Highest Y (= lowest on screen = closest platform) → col 0.
    # Sorting is done purely on relative order, so random block
    # heights do not affect the result.
    # -------------------------------------------------------
    grid = {'A': [0, 0, 0], 'B': [0, 0, 0], 'C': [0, 0, 0]}

    for path, detections in buckets.items():

        # Sort descending by Y so the closest box is first.
        detections.sort(key=lambda d: d[0], reverse=True)

        for rank, (y_center, val, class_name) in enumerate(detections):
            if rank >= 3:
                # More than 3 detections on one path — ignore extras.
                hardware._log(
                    f"  WARNING: More than 3 detections on Path {path}. "
                    f"Ignoring rank {rank} box ({class_name})."
                )
                break

            grid[path][rank] = val
            hardware._log(
                f"  -> Path {path} | rank {rank} | y={y_center:.2f} "
                f"| col {rank} = {val}  ({class_name})"
            )

    return (grid['A'], grid['B'], grid['C'])


# =========================================================
# --- MULTI-FRAME AGGREGATOR ---
# =========================================================
def aggregate_frames(all_grids):
    """
    Takes a list of grids collected across N frames and produces
    one final grid using MAJORITY VOTING per cell.

    For each (path, col) cell:
      - Count how many frames saw a 1 and how many saw a 0.
      - If 1s >= 0s  →  final cell value = 1.
      - If 0s >  1s  →  final cell value = 0.

    This smooths out single-frame YOLO glitches. A real block
    that was briefly missed in 2 frames but detected in 8 frames
    still correctly scores as 1.

    Parameters:
      all_grids: list of (array_A, array_B, array_C) tuples,
                 one tuple per collected frame.

    Returns:
      A single aggregated (array_A, array_B, array_C) tuple.
    """

    # Accumulate vote counts. votes[path][col] = number of frames
    # that detected a 1 at this (path, col) position.
    votes = {
        'A': [0, 0, 0],
        'B': [0, 0, 0],
        'C': [0, 0, 0],
    }

    total_frames = len(all_grids)

    for (arr_A, arr_B, arr_C) in all_grids:
        for col in range(3):
            votes['A'][col] += arr_A[col]   # adds 1 if this frame saw a 1
            votes['B'][col] += arr_B[col]
            votes['C'][col] += arr_C[col]

    # Majority vote: if more than half the frames saw a 1, the cell is 1.
    # Using >= half so that a 50/50 split favours detection (safer for R2).
    threshold = total_frames / 2

    final = {}
    for path in ('A', 'B', 'C'):
        final[path] = [
            1 if votes[path][col] >= threshold else 0
            for col in range(3)
        ]
        hardware._log(
            f"  [{path}] votes={votes[path]}  "
            f"threshold={threshold:.1f}  "
            f"final={final[path]}"
        )

    return (final['A'], final['B'], final['C'])


# =========================================================
# --- VALIDATION ---
# =========================================================
def validate_arrays(array_A, array_B, array_C):
    """
    Sanity-checks all three path arrays before using them
    for a path decision.

    Catches YOLO glitches (wrong length, non-binary values,
    None) before they can corrupt match logic.

    Returns True if all arrays are valid, False otherwise.
    """
    arrays = {'A': array_A, 'B': array_B, 'C': array_C}
    for name, arr in arrays.items():
        if arr is None or not isinstance(arr, (list, tuple)):
            hardware._log(f" Validation failed: Array {name} is None or not a list.")
            return False
        if len(arr) != 3:
            hardware._log(f" Validation failed: Array {name} has {len(arr)} elements (expected 3).")
            return False
        if not all(v in (0, 1) for v in arr):
            hardware._log(f" Validation failed: Array {name} contains non-binary values: {arr}")
            return False
    return True


# =========================================================
# --- SCORING & DECISION ---
# =========================================================
def decide_path(array_A, array_B, array_C):
    """
    Picks the best path by counting 1s (R2 blocks) in each array.
    More 1s = more R2 boxes on that path = higher priority.

    Tiebreaker: A beats B beats C (Python dict insertion order
    guarantees max() returns the first key in case of a tie).

    Returns 'A', 'B', or 'C'.
    """
    scores = {
        'A': sum(array_A),
        'B': sum(array_B),
        'C': sum(array_C),
    }
    hardware._log(f" Path scores -> A:{scores['A']}  B:{scores['B']}  C:{scores['C']}")
    chosen = max(scores, key=scores.get)
    hardware._log(f" Path {chosen} wins.")
    return chosen


# =========================================================
# --- MAIN ENTRY POINT ---
# Called by main2.py after arriving at camera zone.
# Returns 'A', 'B', or 'C' directly to main2.py.
# =========================================================
def read_and_decide():
    """
    Full pipeline:
      1. DRY RUN check   -> skip YOLO, use hardware2.FORCED_PATH
      2. AI availability -> fall back if cv2/ultralytics not installed
      3. Init model + camera
      4. Poll YOLO until valid arrays arrive (or timeout)
      5. Validate arrays
      6. Score and decide path
      7. Release camera, close display window
    """

    # DRY RUN: skip all hardware and return the configured forced path.
    if hardware.DRY_RUN:
        hardware._log(f" [DRY RUN] Bypassing YOLO. Using forced path: '{hardware.FORCED_PATH}'")
        return hardware.FORCED_PATH

    # Library check: graceful fallback if cv2/ultralytics are missing.
    if not AI_AVAILABLE:
        hardware._log(" cv2 / ultralytics not installed. Using fallback path.")
        return FALLBACK_PATH

    # Initialise YOLO model and camera.
    try:
        model = YOLO(MODEL_PATH)
        cap   = cv2.VideoCapture(CAMERA_INDEX)

        # Discard the first 5 frames while the camera sensor warms up.
        # Early frames are often underexposed or blurry.
        for _ in range(5):
            cap.read()

    except Exception as e:
        hardware._log(f" Failed to init YOLO / camera: {e}. Using fallback.")
        return FALLBACK_PATH

    hardware._log(f" Collecting {N_FRAMES} frames for majority voting...")
    all_grids = []   # stores one (array_A, array_B, array_C) per frame
    start     = time.time()

    # -------------------------------------------------------
    # MULTI-FRAME COLLECTION LOOP
    #
    # We keep running until we have collected N_FRAMES valid
    # results OR the timeout is reached (whichever comes first).
    #
    # A 'valid result' means get_yolo_arrays() returned a grid
    # (even if all cells are 0 — that is still a valid frame,
    # it just means YOLO saw nothing detectable that frame).
    # -------------------------------------------------------
    while len(all_grids) < N_FRAMES and (time.time() - start) < POLL_TIMEOUT:
        result = get_yolo_arrays(model, cap)
        if result is not None:
            all_grids.append(result)
            hardware._log(
                f" Frame {len(all_grids)}/{N_FRAMES} collected "
                f"({time.time() - start:.1f}s elapsed)"
            )
        time.sleep(POLL_INTERVAL)

    # Check if we collected enough frames to make a decision.
    if not all_grids:
        # Zero valid frames received — full timeout.
        hardware._log(f" TIMEOUT: No valid frames received within {POLL_TIMEOUT}s.")
        hardware._log(f" Defaulting to fallback path: '{FALLBACK_PATH}'")
        cap.release()
        cv2.destroyAllWindows()
        return FALLBACK_PATH

    if len(all_grids) < N_FRAMES:
        # Partial collection — timeout hit before N_FRAMES were gathered.
        # We still proceed with whatever frames we have.
        hardware._log(
            f" WARNING: Only {len(all_grids)}/{N_FRAMES} frames collected "
            f"before timeout. Proceeding with partial data."
        )

    hardware._log(f" Aggregating {len(all_grids)} frames via majority vote...")
    arrays = aggregate_frames(all_grids)

    # Release camera and close live display window cleanly.
    cap.release()
    cv2.destroyAllWindows()

    # Timeout fallback.
    if arrays is None:
        hardware._log(f" TIMEOUT: YOLO did not respond within {POLL_TIMEOUT}s.")
        hardware._log(f" Defaulting to fallback path: '{FALLBACK_PATH}'")
        return FALLBACK_PATH

    # Unpack and log the final arrays for match_log.txt review.
    array_A, array_B, array_C = arrays
    hardware._log(f" Array A: {array_A}")
    hardware._log(f" Array B: {array_B}")
    hardware._log(f" Array C: {array_C}")

    # Validate before deciding.
    if not validate_arrays(array_A, array_B, array_C):
        hardware._log(f" Validation failed. Defaulting to fallback path: '{FALLBACK_PATH}'")
        return FALLBACK_PATH

    # Score and return the chosen path.
    chosen = decide_path(array_A, array_B, array_C)
    hardware._log(f" Path decided: '{chosen}'")
    return chosen