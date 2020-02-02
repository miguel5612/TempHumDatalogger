#ifndef LCD_H
#define LCD_H

#include "Arduino.h"
#include <LiquidCrystal_I2C.h>

class LCD {
  public:
    void inicializar();
    void mostrarMensajeLinea1(String mensaje);
    void mostrarMensajeLinea2(String mensaje);
};


#endif // PINS_H
