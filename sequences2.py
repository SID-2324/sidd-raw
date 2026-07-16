# =========================================================
# FILE: sequences2.py
# ROLE: Display-Only Label Dictionary
# =========================================================
# This file no longer controls timing or order.
# STM32 owns all sequencing decisions (see hardware2.py).
# This dictionary exists PURELY so match_log.txt and the
# terminal are human-readable.
#
# If a character isn't in this dictionary, the relay loop
# still works fine -- it just logs the raw character.
# =========================================================

CHAR_LABELS = {
    # --- Spearhead ---
    'a': "Drive straight to primary spearhead zone",
    'b': "Rotate 90 degrees to face spearhead",
    'c': "Strafe right to backup spearhead zone",
    'd': "Minor forward adjustment",
    'e': "Lower claw / grab sequence start",
    'f': "Close claw",
    'g': "Raise claw to secure spearhead",

    # --- Camera Input ---
    'h': "Navigate to camera reading zone",
    'i': "Pause to let camera frame stabilize",

    # --- Meihua Forest path entry chars ---
    'j': "Meihua Path A entry (STM32 handles 200→skip→600mm sequence)",
    'k': "Meihua Path B entry (STM32 handles 200→400→skip sequence)",
    'l': "Meihua Path C entry (STM32 handles skip→400→600mm sequence)",

    # --- Meihua Forest movement chars ---
    'm': "High-speed travel to Meihua Forest area",
    'n': "Skip this block (R1 handles it)",
    'o': "Micro-nudge for alignment",
    'p': "Extend arms to Meihua box",
    'q': "Grip box",
    'r': "Retract arms to secure box",

    # --- Tic-Tac-Toe ---
    's': "Travel to primary Tic-Tac-Toe grid location",
    't': "Extend placement mechanism",
    'u': "Release box into grid",
    'v': "Retract mechanism safely",
    'w': "Strafe sideways to the next available grid slot",

    # --- Height simulation chars ---
    'x': "Climb 200mm block",
    'y': "Climb 400mm block",
    'z': "Climb 600mm block",

    'X': "Descend 200mm block",
    'Y': "Descend 400mm block",
    'Z': "Descend 600mm block",

    # --- Sensor outcome chars ---
    'P': "Sensor: Present",
    'M': "Sensor: Missing",
    'D': "Sensor: Alignment Done",
    'N': "Sensor: Alignment Off",
    'E': "Sensor: Grid Empty",

    # --- Protocol ---
    '0': "TERMINAL: Scenario complete, no further commands",
}


def describe(cmd):
    """Returns a human-readable label for a command character."""
    return CHAR_LABELS.get(cmd, "(no description available)")


# =========================================================
# SCENARIO ENTRY POINTS
# Which character kicks off each scenario.
# Everything after that is decided by STM32.
# =========================================================
SCENARIO_START = {
    "spearhead":   'a',
    "camera":      'h',
    "meihua":      'm',   # overridden by main2.py to 'j'/'k'/'l' based on path
    "tictactoe":   's',
}