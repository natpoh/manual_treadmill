import time
import threading

class MovementTracker:
    def __init__(self, stop_timeout=0.2, max_rpm=120):
        """
        :param stop_timeout: Час у секундах без замикань, після якого вважається, що зупинився.
        :param max_rpm: Кількість "ТІКів" на хвилину, яка вважається максимальною швидкістю (1.0).
        """
        self.stop_timeout = stop_timeout
        self.max_rpm = max_rpm
        self.last_trigger_time = 0
        self.current_speed = 0.0
        self.on_speed_change_callback = None
        
        self.running = True
        self.monitor_thread = threading.Thread(target=self._monitor_timeout, daemon=True)
        self.monitor_thread.start()

    def set_callback(self, on_speed_change):
        self.on_speed_change_callback = on_speed_change

    def register_trigger(self, delta_ms):
        """Отримує час між кроками в мілісекундах від Arduino і розраховує швидкість"""
        self.last_trigger_time = time.time()
        
        if delta_ms > 0:
            # Обчислюємо "оберти за хвилину" (як швидко спрацьовує геркон)
            rpm = (1000.0 / delta_ms) * 60.0
            
            # Нормалізуємо швидкість від 0.0 до 1.0 (де 1.0 = max_rpm)
            target_speed = min(1.0, rpm / self.max_rpm)
            
            # Згладжування (щоб швидкість не стрибала занадто різко)
            alpha = 0.3
            self.current_speed = (alpha * target_speed) + ((1.0 - alpha) * self.current_speed)
            
            if self.on_speed_change_callback:
                self.on_speed_change_callback(self.current_speed)

    def _monitor_timeout(self):
        """Потік, який постійно перевіряє, чи не зупинилася людина"""
        while self.running:
            time.sleep(0.1)
            # Якщо пройшло більше часу, ніж stop_timeout, швидкість падає до 0
            if (time.time() - self.last_trigger_time) > self.stop_timeout and self.current_speed > 0:
                self.current_speed = 0.0
                if self.on_speed_change_callback:
                    self.on_speed_change_callback(0.0)

    def stop(self):
        self.running = False
        if self.monitor_thread:
            self.monitor_thread.join(timeout=1.0)
