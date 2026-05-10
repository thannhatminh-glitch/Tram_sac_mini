#ifndef SEPAY_H
#define SEPAY_H

#include <Arduino.h>

class SepayManager {

public:
    void   begin();
    void   update();
    void   resetWiFi();

    float  getPaidKwh()      const;
    void   clearPaidKwh();
    String getLastContent()  const;

private:
    void   connectWiFi();
    void   loadLastID();
    void   saveLastID(const String& id);   

    String   lastID;
    String   _lastContent;
    float    _paidKwh     = 0.0f;
    uint32_t _lastCheckMs = 0;             
};

#endif