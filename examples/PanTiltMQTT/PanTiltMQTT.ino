#include <FeedbackServo.h>
#include <TWS10.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "creds.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "EspMQTTClient.h"

EspMQTTClient mqttClient(
  ssid,
  password,
  mqttBrokerIp,
  "",
  "",
  "PanTiltDistance",
  mqttPort
);

const char* subscribeTopic = "distance";
const char* publishTopic = "distance";

bool deviceIsConnected = false;

const uint8_t rxPin = 20;
const uint8_t txPin = 21;

TWS10* distanceMeter = 0;
FeedbackServo* pan = 0;
FeedbackServo* tilt = 0;

const uint8_t servoPanPin = 3;
const uint8_t servoTiltPin = 4;

const uint8_t servoPanFeedbackPin = 0;
const uint8_t servoTiltFeedbackPin = 1;

StaticJsonDocument<64> doc;

StaticJsonDocument<48> currentStatusJson;
String currentStatus;
uint32_t lastDistance = 0;

void updateStatus() {
  currentStatusJson["pan"] = pan->getAnalogFeedback();
  currentStatusJson["tilt"] = tilt->getAnalogFeedback();
  currentStatusJson["dist"] = lastDistance;
  serializeJson(currentStatusJson, currentStatus);
  Serial.println(currentStatus);
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, 20, 21);

  pan = new FeedbackServo(servoPanPin, servoPanFeedbackPin, 0, 75, 180, 793, 1895, 3183);
  tilt = new FeedbackServo(servoTiltPin, servoTiltFeedbackPin, 70, 90, 110, 1838, 2106, 2408);

  distanceMeter = new TWS10(Serial1, [&](tws10_dist_response_t* resp) {
    Serial.printf("addr(0x%02x),fun(0x%02x),size(%u),dist(%umm),crc(0x%04x)\r\n", resp->address, resp->function, resp->dataSize, resp->distance, resp->crc);
    lastDistance = resp->distance;
    updateStatus();
  });

  mqttClient.enableLastWillMessage("PanTiltDistance/lastwill", "I am going offline");
}

uint32_t lastUpdateMs = 0;
void loop() {
  mqttClient.loop();
  if (millis() - lastUpdateMs > 5000) {
    if (deviceIsConnected) {
      if (currentStatus.length()) {
        mqttClient.publish(publishTopic, currentStatus
        );
      }
    }
    lastUpdateMs = millis();
  }
}

void onConnectionEstablished() {
  deviceIsConnected = true;
  mqttClient.subscribe(subscribeTopic, [](const String & payload) {
    if (payload.length() > 0) {
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }

      String cmd = doc["cmd"];
      if (cmd.indexOf("pan") > -1) {
        int val = doc["val"];
        pan->smartWrite(val);
      } else if (cmd.indexOf("tilt") > -1) {
        int val = doc["val"];
        tilt->smartWrite(val);
      } else if (cmd.indexOf("dist") > -1) {
        distanceMeter->startMeasurement();
      }

      updateStatus();
      Serial.printf("> %s\r\n", currentStatus.c_str());
    }
  });
}
