/* 
Bluetooth Page Turning Pedal
Francis Deck, 7/30/2023
MIT License

Hardware: Seeed XIAO nRF52840 microcontroller board with pushbutton
switches connected from pins 2, 3, and 4 to ground. Small LiPo "bag
battery" connected to battery terminals on the back of the board.

More documentation to come. This circuit simulates a 3 button Bluetooth
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

#define PG_UP_PIN 2
#define PG_DN_PIN 3
#define F11_PIN 4

#define PG_UP 1 // Typically turn back by one page
#define PG_DN 2 // Typically go forward by one page
#define F11 4 // Gets my PDF reader app in and out of full page mode
#define ALL_BUTTONS PG_UP + PG_DN + F11 // Used for controlling power-up and -down
#define BATTERY 6 // used to get battery condition
#define ENOUGH 1*60*60*1000 // Timeout converted to milliseconds

char NAME[] = "PTP003\0"; // This needs to be a unique name if you have more than one

int checkButtons(){
  /*
  Checks all 3 buttons, returns a binary indicating one or more
  buttons pressed at a time. The function provides de-bounce and allows
  for the fact that the buttons won't all be pressed exactly at once.

  The function is non blocking, meaning it will immediately return zero
  if no buttons are being pressed.
  */

  int result = 0;
  int buttons = 0;
  int oldResult;
  bool firstTime = true;

  while(1) {
    oldResult = result;
    // form binary representation of button state
    buttons = !digitalRead(PG_UP_PIN) | (!digitalRead(PG_DN_PIN) << 1) | (!digitalRead(F11_PIN) << 2);
    if (buttons == 0) {
      // Don't bother with de-bounce if no buttons were pressed
      if (!firstTime) delay(10); // de-bounce
      firstTime = false;
      digitalWrite(LED_GREEN, LOW);
      break;
    }
    digitalWrite(LED_GREEN, HIGH);
    if (buttons != oldResult) {
      // Any change in the button status generates a new de-bounce delaly
      delay(10); // de-bounce
      result |= buttons;
      oldResult = buttons;
    }
  }
  return result;
}

void buttonISR() {
  /*
  One of the buttons must be arbitrarily attached to an ISR so that
  pressing all 3 buttons wakes the device up out of sleep mode. But
  the ISR doesn't actually need to do anything because buttons are
  being polled in normal "awake" operation
  */
}

void gotoSleep() {
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
  */
  int i;
  float v = analogRead(PIN_VBAT)*3.3/1024.0;
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
  digitalWrite(LED_GREEN, LOW);
}

void setup() {
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);

  pinMode(PG_UP_PIN, INPUT_PULLUP_SENSE);
  pinMode(PG_DN_PIN, INPUT_PULLUP_SENSE);
  pinMode(F11_PIN, INPUT_PULLUP_SENSE);

  attachInterrupt(digitalPinToInterrupt(F11_PIN), buttonISR, FALLING);

  // Enable battery voltage monitor

  pinMode(14, OUTPUT);
  digitalWrite(14, LOW);
  
  // User needs to be pressing all buttons in order to wake up

  if (checkButtons() != ALL_BUTTONS) {
    checkBattery();
    gotoSleep();
  }

  Keyboard.begin(NAME);

  while(!Keyboard.isConnected()) { 
    // Blinking red LED while it tries to connect
    if (checkButtons() == ALL_BUTTONS) gotoSleep();
    digitalWrite(LED_RED, LOW);
    delay(100);
    digitalWrite(LED_RED, HIGH);
    delay(100);
  };

  digitalWrite(LED_GREEN, LOW);
}

void loop() {
  // Timeout
  static int time0 = millis();
  if (millis() - time0 > ENOUGH) gotoSleep();
  
  // Button based "user interface", should be self-explanatory
  int buttons = checkButtons();
  if (buttons != 0) time0 = millis();
  switch (buttons) {
    case PG_UP:
      Keyboard.press(KEY_PAGE_UP); //hold down the shift
      Keyboard.release(KEY_PAGE_UP);//release the shift
      break;
    case PG_DN:
      Keyboard.press(KEY_PAGE_DOWN); //hold down the shift
      Keyboard.release(KEY_PAGE_DOWN);//release the shift
      break;
    case F11:
      Keyboard.press(KEY_F11);
      Keyboard.release(KEY_F11);
      break;
    case ALL_BUTTONS:
      gotoSleep();
      break;
    case BATTERY:
      checkBattery();
      break;
  }
}