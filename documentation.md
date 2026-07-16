# YOLO Vision & Gripper Alignment System Documentation

This folder contains the AI vision models and scripts used for training, testing, and running the robot's autonomous object detection and alignment system.

## 1. System Overview (How It Works)
This project uses **YOLOv8** (an AI object detection model) to detect specific objects (like "spearheads") and align a robotic gripper to pick them up. 
The system has three main capabilities:
1. **Training:** Fine-tuning the YOLO AI on a custom dataset.
2. **Testing:** Running the AI on a live webcam to visualize detections and skeleton patterns.
3. **Inference (Alignment):** Running the AI to track an object and actively sending Left/Right/Stop commands over serial to an STM32 microcontroller to center the object perfectly in the robot's gripper.

## 2. File Explanations & Connections

- **`R2-INFRENCE.py` (The Alignment Controller)**: This is the main bridge to the robot. It captures live video, runs YOLO to find the target object, and calculates the object's center point. It then compares this point to a central "Target Zone". If the object is too far left, it sends an `'L'` command over serial to the STM32. If it's too far right, it sends an `'R'`. When perfectly centered, it sends an `'S'` (Stop/Grab).
- **`webcam-pose.py` (The Live Tester)**: A simple script to just open the webcam, run the YOLO model, and draw the bounding boxes and keypoints on the screen. It doesn't send commands to the STM32, making it perfect for safely testing if the AI model is working correctly.
- **`yolo-pose.py` (The Trainer)**: This script trains a new YOLO model from scratch (using `yolov8n.pt` as a base). It is configured to run at high resolution (`imgsz=960`) to catch distant objects without crashing a smaller GPU (by keeping the batch size low).
- **`file-rename.py` (The Dataset Helper)**: A small utility script used to clean up dataset folders. It takes a folder of mixed images and text files and safely renames them in order (e.g., `spear_1.jpg`, `spear_2.txt`), which is required before annotating or training.

## 3. How to Execute the Code

### To Run the Gripper Alignment System
Make sure the STM32 is plugged in and the camera is connected.
```bash
python R2-INFRENCE.py
```
*(Press `q` on the video window to stop it).*

### To Test the AI Model Visually (No Robot)
```bash
python webcam-pose.py
```
*(Press `q` on the video window to stop it).*

### To Train a New Model
Make sure your dataset is correctly placed in `dataset/data.yaml`.
```bash
python yolo-pose.py
```

### To Rename a Dataset Folder
Open `file-rename.py`, change `target_folder` on line 37 to your actual folder path, and run:
```bash
python file-rename.py
```

## 4. Key Functions Breakdown

### In `R2-INFRENCE.py`
- **Reference Box Logic:** It creates a virtual rectangle in the center of the camera frame (`REF_BOX_WIDTH = 100`, `REF_BOX_HEIGHT = 150`).
- **Detection & Tracking:** It loops over `results.boxes`, finds the center of the first object matching `TARGET_CLASS_ID`, and checks if `obj_center_x` is inside the reference box.
- **Serial Communication:** Uses `ser.write()` to send `L\n`, `R\n`, or `S\n` to the STM32.

### In `yolo-pose.py`
- **`model.train()`**: Configures the YOLO training parameters. Note the `imgsz=960` for high resolution and `batch=6` to prevent Out Of Memory (OOM) errors on the graphics card.

## 5. What to Keep in Mind (Important Settings)

- **COM Port (`R2-INFRENCE.py`)**: You **must** update `SERIAL_PORT = 'COM3'` on line 10 to match your actual STM32 COM port before running the alignment script.
- **Camera Index**: 
  - `webcam-pose.py` uses `cv2.VideoCapture(1)`.
  - `R2-INFRENCE.py` uses `cv2.VideoCapture(0)`.
  - If your camera doesn't turn on, you may need to swap `0` and `1` depending on which USB port you plugged it into.
- **Model Paths**: Both inference scripts point to `"runs/detect/train/weights/best.pt"`. Ensure your newly trained models are actually saved there, or update this path.
- **Target Folder (`file-rename.py`)**: Currently, it is hardcoded to `C:\Users\Riyan\OneDrive\Desktop\spearhead`. Do not run this script blindly, or it will try to rename files in a folder that might not exist or isn't intended to be renamed.
