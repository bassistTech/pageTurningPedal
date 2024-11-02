/* 
Bluetooth Page Turning Pedal
Francis Deck, 10/17/2024
MIT License

This version comes after some feedback from family members who have used
the pedal. For now I've settled on 2 main buttons, plus a power button
hidden under the pedal. Also, the pedal should emit a signal when the button
is pressed, not when it's released.

More documentation to come. This circuit simulates a 2 button Bluetooth
keyboard flipping pages in a PDF reader, typically for reading sheet
music on a tablet device.

Compiling seems to be hard. Use "Seeed nRF52 boards", non-mbed version.
I've had to compile more than once to make it stop emitting error
messages such as not recognizing "blueduino". Also, board can get itself
into a state where it has to be forced back into bootloader mode by
double-clicking the Reset button.

Warning about ISO C++ forbidding a string constant seems to be OK.
*/

#include <BLE52_Mouse_and_Keyboard.h>

#define PREV_PIN 6
#define NEXT_PIN 5
#define POWER_PIN 7
#define NEXT_KEY KEY_DOWN_ARROW
#define PREV_KEY KEY_UP_ARROW

#define timeout_ms 1*60*60*1000 // Timeout converted to milliseconds

char NAME[] = "PTP002\0"; // This needs to be a unique name

void wakeUp() {
  /*
  One of the buttons must be arbitrarily attached to an ISR so that
  pressing all 3 buttons wakes the device up out of sleep mode. But
  the ISR doesn't actually need to do anything because buttons are
  being polled in normal "awake" operation
  */
}

void gotoSleep() {
  /*
  This puts the device into sleep mode, after flashing the red LED
  for 1 second. It does not return from this function
  */
  pinMode(14, INPUT); // disable battery monitoring
  Keyboard.end();
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);
  delay(1000);
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_BLUE, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  NRF_POWER->SYSTEMOFF = 1;
}

void checkBattery(){
  /*
  Battery check measures battery voltage, then gives off a number
  of blinks for every 0.1 V above 3.4 V. The maximum possible is about
  4.2 V, so a "full" battery is at best 5 blinks.

  New thing: I've measured the performance of the ADC, and its calibration
  is inaccurate. So, I've wired a 50% voltage divider from 3V3 to analog
  input A0, and from A0 to digital D1. When D1 is grounded, A0 should be
  1/2 of the supply voltage. This is used to create a calibration that is
  then applied to the battery voltage.
  */

  pinMode(D0, OUTPUT);
  digitalWrite(D0, 0);
  float cal = 512.0/analogRead(A1);
  pinMode(D0, INPUT);
  int i;
  float v = cal*analogRead(PIN_VBAT)*3.3/1024.0;
  float vBatt = v*1510.0/510.0;
  int nblinks = (vBatt - 3.4)*10.0; // tenths of a volt above 3.4
  if (nblinks > 10) nblinks = 10;
  digitalWrite(LED_GREEN, HIGH);
  delay(250);
  for (i=0; i<nblinks; i++) {
    digitalWrite(LED_BLUE, LOW);
    delay(250);
    digitalWrite(LED_BLUE, HIGH);
    delay(250);
  }
}

void setup() {
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);

  pinMode(PREV_PIN, INPUT_PULLUP_SENSE);
  pinMode(NEXT_PIN, INPUT_PULLUP_SENSE);
  pinMode(POWER_PIN, INPUT_PULLUP_SENSE);

  attachInterrupt(digitalPinToInterrupt(POWER_PIN), wakeUp, FALLING);

  // Enable battery voltage monitor

  pinMode(PIN_VBAT, INPUT);
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);
  
  // TODO need to improve wake-up button behavior, for now 

  if (checkButtons() != POWER) {
    gotoSleep();
  }

  checkBattery();

  Keyboard.begin(NAME);

  while(!Keyboard.isConnected()) { 
    // Blinking blue LED while it tries to connect
    if (checkButtons() == POWER) gotoSleep();
    digitalWrite(LED_BLUE, LOW);
    delay(100);
    digitalWrite(LED_BLUE, HIGH);
    delay(100);
  };

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);
}

void loop() {
  // Timeout
  static int time0 = millis();
  if (millis() - time0 > ENOUGH) gotoSleep();
  
  // Button based "user interface", should be self-explanatory
  int buttons = checkButtons();
  if (buttons != 0) time0 = millis();
  switch (buttons) {
    case PREV:
      Keyboard.press(PREV_KEY); //hold down the shift was KEY_PAGE_UP
      Keyboard.release(PREV_KEY);//release the shift
      break;
    case NEXT:
      Keyboard.press(NEXT_KEY); //hold down the shift
      Keyboard.release(NEXT_KEY);//release the shift
      break;
    //case PG_UP + PG_DN:
    //  Keyboard.press(KEY_F11);
    //  Keyboard.release(KEY_F11);
    //  break;
    case POWER:
      gotoSleep();
      break;
    case BATTERY:
      checkBattery();
      break;
  }
}