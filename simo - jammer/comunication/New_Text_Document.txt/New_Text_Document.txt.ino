/*
 * nRF24L01 Serial Bridge  --  PC <-> Arduino <-> radio <-> Arduino <-> PC
 *
 * Flash this SAME sketch onto BOTH Arduino Nanos.
 * On ONE of them, change the line below to:   #define NODE 1
 * Leave the other as:                          #define NODE 0
 *
 * Then open the Arduino Serial Monitor on each PC at 115200 baud,
 * set the line ending to "Newline", type a message, hit Enter,
 * and it appears on the other PC.
 *
 * Library: TMRh20/RF24  (Tools > Manage Libraries > search "RF24")
 *
 * Wiring (nRF24L01 -> Arduino Nano):
 *   VCC  -> 3.3V   (NOT 5V - 5V will damage the module)
 *   GND  -> GND
 *   CE   -> D9
 *   CSN  -> D10
 *   SCK  -> D13
 *   MOSI -> D11
 *   MISO -> D12
 *   IRQ  -> not connected
 *
 * Tip: solder a 10uF capacitor across the module's VCC and GND pins.
 * The nRF24L01 draws current spikes when transmitting and the Nano's
 * 3.3V regulator is weak; a missing cap is the #1 cause of "it won't talk".
 */

#include <SPI.h>
#include "RF24.h"

#define NODE 0            // <-- set to 1 on the second Arduino

RF24 radio(9, 10);        // CE, CSN

// Indicators (LEDs need a 220-330 ohm resistor in series, long leg toward the pin)
const byte TX_LED_PIN = 2;   // lights up while/after we transmit
const byte RX_LED_PIN = 3;   // lights up when a message arrives
const byte BUZZER_PIN = 4;   // beeps when a message arrives (+ to D4, - to GND)

const unsigned int LED_ON_MS = 150;     // how long an LED stays lit so you can actually see it
const unsigned int BEEP_HZ = 2000;
const unsigned int BEEP_MS = 120;

unsigned long txLedOffAt = 0;
unsigned long rxLedOffAt = 0;

// Two 5-byte addresses. Each node LISTENS on one and WRITES to the other.
const byte addr0[6] = "NODE0";
const byte addr1[6] = "NODE1";

const byte channel = 76;  // 0..125. Pick the same one on both. 76 is a quiet default.

char outBuf[30];          // message being typed in from the PC (max 29 chars + null)
byte outLen = 0;

byte txSeq = 0;           // sequence number of the last packet we sent
byte lastRxSeq = 0;       // sequence number of the last packet we accepted

void setup() {
  Serial.begin(115200);
  while (!Serial) { }     // wait for the USB serial port (harmless on a Nano)
  randomSeed(analogRead(A0) + NODE);  // different backoff pattern on each node

  pinMode(TX_LED_PIN, OUTPUT);
  pinMode(RX_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Short power-on self test: both LEDs + a beep, so you know the wiring works
  digitalWrite(TX_LED_PIN, HIGH);
  digitalWrite(RX_LED_PIN, HIGH);
  tone(BUZZER_PIN, BEEP_HZ, BEEP_MS);
  delay(300);
  digitalWrite(TX_LED_PIN, LOW);
  digitalWrite(RX_LED_PIN, LOW);

  if (!radio.begin()) {
    Serial.println(F("Radio hardware not responding - check wiring/power!"));
    while (1) { }         // stop here; nothing works without the radio
  }

  radio.setChannel(channel);
  radio.setPALevel(RF24_PA_LOW);      // LOW is plenty on a bench; raise later if needed
  radio.setDataRate(RF24_250KBPS);    // slowest rate = best range/reliability
  radio.setPayloadSize(32);           // fixed 32-byte packets
  radio.setAutoAck(true);             // automatic acknowledgements = reliable delivery
  radio.setRetries(5, 15);            // retry on failure

#if NODE == 0
  radio.openWritingPipe(addr1);       // node 0 writes to node 1
  radio.openReadingPipe(1, addr0);    // node 0 listens on its own address
#else
  radio.openWritingPipe(addr0);       // node 1 writes to node 0
  radio.openReadingPipe(1, addr1);
#endif

  radio.startListening();             // default state: receiving
  Serial.print(F("Bridge ready. I am NODE "));
  Serial.println(NODE);
  Serial.println(F("Type a message and press Enter."));
}

void loop() {
  // ---- 1) Receive: anything on the air -> print to this PC ----
  if (radio.available()) {
    byte in[32];
    radio.read(&in, sizeof(in));
    in[31] = '\0';                    // make sure it's null-terminated before printing
    // in[0] = sequence number, in[1..] = text. A repeated number means the
    // sender retried because our ACK was lost, so skip the duplicate.
    if (in[0] != lastRxSeq) {
      lastRxSeq = in[0];
      Serial.print(F("<< "));
      Serial.println((char *)&in[1]);

      digitalWrite(RX_LED_PIN, HIGH);
      rxLedOffAt = millis() + LED_ON_MS;
      tone(BUZZER_PIN, BEEP_HZ, BEEP_MS);   // non-blocking: stops by itself
    }
  }

  // ---- Turn the LEDs back off once their time is up ----
  unsigned long now = millis();
  if (digitalRead(RX_LED_PIN) && (long)(now - rxLedOffAt) >= 0) digitalWrite(RX_LED_PIN, LOW);
  if (digitalRead(TX_LED_PIN) && (long)(now - txLedOffAt) >= 0) digitalWrite(TX_LED_PIN, LOW);

  // ---- 2) Send: anything typed on this PC -> over the air ----
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r' || outLen == sizeof(outBuf) - 1) {
      if (outLen > 0) {
        outBuf[outLen] = '\0';
        sendMessage(outBuf);
        outLen = 0;
      }
    } else {
      outBuf[outLen++] = c;
    }
  }
}

void sendMessage(const char *msg) {
  byte packet[32];
  memset(packet, 0, sizeof(packet));
  packet[0] = ++txSeq;                // sequence number (lets the receiver drop duplicates)
  strncpy((char *)&packet[1], msg, sizeof(packet) - 2);

  bool ok = false;
  for (byte attempt = 0; attempt < 4 && !ok; attempt++) {
    digitalWrite(TX_LED_PIN, HIGH);
    radio.stopListening();            // switch to transmit mode
    ok = radio.write(&packet, sizeof(packet));  // 5 hardware retries happen in here
    radio.startListening();           // switch straight back to receive
    if (!ok) delay(random(5, 40));    // random backoff so two nodes don't collide in lockstep
  }
  txLedOffAt = millis() + LED_ON_MS;  // keep the TX LED lit a moment so a quick send is visible

  Serial.print(ok ? F(">> sent: ") : F(">> FAILED after retries (is the other node on?): "));
  Serial.println((char *)&packet[1]);
}
