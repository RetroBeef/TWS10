#include "TWS10.h"

namespace {
uint8_t cmdSingle[] = {0x01, 0x03, 0x00, 0x0F, 0x00, 0x02, 0xF4, 0x08};
uint8_t cmdContinuous[] = {0x01, 0x03, 0x00, 0x01, 0x00, 0x02, 0x95, 0xCB};
void pollTask(void *parameter) {
  TWS10* tws = (TWS10*) parameter;
  uint8_t buffer[sizeof(tws10_dist_response_t)] = {0};
  uint8_t bufferIndex = 0;
  Serial1.write(cmdSingle, sizeof(cmdSingle));
  uint32_t lastRequestMs = millis();
  while (1) {
loop_start:
    if (millis() - lastRequestMs > 5000) {
      Serial1.write(cmdSingle, sizeof(cmdSingle));
      lastRequestMs = millis();
      while (!Serial1.available()) {
        if (millis() - lastRequestMs > 5000) {
          Serial.println("rx timeout");
          goto loop_start;
        }
      }
    }
    while (tws->serial.available()) {
      uint8_t c = tws->serial.read();
      switch (bufferIndex) {
        case 0: {
            if (c == 0x01) {
              buffer[bufferIndex] = c;
              bufferIndex++;
            } else {
              bufferIndex = 0;
            }
          } break;
        case 1: {
            if (c == 0x03) {
              buffer[bufferIndex] = c;
              bufferIndex++;
            } else {
              bufferIndex = 0;
            }
          } break;
        case 2: {
            if (c == 0x04) {
              buffer[bufferIndex] = c;
              bufferIndex++;
            } else {
              bufferIndex = 0;
            }
          } break;
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8: {
            buffer[bufferIndex] = c;
            bufferIndex++;
          } break;
        default: {
            bufferIndex = 0;
          }
      }
    }
    if (bufferIndex == sizeof(tws10_dist_response_t)) {
      tws10_dist_response_t* resp = (tws10_dist_response_t*)buffer;
      resp->distance = __builtin_bswap32(resp->distance);
      resp->crc = __builtin_bswap16(resp->crc);
      bufferIndex = 0;
      tws->triggerCallback(resp);
      break;
    }
  }
  vTaskDelete(NULL);
}
}

void TWS10::startMeasurement() {
  xTaskCreate(pollTask, "TWSPollTask", 4096, this, 1, NULL);
}
