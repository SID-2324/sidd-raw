# =========================================================
# FILE: hardware2.py
# ROLE: The Relay Bridge
# =========================================================
# STM32 owns ALL timing and sequencing decisions.
# Jetson sends one starting character per scenario, then
# becomes a pure relay: read whatever STM32 says to run
# next, send it back, repeat -- until STM32 sends the
# terminal character ('0'), meaning the scenario is done.
# =========================================================

import sys
import time
import serial

# =========================================================
# --- CONFIGURATION ---
# Edit these before every match or test session.
# =========================================================
SERIAL_PORT = 'COM7' if sys.platform == 'win32' else '/dev/ttyACM0'
BAUD_RATE   = 115200

TERMINAL_CHAR = '0'   # STM32 sends this when a scenario is fully complete
RELAY_TIMEOUT = 15.0  # Max seconds to wait for STM32's next instruction

# --- DRY RUN ---
# Set DRY_RUN = True to simulate the full match without hardware.
# The relay loop will play back a fake STM32 response sequence
# based on FORCED_PATH below, so you can verify all 4 scenarios
# and all 3 meihua paths work correctly from your laptop.
DRY_RUN     = False   # True = no hardware needed
FORCED_PATH = 'B'    # Which meihua path to simulate: 'A', 'B', or 'C'

# =========================================================
# --- DRY RUN SIMULATOR ---
# These are the fake STM32 reply sequences used during
# dry run. Each entry is a list of chars that the simulator
# returns one-by-one, exactly as a real STM32 would.
# '0' at the end = terminal char = scenario done.
#
# Meihua heights per path (column-wise, from rulebook):
#   col1=200mm, col2=400mm, col3=600mm
#   1 = R2 picks this block, 0 = R1 handles it
#
# Path A = [1,0,1] → climb 200mm, skip 400mm, climb 600mm
# Path B = [1,1,0] → climb 200mm, climb 400mm, skip 600mm
# Path C = [0,1,1] → skip 200mm, climb 400mm, climb 600mm
# =========================================================
DRY_RUN_SEQUENCES = {
    # Spearhead: drive → sensor sees present → grab → done
    "spearhead": ['b', 'e', 'f', 'g', '0'],

    # Camera: drive → stabilize → done (path decision happens in camera.py)
    "camera":    ['i', '0'],

    # Meihua per path -- STM32 replies with climb/descend chars per block
    # x = climb 200mm, y = climb 400mm, z = climb 600mm
    # These chars are purely for dry run display -- your STM32 teammate
    # will assign real chars when he writes the C switch cases.
    "meihua_A":  ['x', 'n', 'z', '0'],   # climb 200 → skip → climb 600
    "meihua_B":  ['x', 'y', 'n', '0'],   # climb 200 → climb 400 → skip
    "meihua_C":  ['n', 'y', 'z', '0'],   # skip → climb 400 → climb 600

    # Tic-Tac-Toe: travel → grid empty → place → done
    "tictactoe": ['t', 'u', 'v', '0'],
}

# Human-readable labels for the dry run chars above
DRY_RUN_LABELS = {
    'x': "[DRY RUN] STM32 climbing 200mm block",
    'y': "[DRY RUN] STM32 climbing 400mm block",
    'z': "[DRY RUN] STM32 climbing 600mm block",
    'n': "[DRY RUN] STM32 skipping this block (R1 handles it)",
}

# =========================================================
# --- LOGGING ---
# =========================================================
LOG_FILE = "match_log.txt"

def _log(message):
    """Write timestamped message to both console and log file."""
    timestamped = f"[{time.strftime('%H:%M:%S')}] {message}"
    print(timestamped)
    with open(LOG_FILE, 'a', encoding='utf-8') as f:
        f.write(timestamped + '\n')

# =========================================================
# --- CAMERA INPUT STORAGE ---
# =========================================================
camera_input = None

def set_camera_input(value):
    global camera_input
    camera_input = value
    _log(f"Camera input set to: '{value}'")

# =========================================================
# --- 1. INITIALIZE CONNECTION ---
# =========================================================
stm32 = None

if DRY_RUN:
    _log(f" DRY RUN MODE: No hardware connection. Simulating Path '{FORCED_PATH}'.")
else:
    try:
        stm32 = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)  # Let STM32 finish its USB reset
        _log("Hardware Connection Established.")

        # Startup handshake -- confirms STM32 is alive before match starts
        stm32.reset_input_buffer()
        stm32.write('?'.encode('utf-8'))
        stm32.flush()
        start = time.time()
        handshake_ok = False
        while (time.time() - start) < 3.0:
            if stm32.in_waiting > 0:
                resp = stm32.read(1).decode('utf-8', errors='ignore')
                if resp == '!':
                    _log("STM32 Handshake SUCCESS. Robot is ready.")
                    handshake_ok = True
                    break
        if not handshake_ok:
            _log("WARNING: STM32 did not respond to handshake.")

    except Exception as e:
        _log(f"CRITICAL ERROR: Could not connect to STM32 on {SERIAL_PORT}: {e}")
        stm32 = None

# =========================================================
# --- 2. RAW SEND/RECEIVE (low-level helpers) ---
# =========================================================
def _send_char(cmd):
    """Sends one character to STM32. In DRY_RUN, just logs it."""
    if DRY_RUN:
        _log(f"    [DRY RUN] Jetson sends: '{cmd}'")
        return
    if stm32 is None:
        _log(f"    [SKIPPED] stm32 not connected. Cannot send '{cmd}'")
        return
    stm32.write(cmd.encode('utf-8'))
    stm32.flush()

def _read_char(timeout=RELAY_TIMEOUT):
    """
    Blocks until STM32 sends back exactly one character, or times out.
    Skips stray \\r and \\n bytes so STM32 debug strings don't corrupt
    the protocol. In DRY_RUN, returns None (caller handles simulation).
    """
    if DRY_RUN:
        return None  # dry run relay is handled inside run_relay_scenario

    if stm32 is None:
        _log("    [SKIPPED] stm32 not connected. Cannot read response.")
        return None

    start_time = time.time()
    while (time.time() - start_time) < timeout:
        if stm32.in_waiting > 0:
            byte = stm32.read(1).decode('utf-8', errors='ignore')
            if byte in ('\r', '\n'):
                continue  # skip formatting bytes from debug strings
            return byte

    _log(f"    TIMEOUT: STM32 did not respond within {timeout}s.")
    return None

# =========================================================
# --- 3. THE RELAY LOOP ---
# Core of the new architecture.
# =========================================================
def run_relay_scenario(start_cmd, scenario_name="Scenario", dry_run_key=None):
    """
    Kicks off a scenario by sending start_cmd, then relays whatever
    STM32 says to run next, until STM32 sends terminal char '0'.

    dry_run_key: key into DRY_RUN_SEQUENCES to use for simulation.
                 If None, the scenario completes immediately in dry run.

    Returns True if scenario completed normally, False otherwise.
    """
    _log(f" Starting relay for {scenario_name}: sending '{start_cmd}'")
    _send_char(start_cmd)

    # --- DRY RUN: play back fake STM32 sequence ---
    if DRY_RUN:
        sequence = DRY_RUN_SEQUENCES.get(dry_run_key, ['0'])
        for fake_char in sequence:
            time.sleep(0.4)  # small pause so logs are readable
            if fake_char == TERMINAL_CHAR:
                _log(f" {scenario_name} complete (STM32 sent terminal char).")
                return True
            # show a friendly label if we have one, otherwise raw char
            label = DRY_RUN_LABELS.get(fake_char, f"STM32 says run next: '{fake_char}'")
            _log(f" {label}")
            _send_char(fake_char)
        # if sequence didn't end with '0', still mark complete
        _log(f" {scenario_name} complete (end of dry run sequence).")
        return True

    # --- REAL HARDWARE: standard relay loop ---
    current_cmd = start_cmd
    step_count  = 0
    max_steps   = 50  # safety cap so a firmware bug can't loop forever

    while step_count < max_steps:
        next_cmd = _read_char()

        if next_cmd is None:
            _log(f" Relay broke: no response after '{current_cmd}'. Aborting {scenario_name}.")
            return False

        if next_cmd == TERMINAL_CHAR:
            _log(f" {scenario_name} complete (STM32 sent terminal char).")
            return True

        _log(f" STM32 says run next: '{next_cmd}'")
        _send_char(next_cmd)
        current_cmd = next_cmd
        step_count += 1

    _log(f" Relay exceeded {max_steps} steps without terminal char. Aborting {scenario_name}.")
    return False