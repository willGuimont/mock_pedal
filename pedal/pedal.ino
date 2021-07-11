// Pins
const int LED_PIN = 13;
const int PEDAL_PIN = 12;

// Constants
const char* ON = "on\n";
const char* OFF = "off\n";

// State
bool pressed = false;

// Functions
bool cancelOn();
bool cancelOff();
bool hasConfirmation();
void writeWithConfirmation(const char* x, bool (*cancel)());

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
    writeWithConfirmation(ON, cancelOn);
  } else if (pressed and digitalRead(PEDAL_PIN)) {
    digitalWrite(LED_PIN, LOW);
    pressed = false;
    writeWithConfirmation(OFF, cancelOff);
  }
}

// Implementation
bool cancelOn() {
  return digitalRead(PEDAL_PIN);
}

bool cancelOff() {
  return !digitalRead(PEDAL_PIN);
}

bool hasConfirmation() {
  if (Serial.available() > 0) {
    String confirmation = Serial.readStringUntil('\n');
    return confirmation == "ok\0";
  }
  return false;
}

void writeWithConfirmation(const char* x, bool (*cancel)()) {
  while (!hasConfirmation() and !cancel()) {
    Serial.write(x);
    delay(250);
  }
}
