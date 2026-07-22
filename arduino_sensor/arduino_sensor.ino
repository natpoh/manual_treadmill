// Arduino Analog Hall Sensor Stream (for SS49E)
// Reads the analog voltage from pin A0 and sends raw value (0-1023) to serial.

const int SENSOR_PIN = A0;

void setup() {
  Serial.begin(115200); 
  // Analog pins do not require pull-up resistors
  pinMode(SENSOR_PIN, INPUT);
}

void loop() {
  int val = analogRead(SENSOR_PIN);
  Serial.println(val);
  delay(1); // 1000 Hz sampling rate (1ms delay)
}