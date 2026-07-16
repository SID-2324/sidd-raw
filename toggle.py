import serial
import threading
import sys

# --- CONFIGURATION ---
SERIAL_PORT = 'COM12'  # Jetson COM port
BAUD_RATE = 115200

try:
    stm32 = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    stm32.dtr = True
    stm32.rts = True
    print(f"[OK] Connected to STM32 on {SERIAL_PORT}")
except Exception as e:
    print(f"[ERROR] Failed to connect: {e}")
    sys.exit(1)

# --- LISTENER THREAD (Runs in background) ---
def listen_to_stm32():
    while True:
        try:
            if stm32.in_waiting > 0:
                # Read instantly without waiting for newlines
                incoming = stm32.read(stm32.in_waiting).decode('utf-8', errors='ignore')
                if incoming:
                    print(incoming, end='', flush=True)
        except Exception:
            break

listener = threading.Thread(target=listen_to_stm32, daemon=True)
listener.start()

# --- MAIN LOOP ---
print("\n=== JETSON COMMAND CENTER ===")
print("Type 'm' -> Toggle MANUAL / AUTO Mode")
print("Type '1'-'9' -> Mechanisms 1-9")
print("Type 'a'-'z' -> Mechanisms 10-16")
print("Type '0' -> Stop / Clear")
print("Type 'q' -> Quit Script\n")

while True:
    try:
        cmd = input("Command > ").strip()
        
        if cmd.lower() == 'q':
            print("Shutting down safely...")
            stm32.write('0\n'.encode('utf-8'))
            stm32.flush()
            stm32.close()
            sys.exit(0)
            
        if len(cmd) > 0:
            # We ONLY send the very first character you type.
            # If you type "a", it sends 'a'. 
            char_to_send = cmd[0]
            stm32.write(char_to_send.encode('utf-8'))
            stm32.flush()
            
    except KeyboardInterrupt:
        stm32.write('0\n'.encode('utf-8'))
        stm32.flush()
        stm32.close()
        sys.exit(0)