/*
* Arduino Uno with nRF24L01 
*                
* by jbalagiya
* 
* Library: TMRh20/RF24, https://github.com/tmrh20/RF24/
*/
#include "RF24.h"

RF24 radio(9, 10); // CE, CSN pins for NRF24L01

byte i = 45;  // Initial channel for nRF24L01
unsigned int flag = 0;  // Flag to control channel hopping direction

void setup() {
  Serial.begin(115200); // Initialize serial communication
 if (radio.begin()) {
    delay(200);
    Serial.println("Radio Initialized!");
    radio.setAutoAck(false);       // Disable automatic acknowledgment
    radio.stopListening();         // Set to transmitter mode
    radio.setRetries(0, 0);        // Disable retries
    radio.setPayloadSize(5);       // Set payload size to 5 bytes
    radio.setAddressWidth(3);      // Set address width to 3 bytes
    radio.setPALevel(RF24_PA_MAX, true); // Set power amplification to maximum
    radio.setDataRate(RF24_2MBPS); // Set data rate to 2 Mbps
    radio.setCRCLength(RF24_CRC_DISABLED); // Disable CRC
    radio.printPrettyDetails();    // Print radio details for debugging
    radio.startConstCarrier(RF24_PA_MAX, i);  // Start continuous carrier with specified channel
  } else {
    Serial.println("Failed to initialize radio!");
  }
}

void loop() {
  // Sweep through all channels (0 to 79)
   for (int i = 0; i < 79; i++) {
    radio.setChannel(i);
   }
    // Specifically target BLE advertising channels (37, 38, 39)
  byte ble_channels[] = {37, 38, 39};
  for (int j = 0; j < 3; j++) {
    radio.setChannel(ble_channels[j]);
  }
}
