#pragma once
#include <Arduino.h>
#include <functional>

#pragma pack(push, 1)
typedef struct {
  uint8_t address;
  uint8_t function;
  uint16_t registerStartAddress;
  uint16_t numOfRegistersToRead;
  uint16_t crc;
} tws10_command_t;

typedef struct {
  uint8_t address;
  uint8_t function;
  uint8_t dataSize;
  uint32_t distance;
  uint16_t crc;
} tws10_dist_response_t;
#pragma pack(pop)

class TWS10 {
  private:
  protected:
    std::function<void(tws10_dist_response_t*)> callback;
  public:
    HardwareSerial serial;
    TWS10(const HardwareSerial& serial, const std::function<void(tws10_dist_response_t*)>& callback) : serial(serial), callback(callback) {}
    virtual ~TWS10() {}
    void startMeasurement();

    inline HardwareSerial* getSerial() {
      return &serial;
    }

    inline void triggerCallback(tws10_dist_response_t* response) {
      callback(response);
    }
};
