// Pins
const int LED_PIN = 13;
const int PEDAL_PIN = 12;

// State
bool pressed = false;

// Main Code
void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(PEDAL_PIN, INPUT);
  Serial.begin(9600);
}

void loop() {
  if (!pressed and !digitalRead(PEDAL_PIN)) {
    digitalWrite(LED_PIN, HIGH);
    pressed = true;
    Serial.write("+");
  } else if (pressed and digitalRead(PEDAL_PIN)) {
    digitalWrite(LED_PIN, LOW);
    pressed = false;
    Serial.write("-");
  }
}
