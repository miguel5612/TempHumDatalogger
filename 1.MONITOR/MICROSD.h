#ifndef MICROSD_H
#define MICROSD_H

#include "Arduino.h"
#include <SPI.h>
#include <SD.h>

class MICROSD {
  public:
    void inicializar();
    void imprimirLinea(String mensaje);
};


#endif // PINS_H
