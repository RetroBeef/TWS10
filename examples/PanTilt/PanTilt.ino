#include <ESP32Servo.h>
#include "TWS10.h"

const uint8_t rxPin = 20;
const uint8_t txPin = 21;

TWS10* distanceMeter = 0;

typedef struct {
  uint8_t minValue;
  uint8_t centerValue;
  uint8_t maxValue;
} servo_values_t;

typedef struct {
  uint16_t minValue;
  uint16_t centerValue;
  uint16_t maxValue;
} servo_feedback_values_t;

typedef struct {
  Servo* servo;
  uint8_t servoPin;
  uint8_t feedbackPin;
  servo_values_t servoValues;
  servo_feedback_values_t servoFeedbackValues;
} smart_servo_t;

const uint8_t servoPanPin = 3;
const uint8_t servoTiltPin = 4;

const uint8_t servoPanFeedbackPin = 0;
const uint8_t servoTiltFeedbackPin = 1;

Servo panServo;
Servo tiltServo;

smart_servo_t pan = {&panServo, servoPanPin, servoPanFeedbackPin, {0, 75, 180}, {793, 1895, 3183}};
smart_servo_t tilt = {&tiltServo, servoTiltPin, servoTiltFeedbackPin, {70, 90, 110}, {1838, 2106, 2408}};

bool inRange(uint16_t valIs, uint16_t valShould, uint16_t range) {
  return ((valIs >= valShould - range) && (valIs <= valShould + range));
}

void smartWrite(const smart_servo_t& smartServo, const uint8_t val) {
  uint8_t clippedVal = val;
  if (val < smartServo.servoValues.minValue) {
    clippedVal = smartServo.servoValues.minValue;
  }
  if (val > smartServo.servoValues.maxValue) {
    clippedVal = smartServo.servoValues.maxValue;
  }
  if (!smartServo.servo->attached()) {
    smartServo.servo->attach(smartServo.servoPin);
  }
  smartServo.servo->write(clippedVal);
  uint16_t expectedFeedback = map(clippedVal, 0, 180, 790, 3320);
  uint16_t analogFeedback = analogRead(smartServo.feedbackPin);
  uint32_t startedMs = millis();
  bool timeout = false;
  while (!inRange(analogFeedback, expectedFeedback, 100) && !timeout) {
    analogFeedback = analogRead(smartServo.feedbackPin);
    timeout = millis() - startedMs > 1000;
  }
  if (timeout) {
    Serial.println("servo timed out; position not completely reliable");
  }
  if (smartServo.servo->attached())smartServo.servo->detach();
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, 20, 21);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  panServo.setPeriodHertz(50);
  panServo.attach(servoPanPin, 500, 2500);
  tiltServo.setPeriodHertz(50);
  tiltServo.attach(servoTiltPin, 500, 2500);

  distanceMeter = new TWS10(Serial1, [](tws10_dist_response_t* resp) {
    Serial.printf("addr(0x%02x),fun(0x%02x),size(%u),dist(%umm),crc(0x%04x)\r\n", resp->address, resp->function, resp->dataSize, resp->distance, resp->crc);
  });

  //Serial1.write(cmdContinuous, sizeof(cmdContinuous));
}

uint32_t lastRequestMs = 0;
void loop() {
  if (Serial.available()) {
    String input = Serial.readString();
    int pos = input.indexOf("pan");
    if (pos > - 1) {
      String valStr = input.substring(pos + 3);
      uint8_t val = valStr.toInt();
      Serial.printf("setting pan to %u. expected analog val(%u)\r\n", val, map(val, 0, 180, 790, 3320));
      smartWrite(pan, val);
    } else {
      pos = input.indexOf("tilt");
      if (pos > -1) {
        String valStr = input.substring(pos + 4);
        uint8_t val = valStr.toInt();
        Serial.printf("setting tilt to %u. expected analog val(%u)\r\n", val, map(val, 0, 180, 790, 3320));
        smartWrite(tilt, val);
      } else {
        pos = input.indexOf("meas");
        if (pos > -1) {
          distanceMeter->startMeasurement();
        }
      }
    }
  }
  if (millis() - lastRequestMs > 5000) {
    Serial.printf("pan(%u), tilt(%u)\r\n", analogRead(servoPanFeedbackPin), analogRead(servoTiltFeedbackPin));
    lastRequestMs = millis();
  }
}
