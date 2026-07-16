# STM32CubeIDE Workspace Projects Overview

This document provides a high-level overview of the different projects contained in this workspace, based on their naming conventions. This workspace appears to be heavily focused on robotics components, likely for a Robocon competition (involving locomotion, sensors, and mechanisms).

## 1. Locomotion & Core Movement
*   **`f4_locomotion`**: Core driving and locomotion codebase for the STM32F4. Handles the main movement of the robot (e.g., mecanum or omni-wheel drive).
*   **`step_serv_loco`**: Combined project integrating stepper motors, servo motors, and locomotion into a single control scheme.

## 2. Motor & PID Control
*   **`RPM` / `rmp_f4` / `rpm2`**: Projects dedicated to measuring or regulating the Revolutions Per Minute (RPM) of DC motors (often using encoder feedback).
*   **`pid-pwm-claude` / `pid2`**: Implementations of PID (Proportional-Integral-Derivative) control loops to smoothly drive motors using PWM signals.
*   **`STEPPRRR` / `steprrr` / `stepper` / `nema`**: Various testing and implementation files for driving stepper motors (like NEMA 17/23), likely used for precise arm or lifting mechanisms.
*   **`stepper_esp`**: Stepper motor control specifically interfacing with an ESP module (possibly for wireless control or offloading processing).

## 3. Sensors & Feedback
*   **`Encoder` / `encoder_2`**: Projects for reading rotary encoder ticks to track how far a wheel or motor has turned (crucial for autonomous driving).
*   **`tof` / `tof_f4`**: Time-of-Flight sensor integrations (like the VL53L1X). Used for precise distance measuring, such as detecting walls or aligning with racks (Spearhead).
*   **`f4_button`**: Simple digital input handling for physical pushbuttons or limit switches.

## 4. Sub-Systems & Controllers
*   **`R2_PICKING_F4`**: A highly specific, likely integrated project for "Robot 2 (R2)" to handle its complete picking and grabbing sequence.
*   **`ps4_2` / `PS4_SERVO_STEPPER_CTRL`**: Projects for integrating a PlayStation 4 (PS4) controller. The expanded zip likely contains the full suite for driving servos and steppers manually via Bluetooth/UART from a PS4 controller.
*   **`mech_Py`**: Likely contains Python scripts related to testing or controlling the mechanisms (similar to the Jetson-STM32 serial relay architecture).
*   **`f4_vs`**: Possibly stands for "Vision System" or "Virtual Serial", handling specific comms or sensor parsing for the F4.

---
*Note: If you want a deeper analysis of the C-code inside any specific folder, just let me know the folder name!*
