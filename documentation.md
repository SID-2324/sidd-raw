# Robocon Relay Architecture Documentation

This documentation provides a simple, high-level overview of the robot's control system. The system runs on a Jetson and communicates with an STM32 microcontroller. 

## 1. System Overview (How It Works)
The codebase uses a **"Thin Orchestrator / Relay Bridge"** architecture. 
Instead of the Jetson micromanaging every motor and delay, the **STM32 is the boss of all physical movements and timing**. The Jetson simply acts as a manager that says "Start Scenario X" and then acts as a passive relay, listening to the STM32 and logging what it does. 

The only time the Jetson makes a complex decision is during the **Camera Scenario**, where it uses AI (YOLO) to look at blocks and decide whether the robot should take Path A, B, or C.

## 2. File Explanations & Connections

Here is how the files work together:

- **`main2.py` (The Manager)**: This is the main entry point. It runs the match in four distinct phases: Spearhead, Camera, Meihua Forest, and Tic-Tac-Toe. It connects to `hardware2.py` to start these phases and to `camera.py` to get the vision-based path decision.
- **`hardware2.py` (The Communicator)**: Handles the serial connection to the STM32. It contains the "Relay Loop". When `main2.py` starts a scenario, this file sends a starting character to STM32. Then, it waits for STM32 to send back a character, logs it, and echoes it back. This continues until STM32 sends a `0` (terminal character). It also has a built-in `DRY_RUN` simulator to test code without the actual robot.
- **`camera.py` (The Eyes)**: Uses the Ultralytics YOLO AI model. When called, it takes 5 pictures (frames), looks for 'real' vs 'fake' blocks, maps them into three columns (Paths A, B, C), and uses majority voting to prevent glitches. It tells `main2.py` which path has the most 'real' blocks.
- **`sequences2.py` (The Dictionary)**: This file contains no logic. It is just a lookup table that translates the raw characters (like `a`, `j`, `P`) sent by the STM32 into human-readable English sentences for the log file (`match_log.txt`).
- **`toggle.py` (The Manual Override)**: A separate, standalone tool used for debugging. It lets you type characters manually into the terminal to trigger specific mechanisms on the STM32.

## 3. How to Execute the Code

### Running the Full Automated Match
To run the actual competition sequence:
1. Ensure the STM32 is plugged in and powered.
2. Ensure the camera is connected.
3. Open a terminal in the folder and run:
   ```bash
   python main2.py
   ```
4. To stop it at any time, press `Ctrl+C`.

### Running Manual Testing Mode
To manually test individual mechanisms or sensors without running the full sequence:
1. Open a terminal in the folder and run:
   ```bash
   python toggle.py
   ```
2. Type a character (e.g., `1`, `a`, `m`) and hit Enter to send it to the STM32.

## 4. Key Functions Breakdown

### In `hardware2.py`
- `run_relay_scenario(start_cmd, scenario_name)`: The most important function. It sends the `start_cmd` to STM32, then enters a while loop. Inside the loop, it reads the next command from STM32, logs it, and sends it back until STM32 sends `0`.
- `_log(message)`: Prints messages to the screen and saves them to `match_log.txt`.

### In `camera.py`
- `get_yolo_arrays(model, cap)`: Captures a single image, runs AI on it, and sorts detected blocks into 3 columns (A, B, C) based on their X-coordinates (left, middle, right).
- `aggregate_frames(all_grids)`: Looks at the 5 frames captured and takes a "majority vote". If a block was seen in at least 3 out of 5 frames, it counts as real.
- `decide_path()`: Looks at the final voted grids and picks the path (A, B, or C) that has the most 'real' blocks.
- `read_and_decide()`: The master camera function called by `main2.py` that ties the whole camera process together.

## 5. What to Keep in Mind (Important Settings)

Before running the code, always check these settings:

- **DRY RUN Mode**: In `hardware2.py`, there is a variable `DRY_RUN = False`. If you set it to `True`, you can run `main2.py` on your laptop without the STM32 or camera connected. It will simulate a perfect match. Make sure this is `False` during the real match!
- **Serial Ports**: 
  - In `hardware2.py`, `SERIAL_PORT = 'COM7'`. 
  - In `toggle.py`, `SERIAL_PORT = 'COM12'`. 
  - You **must** change these to match the actual COM port your STM32 is plugged into (check Device Manager on Windows).
- **Camera Index**: In `camera.py`, `CAMERA_INDEX = 1`. If your external webcam doesn't open, change this to `0` or `2`.
- **YOLO Model Path**: `camera.py` expects the AI model to be at a specific absolute path. If you move the folder, you will need to update `MODEL_PATH`.
- **Emergency Stop**: The Jetson handles `Ctrl+C` cleanly. If you press it, it will send a `Z` or `0` to the STM32 to safely stop the robot.
