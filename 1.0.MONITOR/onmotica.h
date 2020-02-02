#ifndef onmotica_H
#define onmotica_H

#include "Arduino.h"

class onmotica {
  public:
    void init();
    String getTime();
    String getOnlyDate();
    String getOnlyTime();
  private:
    String __fecha;
};


#endif // onmotica_H
