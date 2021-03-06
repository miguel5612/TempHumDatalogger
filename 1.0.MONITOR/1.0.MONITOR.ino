#include "PROCESS_DATA.h"
#include "WIFI_PROCESS.h"
#include "MEMORY_ADMINISTRATION.h"
#include "PINS.h"
#include "onmotica.h"
#include "configuration.h"
#include "LCD.h"
#include "MICROSD.h"


#include <ESP8266WiFi.h>
#include <PubSubClient.h>
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"


PROCESS_DATA procesamiento;
WIFI_PROCESS WiFiProcess;
MEMORY_ADMINISTRATION memory;
PINS pinesIO;
onmotica utils;
Adafruit_SHT31 sht31 = Adafruit_SHT31();
MICROSD memoriaSd;
LCD pantallaLCD;


static const int count_mqtt_server = 3;
static char* mqtt_server[count_mqtt_server] = { "mqtt.onmotica.com", "mqtt2.onmotica.com", "mqtt3.onmotica.com"};
char* __mqttServerConnected;
const int serverPort = 1883;
int serverConnectedIndex = 0;
int pin = A0; //Pin A0 TEMP_AD
unsigned long lastPublishedTime = 0;
unsigned long lastGetPetition = 0;
unsigned long lastBlynkPetition = 0;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void setup() {

  pinMode(pin, INPUT); // pin A0 TEMP_ADJ
    
    if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
      Serial.println("Couldn't find SHT31");
      while (1) delay(1);
    }
    Serial.begin(115200);

    pantallaLCD.inicializar();
    memoriaSd.inicializar();
    pantallaLCD.mostrarMensajeLinea1("WiFi: X          ");
    WiFiProcess.inicializar();

    String resultPetition = WiFiProcess.getPetition(URL);
    if(serDebug) Serial.println("Peticion get - " + resultPetition);
    
    setMQTTServer();
    utils.init();

    procesamiento.setTimeToWait(procesamiento.generateRandom());

 }
void loop() {
  
    if(!WiFiProcess.wifiIsConnected()) setup(); //Reinicio si no hay wifi
    if (!mqttIsConnected()) reconnect(); //Reconectar mqtt si perdio conexion

    
    float temperatura = sht31.readTemperature();
    float humedad = sht31.readHumidity();
    //Calcular y guardar la fecha
    String fecha = utils.getTime();
    procesamiento.setFecha(fecha);

    
    if (isnan(humedad) || isnan(temperatura)) {
        Serial.println("Failed to read from SHT31 sensor!");
        temperatura = 0;
        humedad = 0;
    }
    else
    {
       double val  = analogRead(pin); // PIN A0_TEMP ADJ
       double offset = (0.009765*val)-5;
       temperatura = temperatura + offset;
       
      WiFiProcess.publicarBlynk(humedad, temperatura); //Actualiza en los celulares
      String temperatura2excel = String(temperatura);
      temperatura2excel.replace(".",",");
      String humedad2excel = String(humedad);
      humedad2excel.replace(".",",");
      String strOff = String(offset);
      strOff.replace(".",",");
      
      String mensaje = temperatura2excel + " ;" + humedad2excel + " ;" + fecha + ";" + strOff + ";";
      memoriaSd.imprimirLinea(mensaje);
      pantallaLCD.mostrarMensajeLinea2(String(temperatura) + " - " + String(humedad));
      
    }
    //Mensaje MQTT

    String json2MQTT = procesamiento.ensamblarMensajeJSON(temperatura, humedad, 0, 0, 0, 0, 0, 0, 0, 0, fecha);

    if(procesamiento.SAVEJSON(json2MQTT))
    {
      procesamiento.setTimeToWait(procesamiento.generateRandom());
      sendMQTTMsgPacket(procesamiento.getIndex());
    }
    else
    {
      pantallaLCD.mostrarMensajeLinea1("Monitorlab.team ");
    }
    
    //if((millis() - lastPublishedTime)>maxTimeWithNoPublish)  memoriaSD.saveIntoLogMsg("Han pasado " + String(maxTimeWithNoPublish/60000) + " minutos sin enviar actualizaciones" , administracion.freeSpaceReportSerial() , WiFiProcess.wifiIsConnected()?"Conectado":"Desconectado", mqttIsConnected()?"Conectado":"Desconectado", true);   
    if((millis() -lastGetPetition)>maxTimeWithNoPublish) WiFiProcess.getPetition(URL); //Despertar al servidor haciendo una peticion cada media hora
    while((millis() -lastBlynkPetition)< 3000)
    {      
      mqttClient.loop();
      WiFiProcess.cicloBlynk();
    }
    lastBlynkPetition = millis();
    
}
void sendMQTTMsgPacket(int countMsgToSend)
{
  int len = 260;
  char buf[len];
  procesamiento.resetMsgQeueCounter();

  for(int i=0; i<=countMsgToSend - 1; i++)
  {
    Serial.println("Preparando Json #" + String(i+1));
    pantallaLCD.mostrarMensajeLinea1("Publicando: " + String(i+1));
    String mensajeJson = procesamiento.getJSON(i);
    mensajeJson.toCharArray(buf, len);
    savePublishStatusMQTT(publicarInformacion(buf));
  }
}
void savePublishStatusMQTT(boolean result)
{
  if(result)
      { 
         unsigned long span = (millis() - lastPublishedTime)/1000;
         int ratio = span>60?span/60:span;
         String unidad = span>60?" minutos":" segundos";
         lastPublishedTime = millis();
         //memoriaSD.saveIntoLogMsg("Mensaje publicado(" + String(__mqttServerConnected) + ") con exito - ratio (Tiempo transcurrido desde la anterior publicacion): " + String(ratio) + unidad , administracion.freeSpaceReportSerial() , WiFiProcess.wifiIsConnected()?"Conectado":"Desconectado", mqttIsConnected()?"Conectado":"Desconectado", false);   
      }
      else
      {
        //memoriaSD.saveIntoLogMsg("Mensaje no publicado", administracion.freeSpaceReportSerial() , WiFiProcess.wifiIsConnected()?"Conectado":"Desconectado", mqttIsConnected()?"Conectado":"Desconectado", true);   
      }
}


boolean publicarInformacion(char JSON[260]){
   
    bool isPublished = false;
    int attemps = 0;

    while(!isPublished & attemps<=10)
    {
      if (mqttClient.publish(inTopic, JSON) == true) {
        
        if(serDebug) Serial.println("El mensaje se ha publicado correctamente");
        
        isPublished = true;
        if(serDebug) Serial.println("Publicado!!! :)");
        break;
      } else {
        attemps++;
        if(serDebug) Serial.println("No se ha podido publicar - intento " + String(attemps));
        isPublished = false;
        delay(minTime);
      }
    }
    delay(minTime);
    return isPublished;
}

void setMQTTServer()
{   
  if(serverConnectedIndex < count_mqtt_server-1)
  {
    serverConnectedIndex++;
  }
  else
  {
    serverConnectedIndex = 0;
  }
  __mqttServerConnected = mqtt_server[serverConnectedIndex];//"broker.mqtt-dashboard.com";
  if(serDebug) Serial.println("MQTT Server: " + String(__mqttServerConnected));
  mqttClient.setServer(__mqttServerConnected, serverPort);
  mqttClient.setCallback(callback);
}
bool mqttIsConnected()
{
  return mqttClient.connected();
}

void reconnect() {
  int attemps = 0;
  // Loop until we're reconnected
  while (!mqttIsConnected() && attemps<10) {
    attemps ++;
    if(serDebug) Serial.print("reconectando MQTT...");
    //memoriaSD.saveIntoLogMsg("reconectando MQTT(" + String(__mqttServerConnected) + ")...", administracion.freeSpaceReportSerial() , WiFiProcess.wifiIsConnected()?"Conectado":"Desconectado", mqttIsConnected()?"Conectado":"Desconectado", false);   
    // Create a random client ID
    String clientId = obtenerIdCliente();
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
      if(serDebug) Serial.println("Conectado con exito");
      //memoriaSD.saveIntoLogMsg("Conectado con exito", administracion.freeSpaceReportSerial() , WiFiProcess.wifiIsConnected()?"Conectado":"Desconectado", mqttIsConnected()?"Conectado":"Desconectado", false);   
      // Once connected, publish an announcement...
      mqttClient.publish("testMQTT", "probando estacion metereologica");
      // ... and resubscribe
      mqttClient.subscribe(outTopic);
    } else {
      if(serDebug) Serial.print("fallo, rc=");
      if(serDebug) Serial.print(mqttClient.state());
      if(serDebug) Serial.println(" intentando nuevamente en 5 segundos");
     // memoriaSD.saveIntoLogMsg("Fallo, rc = " + String(mqttClient.state()), administracion.freeSpaceReportSerial() , WiFiProcess.wifiIsConnected()?"Conectado":"Desconectado", mqttIsConnected()?"Conectado":"Desconectado", true);   
      // Wait 5 seconds before retrying
      setMQTTServer();
      delay(5000);
    }
  }
}
void callback(char* topic, byte* payload, unsigned int length) {
  if(serDebug) Serial.println(topic);
  if(serDebug) Serial.println("Message arrived [");
  if(serDebug) Serial.println("] ");
  String strTopic = String(topic);
  String strOutTopic = String(outTopic);
  strOutTopic.trim();
  strTopic.trim();
  if(serDebug) Serial.println(strTopic.indexOf(strOutTopic));
  if(strTopic.indexOf(strOutTopic) > 0 || strTopic == strOutTopic)
  {
    String mensaje = "";
    for (int i = 0; i < length; i++) {
      if(serDebug)
      {
        //if(serDebug) Serial.print((char)payload[i]);
        mensaje +=  (char)payload[i];
      }
    }
    if(serDebug) Serial.println("");
  
    mensaje.trim();
    if(mensaje == "actualizar")
    {
      if(serDebug) Serial.println("enviando actualizacion");
      procesamiento.setTimeToWait(0.1);
    }
  }
}
String obtenerIdCliente()
{
  String clientId = "ESP8266Client-";
  clientId += String(random(0xffff), HEX);
  return clientId;
}
