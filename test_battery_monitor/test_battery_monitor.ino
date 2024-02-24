#include <Adafruit_TinyUSB.h>

void setup() {
  Serial.begin(9600);
  pinMode(PIN_VBAT, INPUT);
  pinMode(VBAT_ENABLE, OUTPUT);
  pinMode(VBAT_ENABLE, LOW);
  pinMode(D0, OUTPUT);
  digitalWrite(D0, 0);
}

const float cal = 512.0/474.0;

void loop() {
  float v = cal*analogRead(PIN_VBAT)*3.3/1024.0;
  float vBatt = v*1510.0/510.0;
  Serial.print(v);
  v = analogRead(A1);
  Serial.print("  ");
  Serial.println(v);
  delay(1000);
}
