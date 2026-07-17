// Arduino 200Hz Raw Reed Switch Stream
// Безперервно надсилає сирі аналогові показання з частотою 200 Гц.

const int SENSOR_PIN = A0;

void setup() {
  Serial.begin(115200); 
  pinMode(SENSOR_PIN, INPUT);
}

void loop() {
  int aVal = analogRead(SENSOR_PIN);
  Serial.println(aVal);
  delay(5); // 200 Hz sampling rate
}