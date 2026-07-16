import cv2
import serial
import time
from ultralytics import YOLO

# ==========================================
# CONFIGURATION
# ==========================================
# Update with your correct COM port (e.g., 'COM3' on Windows or '/dev/ttyUSB0' on Linux)
SERIAL_PORT = 'COM3' 
BAUD_RATE = 115200

# YOLO model path and target class to track
MODEL_PATH = '"runs/detect/train/weights/best.pt"' # replace with your custom model if needed
TARGET_CLASS_ID = 0       # 0 is usually 'person' in COCO, change to your specific object ID

# Initialize Serial Communication
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Connected to STM32 on {SERIAL_PORT}")
except Exception as e:
    print(f"Error opening serial port: {e}")
    ser = None

# Initialize YOLO Model
model = YOLO(MODEL_PATH)

# Initialize Video Capture (0 is usually the default webcam)
cap = cv2.VideoCapture(0)

# Define reference bounding box parameters (Target Aligned Zone)
# This creates a central box where the object needs to be aligned
REF_BOX_WIDTH = 100  # Width of the ideal center alignment zone
REF_BOX_HEIGHT = 150 # Height of the ideal center alignment zone

while cap.isOpened():
    success, frame = cap.read()
    if not success:
        print("Failed to grab frame.")
        break

    # Get frame dimensions
    frame_height, frame_width, _ = frame.shape
    
    # Calculate the coordinates of the central reference box
    ref_x1 = int((frame_width - REF_BOX_WIDTH) / 2)
    ref_y1 = int((frame_height - REF_BOX_HEIGHT) / 2)
    ref_x2 = ref_x1 + REF_BOX_WIDTH
    ref_y2 = ref_y1 + REF_BOX_HEIGHT

    # Draw the Reference Bounding Box (Green Box = Target Zone)
    cv2.rectangle(frame, (ref_x1, ref_y1), (ref_x2, ref_y2), (0, 255, 0), 2)
    cv2.putText(frame, "Target Zone", (ref_x1, ref_y1 - 10), 
                cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)

    # Run YOLO inference on the frame
    results = model(frame, verbose=False)
    
    command_to_send = None

    # Process detections
    for result in results:
        for box in result.boxes:
            # Check if detected object matches our target class
            class_id = int(box.cls[0])
            if class_id != TARGET_CLASS_ID:
                continue

            # Get detected bounding box coordinates
            x1, y1, x2, y2 = map(int, box.xyxy[0])
            
            # Calculate the center point of the detected object
            obj_center_x = int((x1 + x2) / 2)
            obj_center_y = int((y1 + y2) / 2)

            # Draw the detected object's bounding box (Blue) and center point (Red)
            cv2.rectangle(frame, (x1, y1), (x2, y2), (255, 0, 0), 2)
            cv2.circle(frame, (obj_center_x, obj_center_y), 5, (0, 0, 255), -1)

            # --- Alignment Logic ---
            # Check if the object's center is inside the horizontal boundaries of the reference box
            if obj_center_x < ref_x1:
                command_to_send = 'L' # Object is to the left, move gripper left
                color = (0, 0, 255)   # Red text for moving
            elif obj_center_x > ref_x2:
                command_to_send = 'R' # Object is to the right, move gripper right
                color = (0, 0, 255)
            else:
                # Center X is within bounds, now check if it fits vertically or close enough
                command_to_send = 'S' # Aligned! Stop and grab
                color = (0, 255, 0)   # Green text for aligned

            # Display the current action on the frame
            cv2.putText(frame, f"CMD: {command_to_send}", (x1, y1 - 10), 
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, color, 2)
            
            # Break after the first target object found to avoid multiple commands
            break 

    # Send the command to STM32 via Serial
    if ser and command_to_send:
        try:
            # Sending command as a single byte string with newline ('L\n', 'R\n', 'S\n')
            ser.write(f"{command_to_send}\n".encode('utf-8'))
            print(f"Sent: {command_to_send}")
        except Exception as e:
            print(f"Serial write error: {e}")

    # Display the frame
    cv2.imshow("Gripper Alignment System", frame)

    # Break loop on 'q' key press
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# Cleanup
cap.release()
if ser:
    ser.close()
cv2.destroyAllWindows()