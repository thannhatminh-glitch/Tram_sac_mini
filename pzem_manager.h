#ifndef PZEM_MANAGER_H
#define PZEM_MANAGER_H
 
#include <Arduino.h>
 
class PzemManager {
 
public:
    void begin(HardwareSerial &serial);
    void resetEnergy();
    bool update();
 
    float getVoltage();
    float getCurrent();
    float getPower();
    double getEnergyKwh();
    float getFrequency();
    float getPowerFactor();
 
private:
    HardwareSerial* _serial;
    uint8_t responseBuf[25];
 
    float    _voltage   = 0;
    float    _current   = 0;
    float    _power     = 0;
    double   _energyKwh = 0;
    float    _frequency = 0;
    float    _pf        = 0;
 
    uint16_t Modbus_CRC16(const uint8_t *buf, uint16_t len);
};
 
#endif