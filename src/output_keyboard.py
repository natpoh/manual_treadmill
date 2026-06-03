from pynput.keyboard import Key, Controller

class KeyboardOutput:
    def __init__(self, forward_key='w'):
        """
        :param forward_key: Клавіша, яка буде натискатись для руху вперед.
        """
        self.keyboard = Controller()
        self.forward_key = forward_key
        self.is_pressing = False

    def start_moving(self):
        if not self.is_pressing:
            print(f"[Keyboard] Натискаю '{self.forward_key}'")
            self.keyboard.press(self.forward_key)
            self.is_pressing = True

    def stop_moving(self):
        if self.is_pressing:
            print(f"[Keyboard] Відпускаю '{self.forward_key}'")
            self.keyboard.release(self.forward_key)
            self.is_pressing = False
