import time
import argparse
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from src.sensor_reader import SensorReader
from src.tracker import MovementTracker

# Дані для графіка
times = []
speeds = []
start_time = time.time()
tracker = None # глобальна змінна

def animate(i):
    # Додаємо дані безперервно, навіть коли ми стоїмо!
    current_time = time.time() - start_time
    times.append(current_time)
    
    # Зчитуємо актуальну швидкість напряму з трекера
    current_speed = tracker.current_speed * 100 if tracker else 0
    speeds.append(current_speed)
    
    # Зберігаємо останні 500 точок (5 секунд при 100 Гц)
    if len(times) > 500:
        times.pop(0)
        speeds.pop(0)

    plt.cla() # Очистити старий кадр
    
    if len(times) > 0:
        plt.plot(times, speeds, label='Швидкість бігу (%)', color='#00aeff', linewidth=3)
        plt.fill_between(times, speeds, color='#00aeff', alpha=0.3)
    
    plt.ylim(-5, 105)
    plt.title('Графік швидкості бігової доріжки (100 оновлень/сек)', fontsize=14)
    plt.xlabel('Час (секунди)')
    plt.ylabel('Швидкість (%)')
    plt.grid(True, linestyle='--', alpha=0.6)
    plt.legend(loc='upper right')

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Побудова графіка швидкості")
    parser.add_argument('--port', type=str, default='COM3', help='COM порт')
    args = parser.parse_args()

    print("Підготовка графіка... Зачекайте.")
    
    # Ініціалізуємо трекер
    tracker = MovementTracker(stop_timeout=0.2, max_rpm=120)
    
    # Читач порту передає дані прямо в трекер
    reader = SensorReader(port=args.port)
    reader.set_callback(tracker.register_trigger)
    
    reader.start()
    time.sleep(1)
    
    if not reader.running or reader.serial_conn is None or not reader.serial_conn.is_open:
        print(f"Помилка: не вдалося підключитися до {args.port}.")
        tracker.stop()
        exit(1)

    print("Готово! Графік запущено в режимі безперервного руху.")

    plt.style.use('dark_background')
    fig = plt.figure(figsize=(10, 5))
    
    # Оновлюємо графік КОЖНІ 10 мілісекунд (це і є швидкість 100 Гц)
    ani = animation.FuncAnimation(fig, animate, interval=10, cache_frame_data=False)
    
    plt.show()

    print("Зупинка зчитування...")
    reader.stop()
    tracker.stop()
