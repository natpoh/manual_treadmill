// Arduino 1000Hz Digital Reed Switch Stream
// Reads the digital state at 1000 Hz and sends 1 (magnet near) or 0 (magnet far).

const int SENSOR_PIN = A0;

void setup() {
  Serial.begin(115200); 
  pinMode(SENSOR_PIN, INPUT);
}

void loop() {
  // Read sensor: since SENSOR_PIN is pulled up, LOW means magnet is near, HIGH means magnet is far.
  // We send 1 for magnet near, 0 for magnet far.
  int val = (digitalRead(SENSOR_PIN) == LOW) ? 1 : 0;
  Serial.println(val);
  delay(1); // 1000 Hz sampling rate (1ms delay)
}