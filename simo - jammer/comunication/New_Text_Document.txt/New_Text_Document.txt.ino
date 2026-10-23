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
 * ======================================================================
 *  HARDWARE SECTION  (build this on BOTH Arduinos - the wiring is identical)
 * ======================================================================
 *
 *  PARTS PER ARDUINO
 *    - 1x Arduino Nano
 *    - 1x nRF24L01 radio module (with its antenna)
 *    - 1x red LED    (TX  = "I am transmitting")
 *    - 1x green LED  (RX  = "I received a message")
 *    - 2x resistor 220-330 ohm (one per LED, REQUIRED)
 *    - 1x buzzer (active or passive)
 *    - 1x capacitor 10uF (strongly recommended, for the radio)
 *    - breadboard + jumper wires
 *
 *  PIN SUMMARY (Nano pin -> what it connects to)
 *    D2   -> TX LED   (through a resistor)
 *    D3   -> RX LED   (through a resistor)
 *    D4   -> Buzzer +
 *    D9   -> nRF24 CE
 *    D10  -> nRF24 CSN
 *    D11  -> nRF24 MOSI
 *    D12  -> nRF24 MISO
 *    D13  -> nRF24 SCK
 *    3.3V -> nRF24 VCC   (NOT 5V!)
 *    GND  -> nRF24 GND, both LED cathodes, Buzzer -
 *
 *  1) nRF24L01 RADIO MODULE
 *     !! Power it from the Nano's 3.3V pin. 5V WILL DESTROY the module. !!
 *     (The signal pins are 5V tolerant, so no level shifter is needed.)
 *
 *       nRF24 pin   ->  Nano pin
 *       ---------       --------
 *       GND         ->  GND
 *       VCC         ->  3.3V
 *       CE          ->  D9
 *       CSN         ->  D10
 *       SCK         ->  D13
 *       MOSI        ->  D11
 *       MISO        ->  D12
 *       IRQ         ->  not connected
 *
 *     Typical 8-pin header layout, looking at the module from above with
 *     the antenna pointing away from you (pin 1 = GND is marked by a
 *     square pad / arrow on most boards - always check YOUR board's labels):
 *
 *                     antenna
 *                 +---------------+
 *          GND  1 | [GND]  [VCC]  | 2  VCC
 *          CE   3 | [CE ]  [CSN]  | 4  CSN
 *          SCK  5 | [SCK]  [MOSI] | 6  MOSI
 *          MISO 7 | [MISO] [IRQ]  | 8  IRQ
 *                 +---------------+
 *
 *     Capacitor: put the 10uF capacitor directly across the module's
 *     VCC and GND pins (as close to the module as possible). The long leg
 *     (+) goes to VCC, the stripe/short leg (-) goes to GND.
 *     The radio draws sharp current spikes when it transmits and the
 *     Nano's 3.3V regulator is weak. A missing capacitor is the #1 cause
 *     of "NO ACK" / "radio not responding".
 *
 *     Antenna: keep the module/antenna away from metal and away from the
 *     USB cable. If you use a high-power PA+LNA module (the one with the
 *     big external antenna), it needs MORE current than the Nano's 3.3V
 *     pin can give - power it from an nRF24 adapter board with its own
 *     3.3V regulator, or you will get random failures.
 *
 *  2) TX LED (red) - lights when this Arduino transmits
 *     An LED has two legs: the LONG leg is the anode (+), the SHORT leg
 *     (flat side of the case) is the cathode (-). It only works one way round.
 *
 *       Nano D2 ---[ 220-330 ohm resistor ]---> LED long leg (+)
 *       LED short leg (-) -----------------------> GND
 *
 *     The resistor is REQUIRED. Without it the LED (and the Nano pin)
 *     can burn out. It can go on either side of the LED.
 *
 *  3) RX LED (green) - lights when this Arduino receives a message
 *     Same as the TX LED, on pin D3:
 *
 *       Nano D3 ---[ 220-330 ohm resistor ]---> LED long leg (+)
 *       LED short leg (-) -----------------------> GND
 *
 *  4) BUZZER - beeps when a message is received
 *     Look for a "+" mark on the buzzer (the longer leg is also +).
 *
 *       Nano D4 -----------------------> Buzzer +
 *       Buzzer - ----------------------> GND
 *
 *     Active buzzers (beep by themselves) and passive buzzers (need a
 *     tone signal) both work with this sketch. Use a small buzzer that
 *     draws under ~20 mA, because it is driven directly from the pin.
 *     A big or loud buzzer needs a transistor - do not connect it directly.
 *
 *  GROUND: everything shares the same ground. On a breadboard, run one
 *  wire from the Nano's GND to the breadboard's blue (-) rail and plug all
 *  the GND connections (radio, both LEDs, buzzer, capacitor) into that rail.
 *
 *  WIRING CHECK before powering on:
 *    [ ] Radio VCC is on 3.3V, NOT 5V
 *    [ ] Capacitor is across the radio's VCC and GND (right polarity)
 *    [ ] Both LEDs have a resistor and the long leg faces the Nano pin
 *    [ ] Buzzer + goes to D4, - goes to GND
 *    [ ] Every GND is connected together
 *
 *  POWER-ON TEST: when you plug it in, both LEDs should light and the
 *  buzzer should beep once for about 0.3 seconds. If that happens, the
 *  LEDs and buzzer are wired correctly. If the Serial Monitor then says
 *  "Radio hardware not responding", re-check the radio wiring and capacitor.
 * ======================================================================
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
  radio.setDataRate(RF24_1MBPS);      // 1 Mbps: far more reliable than 250 kbps on cheap/clone modules
  radio.setAddressWidth(5);           // explicit, so both sides always agree
  radio.setCRCLength(RF24_CRC_16);    // explicit 16-bit CRC on both sides
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
  radio.printPrettyDetails();         // dump the radio's settings: both nodes should match except the addresses
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

  if (!ok) {
    radio.flush_tx();                 // clear anything stuck in the transmit buffer
    Serial.print(F(">> NO ACK (it may STILL have arrived - the ACK can be lost). Radio chip "));
    Serial.println(radio.isChipConnected() ? F("responds.") : F("NOT responding: check wiring/power!"));
  }
  Serial.print(ok ? F(">> sent: ") : F(">> tried: "));
  Serial.println((char *)&packet[1]);
}
