import cv2
from ultralytics import YOLO

def main():
    # 1. Load your custom trained model weights
    # Update this path if your 'runs' folder layout has a different index number (e.g., train2)
    model_path = "runs/detect/train/weights/best.pt"
    model = YOLO(model_path)

    # 2. Initialize the laptop webcam
    # '0' is usually the default built-in camera index on Windows laptops
    cap = cv2.VideoCapture(1)

    if not cap.isOpened():
        print("Error: Could not open webcam.")
        return

    print("Starting webcam stream. Press 'q' to exit.")

    while True:
        ret, frame = cap.read()
        if not ret:
            print("Error: Failed to grab frame.")
            break

        # 3. Run inference on the current frame
        # conf=0.5 filters out weak predictions below 50% confidence threshold
        results = model(frame, stream=True, conf=0.5)

        # 4. Render the bounding boxes and 21 skeleton keypoints
        for result in results:
            annotated_frame = result.plot()  # Automatically draws boxes and custom joints

        # 5. Display the live video stream windows
        cv2.imshow("YOLO Pose - Live Pattern Detection", annotated_frame)

        # Break the loop immediately if the 'q' key is pressed
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    # Clean up and close windows properly
    cap.release()
    cv2.destroyAllWindows()

if __name__ == '__main__':
    main()