import time
import argparse
from src.sensor_reader import SensorReader
from src.tracker import MovementTracker
from src.output_gamepad import GamepadOutput

def main():
    parser = argparse.ArgumentParser(description="VR Treadmill Controller (Gamepad Mode)")
    parser.add_argument('--port', type=str, default='COM3', help='COM порт (наприклад, COM3 або /dev/ttyUSB0)')
    parser.add_argument('--mock', action='store_true', help='Запуск в режимі симулятора датчика (без апаратного забезпечення)')
    parser.add_argument('--timeout', type=float, default=1.0, help='Таймаут в секундах для зупинки')
    parser.add_argument('--max-rpm', type=int, default=120, help='Кількість замикань на хвилину для максимальної швидкості')
    
    args = parser.parse_args()

    import sys
    
    # Якщо програму запустили без аргументів (подвійним кліком), запитаємо порт
    if len(sys.argv) == 1:
        import serial.tools.list_ports
        ports = serial.tools.list_ports.comports()
        print("\n--- Доступні COM порти ---")
        if ports:
            for i, p in enumerate(ports, 1):
                print(f"  {i}) {p.device} ({p.description})")
        else:
            print("  Не знайдено жодного підключеного пристрою!")
        print("--------------------------\n")
        
        port_input = input(f"Виберіть номер порту (або введіть назву) [натисніть Enter для {args.port}]: ").strip()
        if port_input:
            if port_input.isdigit() and 1 <= int(port_input) <= len(ports):
                args.port = ports[int(port_input) - 1].device
            else:
                args.port = port_input

    # Ініціалізація компонентів
    print("Ініціалізація VR Treadmill Controller...")
    
    try:
        gamepad_output = GamepadOutput()
        tracker = MovementTracker(stop_timeout=args.timeout, max_rpm=args.max_rpm)
        reader = SensorReader(port=args.port, mock_mode=args.mock)

        # Зв'язуємо компоненти
        reader.set_callback(tracker.register_trigger)
        tracker.set_callback(gamepad_output.set_speed)

        # Запуск
        reader.start()

        if not reader.running:
            print(f"Не вдалося запустити зчитування з порту {args.port}.")
            input("Натисніть Enter для виходу...")
            return

        print("Система працює. Натисніть Ctrl+C для виходу.")
        while True:
            time.sleep(0.1)
    except KeyboardInterrupt:
        print("\nВихід з програми...")
    except Exception as e:
        print(f"\nСталася помилка: {e}")
        input("Натисніть Enter для виходу...")
    finally:
        if 'reader' in locals(): reader.stop()
        if 'tracker' in locals(): tracker.stop()
        if 'gamepad_output' in locals(): gamepad_output.stop_moving()

if __name__ == "__main__":
    main()
