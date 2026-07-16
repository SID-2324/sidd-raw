# =========================================================
# FILE: main2.py
# ROLE: The Strategy Manager (Thin Orchestrator)
# =========================================================
# main2.py no longer decides delays or step order.
# It kicks off each of the 4 scenarios in sequence and
# lets STM32 drive everything in between via the relay loop.
#
# The only "smart" thing main2.py does is pass the camera
# decision (A/B/C) to the correct meihua relay start char.
# =========================================================

import sys
import hardware2 as hardware
import camera
from sequences2 import SCENARIO_START

# Meihua path → STM32 entry character mapping
# 'j' = Path A, 'k' = Path B, 'l' = Path C
# These chars kick off the correct climbing sequence on STM32
MEIHUA_PATH_CHAR = {
    'A': 'j',
    'B': 'k',
    'C': 'l',
}

# Dry run sequence key for each meihua path
MEIHUA_DRY_KEY = {
    'A': 'meihua_A',
    'B': 'meihua_B',
    'C': 'meihua_C',
}

def match_start():
    hardware._log("\n === ROBOCON MATCH STARTED (RELAY MODE) ===\n")

    # ---------------------------------------------------------
    # SCENARIO 1: Spearhead
    # ---------------------------------------------------------
    #hardware._log("\n--- SPEARHEAD ---")
    #ok = hardware.run_relay_scenario(
    #    SCENARIO_START["spearhead"],
    #    "Spearhead",
    #    dry_run_key="spearhead"
    #)
    #if not ok:
    #    hardware._log(" WARNING: Spearhead did not complete cleanly. Proceeding anyway.")

    # ---------------------------------------------------------
    # SCENARIO 2: Camera Input
    # STM32 drives robot to camera zone first.
    # Then Jetson reads the YOLO output and decides path.
    # ---------------------------------------------------------
    hardware._log("\n--- CAMERA INPUT ---")
    ok = hardware.run_relay_scenario(
        SCENARIO_START["camera"],
        "Drive to Camera Zone",
        dry_run_key="camera"
    )
    if not ok:
        hardware._log(" WARNING: Camera travel did not complete cleanly. Proceeding anyway.")

    # Jetson asks YOLO / uses forced path in dry run
    chosen_path = camera.read_and_decide()

    # Resolve which char and dry run key to use for meihua
    meihua_char    = MEIHUA_PATH_CHAR.get(chosen_path, 'j')  # default A if unknown
    meihua_dry_key = MEIHUA_DRY_KEY.get(chosen_path, 'meihua_A')

    if chosen_path not in MEIHUA_PATH_CHAR:
        hardware._log(f" WARNING: Unknown path '{chosen_path}'. Defaulting to Path A.")

    # ---------------------------------------------------------
    # SCENARIO 3: Meihua Forest
    # STM32 receives 'j'/'k'/'l' and internally knows which
    # heights to climb/descend for that path.
    # ---------------------------------------------------------
    hardware._log(f"\n--- MEIHUA FOREST (PATH {chosen_path}) ---")
    ok = hardware.run_relay_scenario(
        meihua_char,
        f"Meihua Forest Path {chosen_path}",
        dry_run_key=meihua_dry_key
    )
    if not ok:
        hardware._log(" WARNING: Meihua Forest did not complete cleanly. Proceeding anyway.")

    # ---------------------------------------------------------
    # SCENARIO 4: Tic-Tac-Toe
    # ---------------------------------------------------------
    hardware._log("\n--- TIC-TAC-TOE ---")
    ok = hardware.run_relay_scenario(
        SCENARIO_START["tictactoe"],
        "Tic-Tac-Toe",
        dry_run_key="tictactoe"
    )
    if not ok:
        hardware._log(" WARNING: Tic-Tac-Toe did not complete cleanly.")

    hardware._log("\n === MATCH COMPLETE ===")


if __name__ == "__main__":
    try:
        match_start()
    except KeyboardInterrupt:
        hardware._log("\n Match aborted by Emergency Stop (Ctrl+C).")
        if hardware.stm32:
            hardware.stm32.write('Z'.encode('utf-8'))
        sys.exit(0)