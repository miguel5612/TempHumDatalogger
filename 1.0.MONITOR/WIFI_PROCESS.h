#ifndef WIFI_PROCESS_H
#define WIFI_PROCESS_H

#include "Arduino.h"

class WIFI_PROCESS {
  public:
    void inicializar();    
    String getPetition(String URL2Get);
    boolean wifiIsConnected();
    void publicarBlynk(float h, float t);
    void cicloBlynk();
};


#endif // MQTT_PROCESS_H
