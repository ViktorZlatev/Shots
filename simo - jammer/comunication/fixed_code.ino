#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// ======================================================================
// CONFIGURATION: Set NODE to 0 for Board #1, and set NODE to 1 for Board #2
// ======================================================================
#define NODE 0  // <--- Change to 1 on the SECOND Arduino Nano before uploading!

// Pin Definitions based on your pinout
#define PIN_TX_LED 2   // Red LED (Sending)
#define PIN_RX_LED 3   // Green LED (Receiving)
#define PIN_BUZZER 4   // Buzzer (+)
#define PIN_CE     9   // nRF24 CE
#define PIN_CSN    10  // nRF24 CSN

// RF24 Radio Object Initialization
RF24 radio(PIN_CE, PIN_CSN);

// Pipe addresses for two-way communication
const byte addresses[][6] = {"1Node", "2Node"};

// Buffer for incoming and outgoing messages
char messageBuffer[32] = "";

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial);

  // Initialize hardware pins
  pinMode(PIN_TX_LED, OUTPUT);
  pinMode(PIN_RX_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  digitalWrite(PIN_TX_LED, LOW);
  digitalWrite(PIN_RX_LED, LOW);
  digitalWrite(PIN_BUZZER, LOW);

  // Start SPI & Radio
  if (!radio.begin()) {
    Serial.println(F("[-] Hardware Alert: nRF24L01 radio not responding! Check SPI/Power wiring."));
    while (1); // Halt if radio fails to initialize
  }

  // Set RF parameters
  radio.setPALevel(RF24_PA_LOW);      // Low power for close range testing (helps stability)
  radio.setDataRate(RF24_1MBPS);      // Standard data rate
  radio.setChannel(76);               // Noise-free channel

  // Configure reading & writing pipes based on NODE ID
  if (NODE == 0) {
    radio.openWritingPipe(addresses[0]); // Writes to 1Node
    radio.openReadingPipe(1, addresses[1]); // Listens on 2Node
  } else {
    radio.openWritingPipe(addresses[1]); // Writes to 2Node
    radio.openReadingPipe(1, addresses[0]); // Listens on 1Node
  }

  radio.startListening();

  // Startup indication (LEDs & Buzzer test)
  digitalWrite(PIN_TX_LED, HIGH);
  delay(150);
  digitalWrite(PIN_TX_LED, LOW);
  
  digitalWrite(PIN_RX_LED, HIGH);
  delay(150);
  digitalWrite(PIN_RX_LED, LOW);

  tone(PIN_BUZZER, 1000, 100);
  delay(100);

  Serial.println(F("========================================"));
  Serial.print(F("[+] Node Started as Node ID: "));
  Serial.println(NODE);
  Serial.println(F("[+] Type a message in Serial Monitor and press Enter to send."));
  Serial.println(F("========================================"));
}

void loop() {
  // -------------------------------------------------------------------
  // 1. TRANSMITTING DATA (Serial Input -> RF Output)
  // -------------------------------------------------------------------
  if (Serial.available() > 0) {
    String inputString = Serial.readStringUntil('\n');
    inputString.trim(); // Remove whitespace / newlines

    if (inputString.length() > 0) {
      // Turn on TX (Red) LED
      digitalWrite(PIN_TX_LED, HIGH);

      // Convert String to Char Array (max 32 bytes for nRF24 payload)
      inputString.toCharArray(messageBuffer, sizeof(messageBuffer));

      radio.stopListening(); // Stop listening to allow transmission
      
      bool success = radio.write(&messageBuffer, strlen(messageBuffer) + 1);

      radio.startListening(); // Resume listening

      // Turn off TX LED
      digitalWrite(PIN_TX_LED, LOW);

      if (success) {
        Serial.print(F("[TX] Sent successfully: "));
        Serial.println(messageBuffer);
      } else {
        Serial.println(F("[TX Error] Transmission failed (No ACK from target)."));
      }
    }
  }

  // -------------------------------------------------------------------
  // 2. RECEIVING DATA (RF Input -> Serial Output)
  // -------------------------------------------------------------------
  if (radio.available()) {
    char receivedData[32] = "";
    radio.read(&receivedData, sizeof(receivedData));

    // Visual & Acoustic Feedback
    digitalWrite(PIN_RX_LED, HIGH);
    triggerBuzzer();

    // Print received message
    Serial.print(F("[RX] Received: "));
    Serial.println(receivedData);

    delay(200);
    digitalWrite(PIN_RX_LED, LOW);
  }
}

// Function for distinct double-beep alert on message arrival
void triggerBuzzer() {
  tone(PIN_BUZZER, 2000, 80);
  delay(100);
  tone(PIN_BUZZER, 2000, 80);
}