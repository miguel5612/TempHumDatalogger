#include "configuration.h"
#include "LCD.h"

//Crear el objeto lcd  dirección  0x3F y 16 columnas x 2 filas
LiquidCrystal_I2C lcd(0x3F,16,2);

void LCD::inicializar()
{
    lcd.init();                      
    lcd.init();
  
    //Encender la luz de fondo.
    lcd.backlight();
    lcd.setCursor(0,0);
    
    // Escribimos el Mensaje en el LCD.
    lcd.print("Monitorlab.team");
}

void LCD::mostrarMensajeLinea1(String mensaje)
{
    lcd.setCursor(0,0);
    
    // Escribimos el Mensaje en el LCD.
    lcd.print(mensaje);
}

void LCD::mostrarMensajeLinea2(String mensaje)
{
    lcd.setCursor(0,1);
    
    // Escribimos el Mensaje en el LCD.
    lcd.print(mensaje);
}
