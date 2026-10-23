// Connect + to Pin 8
// Connect -to Pin GND

const int buzzerPin = 8;

// How long each note lasts, in "beat" units (1 = short, 2 = normal, 4 = long)
int jingle[]  = { 196, 0, 196, 262, 0, 330, 392, 0, 523 };
int lengths[] = { 150, 50, 150, 300, 80, 150, 150, 80, 600 };

const int jingleCount = sizeof(jingle) / sizeof(jingle[0]);

void setup() {
  pinMode(buzzerPin, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  for (int i = 0; i < jingleCount; i++) {
    if (jingle[i] != 0) {
      tone(buzzerPin, jingle[i]);
      digitalWrite(LED_BUILTIN, HIGH);
    }
    delay(lengths[i]);
    noTone(buzzerPin);
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void loop() {}