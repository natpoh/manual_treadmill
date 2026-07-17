import serial
import threading
import time

class SensorReader:
    def __init__(self, port, baudrate=115200, mock_mode=False):
        self.port = port
        self.baudrate = baudrate
        self.mock_mode = mock_mode
        self.serial_conn = None
        self.running = False
        self.on_trigger_callback = None
        self.on_analog_callback = None
        self.thread = None

    def set_callback(self, callback):
        """Встановлює функцію, яка буде викликатись при кожному замиканні датчика."""
        self.on_trigger_callback = callback

    def set_analog_callback(self, callback):
        """Встановлює функцію, яка буде викликатись при отриманні аналогового сигналу."""
        self.on_analog_callback = callback

    def start(self):
        self.running = True
        if self.mock_mode:
            print(f"[SensorReader] Запущено в режимі симуляції (Mock).")
            self.thread = threading.Thread(target=self._mock_loop, daemon=True)
            self.thread.start()
        else:
            try:
                self.serial_conn = serial.Serial(self.port, self.baudrate, timeout=1)
                print(f"[SensorReader] Підключено до {self.port} на швидкості {self.baudrate}.")
                self.thread = threading.Thread(target=self._read_loop, daemon=True)
                self.thread.start()
            except Exception as e:
                print(f"[SensorReader] Помилка підключення: {e}")
                self.running = False

    def stop(self):
        self.running = False
        if self.serial_conn and self.serial_conn.is_open:
            self.serial_conn.close()
        if self.thread:
            self.thread.join(timeout=1.0)
        print("[SensorReader] Зупинено.")

    def _read_loop(self):
        while self.running and self.serial_conn and self.serial_conn.is_open:
            try:
                if self.serial_conn.in_waiting > 0:
                    line = self.serial_conn.readline().decode('utf-8').strip()
                    if line:
                        if line.startswith("TICK"):
                            parts = line.split(":")
                            if len(parts) == 2:
                                try:
                                    delta_ms = int(parts[1])
                                    self._trigger(delta_ms)
                                except ValueError:
                                    pass
                        elif line.startswith("ANALOG:"):
                            parts = line.split(":")
                            if len(parts) == 2:
                                try:
                                    subparts = parts[1].split(",")
                                    val = int(subparts[0])
                                    if self.on_analog_callback:
                                        self.on_analog_callback(val)
                                except ValueError:
                                    pass
                        elif line.isdigit():
                            try:
                                val = int(line)
                                if self.on_analog_callback:
                                    self.on_analog_callback(val)
                            except ValueError:
                                pass
                        else:
                            # Виводимо все інше
                            print(f"[SensorReader RAW] Arduino каже: {line}")
            except Exception as e:
                print(f"[SensorReader] Помилка читання: {e}")
                time.sleep(1)

    def _mock_loop(self):
        """Симулює замикання датчика для тестування (наприклад, 2 рази на секунду)"""
        while self.running:
            # Симуляція часу між кроками: 500 мс
            self._trigger(500)
            time.sleep(0.5)

    def _trigger(self, delta_ms):
        if self.on_trigger_callback:
            self.on_trigger_callback(delta_ms)
