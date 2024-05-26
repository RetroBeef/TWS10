#include <FeedbackServo.h>
#include <TWS10.h>
#include <NimBLEDevice.h>
#include <ArduinoJson.h>

#define SERVICE_UUID "8a39df89-82c9-47d6-b16f-86d97d1ca78b"
#define COMMAND_CHAR_UUID "86dce9e0-818b-4183-ba5c-6395e30addfe"
#define NOTIFICATION_CHAR_UUID "86dce9e0-818b-4183-ba5c-6395e30addff"

uint16_t mtu = 247;

const uint8_t rxPin = 20;
const uint8_t txPin = 21;

TWS10* distanceMeter = 0;
FeedbackServo* pan = 0;
FeedbackServo* tilt = 0;

const uint8_t servoPanPin = 3;
const uint8_t servoTiltPin = 4;

const uint8_t servoPanFeedbackPin = 0;
const uint8_t servoTiltFeedbackPin = 1;

NimBLECharacteristic *commandCharacteristic;
NimBLECharacteristic *notificationCharacteristic;
bool deviceConnected = false;

StaticJsonDocument<64> doc;

class MyServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Client connected");
    };

    void onDisconnect(NimBLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Client disconnected");
      NimBLEDevice::startAdvertising();
    }
};

StaticJsonDocument<48> currentStatusJson;
String currentStatus;
uint32_t lastDistance = 0;

void updateStatus() {
  currentStatusJson["pan"] = pan->getAnalogFeedback();
  currentStatusJson["tilt"] = tilt->getAnalogFeedback();
  currentStatusJson["dist"] = lastDistance;
  serializeJson(currentStatusJson, currentStatus);
}

class CommandCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic *pCharacteristic) {
      String input = pCharacteristic->getValue();
      Serial.printf("< %s\r\n", input.c_str());
      if (input.length() > 0) {
        DeserializationError error = deserializeJson(doc, input);
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

        //updateStatus();
        Serial.printf("> %s\r\n", currentStatus.c_str());
        notificationCharacteristic->setValue(currentStatus);
        notificationCharacteristic->notify();
      }
    }
};

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, 20, 21);

  pan = new FeedbackServo(servoPanPin, servoPanFeedbackPin, 0, 75, 180, 793, 1895, 3183);
  tilt = new FeedbackServo(servoTiltPin, servoTiltFeedbackPin, 70, 90, 110, 1838, 2106, 2408);

  distanceMeter = new TWS10(Serial1, [](tws10_dist_response_t* resp) {
    Serial.printf("addr(0x%02x),fun(0x%02x),size(%u),dist(%umm),crc(0x%04x)\r\n", resp->address, resp->function, resp->dataSize, resp->distance, resp->crc);
    lastDistance = resp->distance;
    updateStatus();
    if (deviceConnected) {
      Serial.printf("> %s\r\n", currentStatus.c_str());
      notificationCharacteristic->setValue(currentStatus);
      notificationCharacteristic->notify();
    }
  });

  NimBLEDevice::init("TWS10PanTilt");
  NimBLEDevice::setMTU(mtu);
  NimBLEServer *pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  NimBLEService *pService = pServer->createService(SERVICE_UUID);
  notificationCharacteristic = pService->createCharacteristic(NOTIFICATION_CHAR_UUID, NIMBLE_PROPERTY::NOTIFY);
  commandCharacteristic = pService->createCharacteristic(COMMAND_CHAR_UUID, NIMBLE_PROPERTY::WRITE);
  commandCharacteristic->setCallbacks(new CommandCallbacks());
  pService->start();

  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(pService->getUUID());
  pAdvertising->setScanResponse(true);
  pAdvertising->start();
  Serial.println("Waiting for a client connection to notify...");
}

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
}
