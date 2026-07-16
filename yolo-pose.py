from ultralytics import YOLO

def main():
    # Using the Small model for better feature retention of distant objects
    model = YOLO("yolov8n.pt") 

    # Train the model
    results = model.train(
        data="dataset/data.yaml", 
        epochs=50,
        imgsz=960,         # High resolution to prevent distant objects from blurring
        batch=6,           # Kept low to prevent RTX 3050 VRAM OOM with AdamW + 960 imgsz
        device=0,         
        workers=4,        
    )

if __name__ == '__main__':
    main()