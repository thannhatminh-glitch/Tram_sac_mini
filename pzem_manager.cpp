#include "pzem_manager.h"

// ===== PZEM-004T Modbus register map (dia chi 0x0000 - 0x0009) =====
// Reg 0x0000 : Voltage      (0.1 V/LSB)
// Reg 0x0001 : Current Lo   (0.001 A/LSB) - high word o Reg 0x0002
// Reg 0x0002 : Current Hi
// Reg 0x0003 : Power Lo     (0.1 W/LSB)   - high word o Reg 0x0004
// Reg 0x0004 : Power Hi
// Reg 0x0005 : Energy Lo    (1 Wh/LSB)    - high word o Reg 0x0006
// Reg 0x0006 : Energy Hi
// Reg 0x0007 : Frequency    (0.1 Hz/LSB)
// Reg 0x0008 : Power Factor (0.01/LSB)
// Reg 0x0009 : Alarm status

// Response frame layout (25 bytes):
// [0]      : Address  (0x01)
// [1]      : Function (0x04)
// [2]      : Byte count (0x14 = 20)
// [3][4]   : Reg0 - Voltage
// [5][6]   : Reg1 - Current Lo
// [7][8]   : Reg2 - Current Hi
// [9][10]  : Reg3 - Power Lo
// [11][12] : Reg4 - Power Hi
// [13][14] : Reg5 - Energy Lo
// [15][16] : Reg6 - Energy Hi
// [17][18] : Reg7 - Frequency
// [19][20] : Reg8 - Power Factor
// [21][22] : Reg9 - Alarm
// [23][24] : CRC

void PzemManager::begin(HardwareSerial &serial) {
    _serial = &serial;
}

uint16_t PzemManager::Modbus_CRC16(const uint8_t *buf, uint16_t len) {

    uint16_t crc = 0xFFFF;

    for (uint16_t pos = 0; pos < len; pos++) {

        crc ^= buf[pos];

        for (int i = 0; i < 8; i++) {

            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

void PzemManager::resetEnergy() {

    uint8_t frame[4] = {0x01, 0x42};

    uint16_t crc = Modbus_CRC16(frame, 2);

    frame[2] = crc & 0xFF;
    frame[3] = (crc >> 8) & 0xFF;

    _serial->write(frame, 4);
}

bool PzemManager::update() {
    while (_serial->available()) _serial->read();
    // Read 10 registers (0x0000 - 0x0009), slave address 0x01
    uint8_t readFrame[] = {0x01, 0x04, 0x00, 0x00, 0x00, 0x0A, 0x70, 0x0D};

    _serial->write(readFrame, 8);

    // [FIX #4] Thay delay(200) bằng vòng chờ non-blocking tối đa 300ms
    unsigned long t = millis();
    while (_serial->available() < 25 && millis() - t < 300) {
        delayMicroseconds(500);
    }

    int index = 0;

    while (_serial->available() && index < 25) {
        responseBuf[index++] = _serial->read();
    }

    if (index < 25 || responseBuf[1] != 0x04) {
        return false;
    }

    // ===== Voltage : Reg0, 0.1 V/LSB =====
    uint16_t reg0 = (responseBuf[3] << 8) | responseBuf[4];
    _voltage = reg0 / 10.0f;

    // ===== Current : Reg1 (Lo) + Reg2 (Hi), 0.001 A/LSB =====
    uint16_t reg1 = (responseBuf[5]  << 8) | responseBuf[6];
    uint16_t reg2 = (responseBuf[7]  << 8) | responseBuf[8];
    uint32_t currentRaw = ((uint32_t)reg2 << 16) | reg1;
    _current = currentRaw / 1000.0f;

    // ===== Power : Reg3 (Lo) + Reg4 (Hi), 0.1 W/LSB =====
    uint16_t reg3 = (responseBuf[9]  << 8) | responseBuf[10];
    uint16_t reg4 = (responseBuf[11] << 8) | responseBuf[12];
    uint32_t powerRaw = ((uint32_t)reg4 << 16) | reg3;
    _power = powerRaw / 10.0f;

    // ===== Energy : Reg5 (Lo) + Reg6 (Hi), 1 Wh/LSB =====
    uint16_t reg5 = (responseBuf[13] << 8) | responseBuf[14];
    uint16_t reg6 = (responseBuf[15] << 8) | responseBuf[16];
    uint32_t energyWh = ((uint32_t)reg6 << 16) | reg5;
    _energyKwh = energyWh / 1000.0;

    // ===== Frequency : Reg7, 0.1 Hz/LSB =====
    uint16_t reg7 = (responseBuf[17] << 8) | responseBuf[18];
    _frequency = reg7 / 10.0f;

    // ===== Power Factor : Reg8, 0.01/LSB =====
    uint16_t reg8 = (responseBuf[19] << 8) | responseBuf[20];
    _pf = reg8 / 100.0f;

    return true;
}

// ===== Getters =====

float PzemManager::getVoltage()    { return _voltage;   }
float PzemManager::getCurrent()    { return _current;   }
float PzemManager::getPower()      { return _power;     }
double PzemManager::getEnergyKwh() { return _energyKwh; }
float PzemManager::getFrequency()  { return _frequency; }
float PzemManager::getPowerFactor(){ return _pf;        }