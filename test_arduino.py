import time
import argparse
import sys
from src.sensor_reader import SensorReader

def test_callback(delta_ms):
    print(f"Отримано TICK! Час між кроками: {delta_ms} мс", flush=True)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Тестування підключення до Ардуіно")
    parser.add_argument('--port', type=str, default='COM3', help='COM порт (за замовчуванням COM3)')
    args = parser.parse_args()

    print(f"Тестування підключення до Ардуіно на порту {args.port}...", flush=True)
    reader = SensorReader(port=args.port)
    reader.set_callback(test_callback)
    
    reader.start()
    
    # Зачекаємо трохи для ініціалізації
    time.sleep(1)
    
    if not reader.running or reader.serial_conn is None or not reader.serial_conn.is_open:
        print(f"Не вдалося підключитися до порту {args.port}. Переконайтесь, що Ардуіно підключено саме до цього порту.", flush=True)
        sys.exit(1)

    print("Підключено успішно! Будь ласка, замикайте/розмикайте геркон магнітом, щоб побачити події.", flush=True)
    print("Натисніть Ctrl+C для виходу.", flush=True)
    
    try:
        while True:
            time.sleep(0.1)
    except KeyboardInterrupt:
        print("\nЗавершення роботи...", flush=True)
    finally:
        reader.stop()
