#ifndef onmotica_H
#define onmotica_H

#include "Arduino.h"

class onmotica {
  public:
    void init();
    String getTime();
  private:
    String __fecha;
};


#endif // onmotica_H
