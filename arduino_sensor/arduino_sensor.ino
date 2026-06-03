// Пін датчика (Геркон)
const int SENSOR_PIN = 2;

unsigned long lastTickTime = 0;

void setup() {
  Serial.begin(9600);
  // Підтягуючий резистор. Якщо магніт піднесено - буде LOW
  pinMode(SENSOR_PIN, INPUT_PULLUP);
}

void loop() {
  int currentState = digitalRead(SENSOR_PIN);

  // Спрацювання датчика (магніт близько)
  if (currentState == LOW) {
    unsigned long currentTime = millis();
    unsigned long timeDelta = currentTime - lastTickTime;

    // Захист від брязкоту (мінімум 50 мс між "тіками")
    if (timeDelta > 50) {
      
      // Якщо це перший крок після довгої зупинки
      if (lastTickTime == 0 || timeDelta > 3000) {
        // Щоб комп'ютер не ігнорував перший крок, скажемо йому, що це "повільний крок" (напр. 2 секунди)
        Serial.println("TICK:2000");
      } else {
        // Звичайний крок, відправляємо реальний час
        Serial.print("TICK:");
        Serial.println(timeDelta);
      }
      
      lastTickTime = currentTime;
    }

    // Чекаємо, поки магніт не відійде від датчика!
    // Без цього циклу програма буде "спамити" тисячі повідомлень.
    while(digitalRead(SENSOR_PIN) == LOW) {
      delay(1);
    }
  }
}
