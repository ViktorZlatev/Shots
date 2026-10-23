/*
 * STEP 1 - HARDWARE + RADIO LINK TEST   (flash this BEFORE radio_chat)
 *
 * Flash this SAME sketch onto BOTH Arduino Nanos.
 * On ONE of them set:  #define NODE 1   (leave the other at NODE 0)
 *
 * What it does:
 *   1. Blinks the TX LED, then the RX LED, then beeps  -> proves LEDs + buzzer work
 *   2. Checks the nRF24 chip answers over SPI           -> proves radio wiring/power
 *   3. Every second each board sends one packet to the other and prints:
 *        sent / acked / received
 *      If both boards show acked ~= sent and received is growing, the link works
 *      in BOTH directions and you can move on to radio_chat.
 *
 * Open the Serial Monitor at 115200 baud.
 * Library: "RF24" by TMRh20 (Tools > Manage Libraries), version 1.4.x or newer.
 *
 * ======================================================================
 *  HARDWARE SECTION (identical on BOTH Arduinos)
 * ======================================================================
 *
 *  PARTS PER ARDUINO
 *    1x Arduino Nano, 1x nRF24L01 module, 1x red LED (TX), 1x green LED (RX),
 *    2x resistor 220-330 ohm, 1x small buzzer, 1x capacitor 10uF (or bigger),
 *    breadboard + jumper wires.
 *
 *  PIN SUMMARY
 *    D2   -> TX LED (through a resistor)
 *    D3   -> RX LED (through a resistor)
 *    D4   -> Buzzer +
 *    D9   -> nRF24 CE
 *    D10  -> nRF24 CSN
 *    D11  -> nRF24 MOSI
 *    D12  -> nRF24 MISO
 *    D13  -> nRF24 SCK
 *    3.3V -> nRF24 VCC      (NEVER 5V - it destroys the module)
 *    GND  -> nRF24 GND, LED short legs, Buzzer -, capacitor -
 *
 *  nRF24L01:   GND->GND   VCC->3.3V   CE->D9   CSN->D10
 *              SCK->D13   MOSI->D11   MISO->D12   IRQ->(not connected)
 *
 *              antenna side (top)
 *            +------------------+
 *      GND 1 | [GND]  [VCC]     | 2 VCC
 *       CE 3 | [CE ]  [CSN]     | 4 CSN
 *      SCK 5 | [SCK]  [MOSI]    | 6 MOSI
 *     MISO 7 | [MISO] [IRQ]     | 8 IRQ
 *            +------------------+
 *      (check the labels printed on YOUR module - layouts vary)
 *
 *  Capacitor: 10uF (or 100uF) directly across the radio's VCC and GND, as
 *  close to the module as possible. Long leg (+) to VCC, stripe (-) to GND.
 *  Without it the radio browns out when it transmits - the #1 cause of
 *  "it receives but can't send" and lost acknowledgements.
 *
 *  LEDs (long leg = +, short leg = -). The resistor is REQUIRED.
 *    D2 --[220-330 ohm]--> red LED long leg,   short leg --> GND
 *    D3 --[220-330 ohm]--> green LED long leg, short leg --> GND
 *
 *  Buzzer (small one, under ~20 mA, active or passive):
 *    D4 --> Buzzer +        Buzzer - --> GND
 *
 *  All grounds must be connected together (use the breadboard GND rail).
 *  Keep the two boards 1-2 metres apart while testing (too close can
 *  overload the receivers).
 * ======================================================================
 */

#include <SPI.h>
#include <RF24.h>

#define NODE 0                         // <-- 0 on one board, 1 on the other

const uint8_t PIN_CE      = 9;
const uint8_t PIN_CSN     = 10;
const uint8_t PIN_TX_LED  = 2;
const uint8_t PIN_RX_LED  = 3;
const uint8_t PIN_BUZZER  = 4;

RF24 radio(PIN_CE, PIN_CSN);

// Each node listens on its own address and writes to the other's.
const uint8_t ADDR[2][6] = { "1Node", "2Node" };
const uint8_t CHANNEL = 76;

uint32_t sent = 0, acked = 0, received = 0;
unsigned long nextSendAt = 0;
unsigned long txLedOffAt = 0, rxLedOffAt = 0;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_TX_LED, OUTPUT);
  pinMode(PIN_RX_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  Serial.print(F("HW test, I am NODE "));
  Serial.println(NODE);

  // --- LED and buzzer test ---
  Serial.println(F("TX LED..."));
  digitalWrite(PIN_TX_LED, HIGH); delay(400); digitalWrite(PIN_TX_LED, LOW);
  Serial.println(F("RX LED..."));
  digitalWrite(PIN_RX_LED, HIGH); delay(400); digitalWrite(PIN_RX_LED, LOW);
  Serial.println(F("Buzzer..."));
  tone(PIN_BUZZER, 2000, 200); delay(400);

  // --- Radio test ---
  if (!radio.begin()) {
    Serial.println(F("RADIO NOT RESPONDING: check wiring (CE=D9 CSN=D10 SCK=D13 MOSI=D11 MISO=D12), 3.3V power, capacitor."));
    while (1) { }
  }
  if (!radio.isChipConnected()) {
    Serial.println(F("SPI answers but the nRF24 chip is not recognised: check wiring / power / module."));
    while (1) { }
  }

  radio.setChannel(CHANNEL);
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_1MBPS);
  radio.setAddressWidth(5);
  radio.setCRCLength(RF24_CRC_16);
  radio.setAutoAck(true);
  radio.setRetries(5, 15);
  radio.setPayloadSize(32);
  radio.openWritingPipe(ADDR[1 - NODE]);       // talk to the other node
  radio.openReadingPipe(1, ADDR[NODE]);        // listen on my own address
  radio.startListening();

  radio.printPrettyDetails();
  randomSeed(analogRead(A0) + NODE);
  Serial.println(F("Radio OK. Starting link test (one packet per second)..."));
}

void loop() {
  unsigned long now = millis();

  // --- receive ---
  while (radio.available()) {
    uint8_t buf[32];
    radio.read(buf, sizeof(buf));
    received++;
    digitalWrite(PIN_RX_LED, HIGH);
    rxLedOffAt = now + 100;
  }

  // --- send one packet per second (with a little random jitter) ---
  if ((long)(now - nextSendAt) >= 0) {
    uint8_t buf[32] = { 0 };
    buf[0] = (uint8_t)sent;
    digitalWrite(PIN_TX_LED, HIGH);
    radio.stopListening();
    bool ok = radio.write(buf, sizeof(buf));
    radio.startListening();
    txLedOffAt = millis() + 100;
    sent++;
    if (ok) acked++;

    Serial.print(F("sent "));     Serial.print(sent);
    Serial.print(F("  acked "));   Serial.print(acked);
    Serial.print(F("  received ")); Serial.print(received);
    Serial.println(ok ? F("   [ack ok]") : F("   [NO ACK]"));

    nextSendAt = millis() + 1000 + random(0, 200);
  }

  if ((long)(now - txLedOffAt) >= 0) digitalWrite(PIN_TX_LED, LOW);
  if ((long)(now - rxLedOffAt) >= 0) digitalWrite(PIN_RX_LED, LOW);
}
