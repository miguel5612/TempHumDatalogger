#include "WIFI_PROCESS.h"
#include "configuration.h"
#include <WiFiManager.h>  
#include <ESP8266HTTPClient.h>

WiFiManager wifiManager;

#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

char auth[] = "g4HK21Y2NyNNRdiO24nJUGDRmJZrUbHS";
    
void WIFI_PROCESS::inicializar(){
   WiFiManagerParameter custom_blynk_token("Blynk", "blynk token", auth, 33);
   wifiManager.addParameter(&custom_blynk_token);
   while (!wifiManager.autoConnect(wiFiname)) {
      if(serDebug) Serial.println("Connection to hostname failed, restarting in 5 seconds");
      Blynk.config(custom_blynk_token.getValue());
      delay(minDelay);
  }
}

void WIFI_PROCESS::publicarBlynk(float h, float t)
{
  //Esta funcion actualiza en los moviles los datos visualizados de temperatura y humedad
  Blynk.virtualWrite(V5, h);
  Blynk.virtualWrite(V6, t);
}
void WIFI_PROCESS::cicloBlynk()
{
   Blynk.run();
}

String WIFI_PROCESS::getPetition(String URL2Get)
{
    char payload[300];
    HTTPClient http;  //Declare an object of class HTTPClient
    http.begin(URL2Get);  //Specify request destination
    int httpCode = http.GET();         
    if(serDebug) Serial.println("Codigo de respuesta HTTP: " + String(httpCode));
    //Send the request
    if (httpCode > 0) { //Check the returning code 
      http.getString().toCharArray(payload,200);   //Get the request response payload
      if(serDebug) Serial.println("Resultado de la peticion: ");
      if(serDebug) Serial.println(String(httpCode));
      http.end();   //Close connection
  }
  String tmpPayload = String(payload);
  tmpPayload.replace("\r", "");
  tmpPayload.replace("\n", "");
  tmpPayload.replace("\r\n", "");
  return tmpPayload;
}



boolean WIFI_PROCESS::wifiIsConnected()
{
  return WiFi.isConnected();
}
