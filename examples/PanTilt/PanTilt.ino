#include <FeedbackServo.h>
#include <TWS10.h>

const uint8_t rxPin = 20;
const uint8_t txPin = 21;

TWS10* distanceMeter = 0;
FeedbackServo* pan = 0;
FeedbackServo* tilt = 0;

const uint8_t servoPanPin = 3;
const uint8_t servoTiltPin = 4;

const uint8_t servoPanFeedbackPin = 0;
const uint8_t servoTiltFeedbackPin = 1;

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, 20, 21);

  pan = new FeedbackServo(servoPanPin, servoPanFeedbackPin, 0, 75, 180, 793, 1895, 3183);
  tilt = new FeedbackServo(servoTiltPin, servoTiltFeedbackPin, 70, 90, 110, 1838, 2106, 2408);

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
      pan->smartWrite(val);
    } else {
      pos = input.indexOf("tilt");
      if (pos > -1) {
        String valStr = input.substring(pos + 4);
        uint8_t val = valStr.toInt();
        Serial.printf("setting tilt to %u. expected analog val(%u)\r\n", val, map(val, 0, 180, 790, 3320));
        tilt->smartWrite(val);
      } else {
        pos = input.indexOf("meas");
        if (pos > -1) {
          distanceMeter->startMeasurement();
        }
      }
    }
  }
  if (millis() - lastRequestMs > 5000) {
    Serial.printf("pan(%u), tilt(%u)\r\n", pan->getAnalogFeedback(), tilt->getAnalogFeedback());
    lastRequestMs = millis();
  }
}
