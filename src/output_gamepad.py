import vgamepad as vg
import time
import math

class GamepadOutput:
    def __init__(self):
        print("[Gamepad] Ініціалізація віртуального Xbox 360 контролера...")
        self.gamepad = vg.VX360Gamepad()
        # Даємо системі час розпізнати новий пристрій
        time.sleep(1)
        
        # Натискаємо кнопку "A" на мить, щоб браузери (як от Gamepad Tester) одразу побачили геймпад
        self.gamepad.press_button(button=vg.XUSB_BUTTON.XUSB_GAMEPAD_A)
        self.gamepad.update()
        time.sleep(0.1)
        self.gamepad.release_button(button=vg.XUSB_BUTTON.XUSB_GAMEPAD_A)
        self.gamepad.update()

        self.current_speed = 0.0

    def set_speed(self, speed_normalized):
        """
        Встановлює швидкість руху (відхилення стіка).
        :param speed_normalized: Значення від 0.0 (стоїть) до 1.0 (максимальна швидкість)
        """
        # Обмежуємо значення між 0.0 та 1.0
        speed_normalized = max(0.0, min(1.0, speed_normalized))
        self.current_speed = speed_normalized
        
        # Вісь Y лівого стіка в XInput має значення від -32768 до 32767.
        # Вперед — це позитивні значення.
        y_value = int(speed_normalized * 32767)
        
        self.gamepad.left_joystick(x_value=0, y_value=y_value)
        self.gamepad.update()
        
        # Виводимо для дебагу, якщо швидкість змінилася
        print(f"[Gamepad] Швидкість: {speed_normalized:.2f} (Y: {y_value})")

    def stop_moving(self):
        self.set_speed(0.0)
        print("[Gamepad] Зупинка.")
