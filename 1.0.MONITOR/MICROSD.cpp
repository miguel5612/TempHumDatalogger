#include "configuration.h"
#include "MICROSD.h"

File myFile;

void MICROSD::inicializar()
{
  Serial.print("Initializing SD card...");

  if (!SD.begin(D8)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  
  myFile = SD.open("léeme.txt", FILE_WRITE);
  if (myFile) {
   
    myFile.println ("Por favor lea atentamente el manual de usuario \r\n Este es un test de prueba :v");
    myFile.close();
  }
}


void MICROSD::imprimirLinea(String mensaje)
{
  myFile = SD.open("registros.csv", FILE_WRITE);
  if (myFile) {
    Serial.print("registro: "); Serial.println(mensaje);
    myFile.println (mensaje);
    myFile.close();
  }
}
