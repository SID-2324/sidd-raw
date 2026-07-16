"""
Serial Bridge — Reliable UART communication with the STM32
=============================================================
Handles connection, reconnection, and command transmission.
"""

import serial
import time


class SerialBridge:
    """Manages serial communication with the STM32 MCU."""

    def __init__(self, port: str, baud_rate: int = 115200, timeout: float = 1.0):
        self.port = port
        self.baud_rate = baud_rate
        self.timeout = timeout
        self._ser = None
        self._connected = False

    @property
    def connected(self) -> bool:
        return self._connected and self._ser is not None and self._ser.is_open

    def connect(self) -> bool:
        """Attempt to open the serial port. Returns True on success."""
        try:
            self._ser = serial.Serial(
                self.port,
                self.baud_rate,
                timeout=self.timeout
            )
            self._connected = True
            print(f"[SERIAL] Connected to STM32 on {self.port} @ {self.baud_rate} baud")
            # Brief pause for STM32 to finish reset (some boards reset on serial open)
            time.sleep(0.5)
            return True
        except serial.SerialException as e:
            print(f"[SERIAL] Connection failed: {e}")
            self._connected = False
            return False

    def send(self, command: str) -> bool:
        """
        Send a single-character command to the STM32.
        
        Protocol: sends the command character followed by newline.
        Example: 'L\\n', 'R\\n', 'S\\n'
        
        Returns True if sent successfully.
        """
        if not self.connected:
            return False
        try:
            payload = f"{command}\n".encode('utf-8')
            self._ser.write(payload)
            self._ser.flush()
            return True
        except serial.SerialException as e:
            print(f"[SERIAL] Write error: {e}")
            self._connected = False
            return False

    def read_response(self) -> str | None:
        """Read a line from the STM32 (non-blocking, returns None if nothing)."""
        if not self.connected:
            return None
        try:
            if self._ser.in_waiting > 0:
                line = self._ser.readline().decode('utf-8', errors='replace').strip()
                return line if line else None
        except serial.SerialException:
            pass
        return None

    def close(self):
        """Close the serial connection."""
        if self._ser and self._ser.is_open:
            self._ser.close()
            print("[SERIAL] Connection closed.")
        self._connected = False