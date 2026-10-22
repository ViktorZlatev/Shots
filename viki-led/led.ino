// Mapping Nano digital pins to your specific segments
const int pinA = 2; // White wire  -> Segment A (Top)
const int pinC = 3; // Orange wire -> Segment C (Bottom Right)
const int pinE = 4; // Black wire  -> Segment E (Bottom Left)
const int pinF = 5; // Purple wire -> Segment F (Top Left)
const int pinB = 7; // Yellow wire -> Segment B (Top Right)
const int pinD = 8; // Red wire    -> Segment D (Bottom)
const int pinG = 9; // Blue wire   -> Segment G (Middle)


const int segmentPins[7] = {pinA, pinB, pinC, pinD, pinE, pinF, pinG};

// Segment patterns for numbers 0 through 9 (1 = HIGH / ON, 0 = LOW / OFF)
// Order: {A, B, C, D, E, F, G}
const byte digits[10][7] = {
  {1, 1, 1, 1, 1, 1, 1}, // 0
  {0, 1, 1, 0, 0, 0, 0}, // 1
  {1, 1, 0, 1, 1, 0, 1}, // 2
  {1, 1, 1, 1, 0, 0, 1}, // 3
  {0, 1, 1, 0, 0, 1, 1}, // 4
  {1, 0, 1, 1, 0, 1, 1}, // 5
  {1, 0, 1, 1, 1, 1, 1}, // 6
  {1, 1, 1, 0, 0, 0, 0}, // 7
  {1, 1, 1, 1, 1, 1, 1}, // 8
  {1, 1, 1, 1, 0, 1, 1}  // 9
};

void setup() {
  for (int i = 0; i < 7; i++) {
    pinMode(segmentPins[i], OUTPUT);
  }
}

void loop() {
  for (int num = 0; num < 10; num++) {
    displayDigit(num);
    delay(1000); // Wait 1 second before switching to the next number
  }
}

void displayDigit(int digitIndex) {
  for (int seg = 0; seg < 7; seg++) {
    digitalWrite(segmentPins[seg], digits[digitIndex][seg]);
  }
}