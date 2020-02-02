// Monitor de temperatura y humedad para el laboratorio
// de calibracion de medidores electricos de Centrales Electricas
// de norte de santander
// Autor: Andres Martinez
// Tipo de dispositivo: Datalogger Temeperatura y humedad CENS
// Pagina web: www.monitorlab.team - onmotica.com
// Fecha: 02.02.2020

//Librerias
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include <SPI.h>
#include <SD.h>
// WiFi
#include <WiFiManager.h>  
#include <ESP8266HTTPClient.h>
// Blink
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
// Manejo del tiempo
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h> 
// Ensamblaje del mensaje MQTT
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>

//Variables
float temperatura, humedad;
double val;
String excelLine, clientId = "ESP8266Client-";
unsigned long lastTime;
double lastCal;
const long utcOffsetInSeconds = -18000; //GMT -5:00 For UTC -5.00 : -5 * 60 * 60 : -18000
const int serverPort = 1883;
static char* mqtt_server =  "mqtt.onmotica.com";
char auth[] = "g4HK21Y2NyNNRdiO24nJUGDRmJZrUbHS"; // Token del modulo 1
//char auth[] = "g4HK21Y2NyNNRdiO24nJUGDRmJZrUbHS"; // Token del modulo 2
    
#define outTopic "/Cens/Cucuta/Lab_calibracion/Output"
#define inTopic "/Cens/Cucuta/Lab_calibration/Input"
#define wiFiname "Cens_Cucuta_Lab_Calibracion"

//Instanciacion de las librerias
LiquidCrystal_I2C lcd(0x3F,16,2);
Adafruit_SHT31 sht31 = Adafruit_SHT31();
WiFiManager wifiManager;
File myFile;
WiFiUDP ntpUDP;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
NTPClient timeClient(ntpUDP, "south-america.pool.ntp.org", utcOffsetInSeconds);

void setup() {
  //Inicializacion de librerias
  Serial.begin(9600);
  lcd.init(); 
  
  //Encender la luz de fondo.
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Conectate: ");
  lcd.setCursor(0,1);
  lcd.print(wiFiname);
  
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
      Serial.println("Error encontrando el sensor");
      lcd.setCursor(0,1);
      lcd.print("***Sensor error***");
  }       

  if (!SD.begin(D8)) {
    Serial.println("Error de memoria SD");
    lcd.setCursor(0,1);
    lcd.print("***SD error***");
  }

  WiFiManagerParameter custom_blynk_token("Blynk", "blynk token", auth, 33);
  wifiManager.addParameter(&custom_blynk_token);
  while (!wifiManager.autoConnect(wiFiname)) {
      Serial.println("Fallo al conectarse, reintentando...");      
      lcd.setCursor(0,1);
      lcd.print("***WiFi error***");
      delay(5000);
  }
  Blynk.config(custom_blynk_token.getValue());
  Blynk.begin(custom_blynk_token.getValue(), WiFi.SSID().c_str(), WiFi.psk().c_str());
  mqttClient.setServer(mqtt_server, serverPort);
  mqttClient.setCallback(callback);
  clientId += String(random(0xffff), HEX); mqttClient.connect(clientId.c_str());
  
  lcd.setCursor(0,0);
  lcd.print("Monitorlab.team");
  
  timeClient.begin();
  
  // Escribimos el mensaje de bienvenida en el LCD.
  delay(2000); 
  lcd.setCursor(0,0); lcd.print("Cal="); lcd.print(getOffset());
  
  Serial.println("Inicializacion exitosa");
  lcd.clear();
}

void loop() {
  //Ejecucion del programa
  calcularTemperaturaHumedad();
  
  mostrarTempHumEnPantalla();  
  
  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.print(" °C");

  Serial.print(" - ");
  
  Serial.print("Humedad: ");
  Serial.print(humedad);
  Serial.println(" %RH");

  excelLine = String(temperatura) + ";" +  String(humedad) + ";" + getOffset() + ";" + getTime();

  myFile = SD.open("registros.csv", FILE_WRITE);
  if (myFile) {
    Serial.print("registro excel: "); Serial.println(excelLine);
    myFile.println (excelLine);
    myFile.close();
  }
  
  // Enviamos el mensaje a la plataforma de monitorlab.team - onmotica.com 
  publicarMQTT();
  
  // Durante el tiempo de espera va actualizando la variable offset o calibracion
  smartDelay(30000);
}

double getOffset()
{
  val  = analogRead(A0); // PIN A0_TEMP ADJ
  return (0.009765*val)-5;
}

void smartDelay(int mS)
{
    lastTime = millis();
    while(millis() - lastTime< mS) {

      //Ciclo de blink
      Blynk.run();

      //Ciclo de MQTT
      mqttClient.loop();
      
      //Si el valor es distinto al anterior impreso en pantalla lo muestra
      if(abs((getOffset() - lastCal)*100) > 5)
      {
        lcd.setCursor(0,0); 
        lcd.print("                ");
        lcd.setCursor(0,0); 
        lcd.print("Cal="); 
        lcd.print(getOffset());

        lcd.print(" WiFi:");
        lcd.print(WiFi.isConnected()?"OK":" F");
        
        lastCal = getOffset();
        
        calcularTemperaturaHumedad();
        mostrarTempHumEnPantalla();  
  
      }

      // Para que blink este actualizado
      calcularTemperaturaHumedad(); actualizarBlink();
      
      delay(100);     
    }
}

void publicarMQTT()
{
  const size_t CAPACITY = JSON_OBJECT_SIZE(11);
  StaticJsonDocument<CAPACITY> jsonBuffer;
  JsonObject root = jsonBuffer.to<JsonObject>();    
  root["D1"] = temperatura;  
  root["D2"] = humedad;
  char JSONmessageBuffer[200];
  //root.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  serializeJson(root, JSONmessageBuffer);

  Serial.println(JSONmessageBuffer);
  
  if (mqttClient.publish(inTopic, JSONmessageBuffer) == true) {
        Serial.println("Publicado!!! :)");
      } else {
        Serial.println("No se ha podido publicar");
        lcd.setCursor(0,0);
        lcd.print("***Falla MQTT***");
      }
}

void calcularTemperaturaHumedad()
{
  // Esta funcion obtiene del sensor los valores de temperatura y humedad 
  temperatura = sht31.readTemperature();
  humedad = sht31.readHumidity();
  temperatura = temperatura + getOffset(); 
}

void mostrarTempHumEnPantalla()
{
  // Esta funcion muestra en pantalla la temperatura y humedad
  lcd.setCursor(0,1);
  lcd.print("T="); lcd.print(temperatura); 
  lcd.print(" ");
  lcd.print("H="); lcd.print(humedad); 

  actualizarBlink();
}

void actualizarBlink()
{
  Blynk.virtualWrite(V5, temperatura);
  Blynk.virtualWrite(V6, humedad);
 
  // Send time to the App
  Blynk.virtualWrite(V1, getOnlyTime());
  // Send date to the App
  Blynk.virtualWrite(V2, getOnlyDate());

  Blynk.virtualWrite(V7, "Monitor v1");
  Blynk.virtualWrite(V8, "CENS CÚCUTA");
}

String getOnlyTime()
{
  timeClient.update();
   time_t rawtime = timeClient.getEpochTime();
   struct tm * ti;
   ti = localtime (&rawtime);

   uint16_t year = ti->tm_year + 1900;
   String yearStr = String(year);

   uint8_t month = ti->tm_mon + 1;
   String monthStr = month < 10 ? "0" + String(month) : String(month);

   uint8_t day = ti->tm_mday;
   String dayStr = day < 10 ? "0" + String(day) : String(day);

   uint8_t hours = ti->tm_hour;
   String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

   uint8_t minutes = ti->tm_min;
   String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

   uint8_t seconds = ti->tm_sec;
   String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

   return hoursStr + ":" + minuteStr + ":" + secondStr;
}
String getOnlyDate()
{
  timeClient.update();
   time_t rawtime = timeClient.getEpochTime();
   struct tm * ti;
   ti = localtime (&rawtime);

   uint16_t year = ti->tm_year + 1900;
   String yearStr = String(year);

   uint8_t month = ti->tm_mon + 1;
   String monthStr = month < 10 ? "0" + String(month) : String(month);

   uint8_t day = ti->tm_mday;
   String dayStr = day < 10 ? "0" + String(day) : String(day);

   uint8_t hours = ti->tm_hour;
   String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

   uint8_t minutes = ti->tm_min;
   String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

   uint8_t seconds = ti->tm_sec;
   String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

   return dayStr + "/" + monthStr + "/" + yearStr;
}

String getTime()
{
   timeClient.update();
   time_t rawtime = timeClient.getEpochTime();
   struct tm * ti;
   ti = localtime (&rawtime);

   uint16_t year = ti->tm_year + 1900;
   String yearStr = String(year);

   uint8_t month = ti->tm_mon + 1;
   String monthStr = month < 10 ? "0" + String(month) : String(month);

   uint8_t day = ti->tm_mday;
   String dayStr = day < 10 ? "0" + String(day) : String(day);

   uint8_t hours = ti->tm_hour;
   String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

   uint8_t minutes = ti->tm_min;
   String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

   uint8_t seconds = ti->tm_sec;
   String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

   return dayStr + "/" + monthStr + "/" + yearStr + " - " +
          hoursStr + ":" + minuteStr + ":" + secondStr;

}

void callback(char* topic, byte* payload, unsigned int length) {
  // En llegada de mensajes no realiza nada.
}
