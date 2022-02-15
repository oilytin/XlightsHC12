#include <FastLED.h>

#include <AceButton.h>

using namespace ace_button;
#define CHANNELS_IN 8
#define CHANNELS_OUT 8
#define BUTTON_PIN 3
#define startMarker 0x82
#define endMarker 0x83
AceButton button(BUTTON_PIN);

bool dataReceived = false;
bool dataPrepared = false;
byte receivedData[CHANNELS_IN];
byte outputData[CHANNELS_OUT];
byte totalArrows = 1;
//uint8_t arrowSelect = 0;

uint32_t lastButton = 0;
uint32_t lastMillis = 0;
bool buttonLatch = 0;
uint8_t const oddEven[] = {10, 11};
bool oddEvenSelect = 0;
CRGB rgb (0, 0, 0);
CHSV hsv (0, 0, 0);
byte hue = 0;


/* Seven segment
  Segments:
        G F - A B
          a a a
        f       b
        f       b
        f       b
          g g g
        e       c
        e       c
        e       c
          d d d   P
        E D - C P
*/

//                   a  b  c   d   e  f  g
const byte pins[] = {9, 10, 4, 5, 6, 8, 7};
const byte digits[] = {
  //gfedcba
  0b1110111,  // A
  0b0000110,  // 1
  0b1011011,  // 2
  0b1001111,  // 3
  0b1100110,  // 4
  0b1101101,  // 5
  0b1111101,  // 6
  0b0100111,  // 7
  0b1111111,  // 8
  0b1101111   // 9
};
void setup() {
  Serial1.begin(2400);
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  button.setEventHandler(handleEvent);

  //7seg pins
  for (byte j = 0; j < sizeof(pins); j++) {
    pinMode(pins[j], OUTPUT);
  }
}
void loop() {
  button.check();
  if (millis() - lastButton > 3000)buttonLatch = 0;
  lastMillis = millis();
  recveData();
  processData();
  HC12snd();

  //Serial1.println(String("<") + arrowSelect + String(",") + values[1] + String(",")
  //              + values[2] + String(",") + values[3] + String(",") + values[4] + String(">"));


  if (buttonLatch == 0)displayNumber(outputData[0]);
  else displayNumber(totalArrows);
}
void recveData() {
  byte checksum = 0;
  static uint8_t checksumPrev = 0;
  if (Serial.available() >= CHANNELS_IN && dataReceived == false) {
    // read the oldest byte in the serial buffer:
    for (int i = 0; i <= CHANNELS_IN; i++) {
      // read each byte
      receivedData[i] = Serial.read();
    }

    checksum = 0;
    for (uint8_t i = 0; i < sizeof(receivedData); i++) {
      checksum ^= receivedData[i];
    }
    if (checksum != checksumPrev && checksum != 0) dataReceived = true;
    checksumPrev = checksum;
  }
}
void processData() {
  if (dataReceived) {
    switch (receivedData[0]) {
      case 0:               //individual
        if (outputData[0] >= totalArrows)outputData[0] = 1;
        else outputData[0]++;
        break;
      case 1:               //all
        outputData[0] = 0;
        break;
      case 2:               //odd even toggle
        oddEvenSelect ^= 1;
        outputData[0] = oddEven[oddEvenSelect];
        break;
      case 3:
        outputData[0] = 10; //odd
        break;
      case 4:
        outputData[0] = 11; //even
        break;
      default:
        outputData[0] = 0;
        break;
    }

    if (receivedData[1] == 10) {
      outputData[1] = 0;
      rainbowColour(receivedData[2], receivedData[3]);
      outputData[2] = rgb.r;
      outputData[3] = rgb.g;
      outputData[4] = rgb.b;
    }
    else if (receivedData[1] == 11) {
      outputData[1] = 0;
      randomColour(receivedData[2]);
      outputData[2] = rgb.r;
      outputData[3] = rgb.g;
      outputData[4] = rgb.b;
    }
    else {
      for (byte c = 1; c <= 4; c++) {
        outputData[c] = receivedData[c];
      }
    }
    outputData[5] = random8();
    uint8_t checksum = 0;
    for (byte o = 0; o < CHANNELS_OUT - 2; o++) {
      if (outputData[o] == startMarker)outputData[o]--;
      if (outputData[o] == endMarker)outputData[o]++;
      checksum ^= outputData[o];
    }
    outputData[6] = checksum / 2;
    outputData[7] = outputData[6] + (checksum % 2);
  }
}
void HC12snd() {
  if (dataReceived) {
    Serial1.write(startMarker);
    Serial1.write(outputData, CHANNELS_OUT);
    Serial1.write(endMarker);
    dataReceived = false;
  }
}
void displayNumber(byte number) {
  number = number % 10;   // hold the number within array bounds
  byte d = digits[number];
  for (byte i = 0; i < sizeof(pins); i++) {
    digitalWrite(pins[i], bitRead(d, i));  // turn segments on or off
  }
}

void handleEvent(AceButton * button, uint8_t eventType, uint8_t buttonState) {
  switch (eventType) {
    case AceButton::kEventPressed:
      digitalWrite(LED_BUILTIN, HIGH);
      if (millis() - lastButton < 1500) {
        totalArrows++;
        if (totalArrows > 8)totalArrows = 1;
      }
      buttonLatch = 1;
      lastButton = millis();
      break;
    case AceButton::kEventReleased:
      digitalWrite(LED_BUILTIN, LOW);
      break;
  }
}
void rainbowColour(byte hueStep, byte v) {//v determines how bright
  hue = hue + hueStep;
  hsv = CHSV(hue, 255, v);
  hsv2rgb_rainbow( hsv, rgb);  //convert HSV to RGB
}
void randomColour(byte v) {
  hue = random8();
  hsv = CHSV(hue, 255, v);
  hsv2rgb_rainbow( hsv, rgb);  //convert HSV to RGB
}
