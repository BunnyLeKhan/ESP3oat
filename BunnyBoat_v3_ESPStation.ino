//// BunnyBoat ESPStation 
//// v.3.0 
//// WIFI   SPI:nRF24l01  DHT11   WEB:LED   ThingSpeak  Joystick
//// idea : FreeRTOS or an other interrupt/operating system


// Bibliotheque  -----------------------------------
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPI.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>
#include "printf.h"
#include "nRF24L01.h"
#include "RF24.h"
#include "time.h"
#include "ThingSpeak.h"


// Wifi  -----------------------------------
//const char* ssid = "Samsung Guillaume";
//const char* password = "Bunnylekhan";
const char *ssid = "Livebox-F220";
const char *password = "JAnopadANK4nk4go72";
WiFiClient  client;
const int led = 2; // pin led for Highlights


// Thing Speak  -----------------------------------
unsigned long myChannelNumber = 1651525;
const char * myWriteAPIKey = "2ZI76GJ5Q1ZZL07C";
int a = 0;


// Webserver gestion  -----------------------------------
AsyncWebServer server(80); // Create AsyncWebServer object on port 80
AsyncEventSource events("/events"); // Create an Event Source on /events


// Clock serveur  -----------------------------------
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
void printLocalTime(){
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S"); }


// Joystick  -----------------------------------
#define pinBpJoystick     34     // [Entrée] Lorsqu'on appuie au centre du joystick, ce BP est activé (actif à l'état bas)
#define pinAxeX_joystick  33    // [Entrée] Mesure la tension du potentiomètre d'axe X du joystick
#define pinAxeY_joystick  35    // [Entrée] Mesure la tension du potentiomètre d'axe Y du joystick
int etatBPjoystick;         // Variable 16 bits, indiquant l'état du BP du joystick (valeur : HIGH ou LOW, soit 1 ou 0)
int valPotX;                // Variable 16 bits, qui contiendra une lecture 10 bits (0..1023), représentant la tension du point milieu du potentiomètre d'axe X
int valPotY;                // Variable 16 bits, qui contiendra une lecture 10 bits (0..1023), représentant la tension du point milieu du potentiomètre d'axe Y


// Communication Arduino-ESP  ----------------------------------- 
#define pinCE_RF24        4     // On associe la broche "CE" du NRF24L01 à la pin D7 de l'arduino
#define pinCSN_RF24       5     // On associe la broche "CSN" du NRF24L01 à la pin D8 de l'arduino
RF24 radio(pinCE_RF24, pinCSN_RF24);    // Instanciation du NRF24L01
bool radioNumber = 1;
bool role = false;  // true = TX role, false = RX role
const byte adresses[][6] = {"00001", "00002"};         // Mise au format "byte array" du nom du tunnel

// ---------------
struct DonneesAenvoyer {
  int valAvancerReculer;     // int = 2 octets (0..65535), utilisés de -255 à +255  (-255 signifie ici reculer à vitesse maximale, et +255, avancer à vitesse maximale)
  int valDroiteGauche;       // int = 2 octets (0..65535), utilisés de -255 à +255  (-255 signifie ici tourner à gauche pleinement, et +255, signifie tourner pleinement à droite)
  int captureESP;
  int highlights; //int prelevement; 
  }; DonneesAenvoyer donnees;

// ----------------
struct DonneesArecevoir {
  int temp;     // int = 2 octets (0..65535), utilisés de -255 à +255  (-255 signifie ici reculer à vitesse maximale, et +255, avancer à vitesse maximale)
  int hum;       // int = 2 octets (0..65535), utilisés de -255 à +255  (-255 signifie ici tourner à gauche pleinement, et +255, signifie tourner pleinement à droite)
  int distanceSR04;
  int seagroundDistance; 
  //float gpslong;
  //float gpslat;
  //float commande; 
  }; DonneesArecevoir donnees2;



// Compteur ou "click"  -----------------------------------
int etape = 0;
int compteur = 0;
int clickCaptureWeb = 0;
int clickCaptureWebSub = 0;
bool switchMode = 0;
bool switchTS = 0;
bool switchSD = 0;


// Jauge temperature / humidité (cf. Random Nurd...)  -----------------------------------
// Json Variable to Hold Sensor Readings
JSONVar readings;
unsigned long lastTime = 0; // Timer variables
unsigned long timerDelay = 5000;
int temp2; 
int hum2;


// Initialize SPIFFS  -----------------------------------
void initSPIFFS() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}


// Initialize WiFi  -----------------------------------
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}


// -----------------------------------
//  ----------------------------------- 
//  -----------------------------------


void setup() {

  // Initialisation  -----------------------------------
  Serial.begin(115200); // Serial 
  initWiFi(); // Wifi
  pinMode(led, OUTPUT); // pin led 
  digitalWrite(led, LOW);
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // Clock server 
  printLocalTime(); // print clock 
  pinMode(0, INPUT_PULLUP); // Button Capture
  

  // SPIFFS  -----------------------------------
  initSPIFFS(); //  html, css, javascript
  if(!SPIFFS.begin())
  { Serial.println("Erreur SPIFFS...");
    return; }
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while(file)
  { Serial.print("File: ");
    Serial.println(file.name());
    file.close();
    file = root.openNextFile(); }


  // Web Server Root URL  -----------------------------------
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");  });
    
  server.on("/w3.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/w3.css", "text/css");  });
  
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/script.js", "text/javascript");  });

  server.on("/capture", HTTP_GET, [](AsyncWebServerRequest *request) {
    clickCaptureWeb = 1;
    request->send(200);  });

  server.on("/mode", HTTP_GET, [](AsyncWebServerRequest *request)  {
    switchMode = !switchMode;
    Serial.println(switchMode);
    request->send(200);  });

  server.on("/thingSpeak", HTTP_GET, [](AsyncWebServerRequest *request)  {
    switchTS = !switchTS;
    Serial.println(switchTS);
    request->send(200);  });

  server.on("/sd", HTTP_GET, [](AsyncWebServerRequest *request)  {
    switchSD = !switchSD;
    Serial.println(switchSD);
    request->send(200);  });

  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request)  {
    digitalWrite(led, HIGH);
    request->send(200);  });

  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request)  {
    digitalWrite(led, LOW);
    request->send(200);  });

  server.on("/distanceSR04", HTTP_GET, [](AsyncWebServerRequest *request)  {
    int val = donnees2.distanceSR04;
    String dsr = String(val);
    request->send(200, "text/plain", dsr);  });

  server.on("/seagroundDistance", HTTP_GET, [](AsyncWebServerRequest *request) {
    int val = donnees2.seagroundDistance;
    String sgd = String(val);
    request->send(200, "text/plain", sgd);  });

  server.serveStatic("/", SPIFFS, "/");

  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getSensorReadings();
    request->send(200, "application/json", json);
    json = String();  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  server.begin();
  //  -----------------------------------


  // Joystick  -----------------------------------
  pinMode(pinBpJoystick, INPUT);      // Entrée BP joystick
  digitalWrite(pinBpJoystick, HIGH);


  // Communication Arduino-ESP module NRF24L01+   -----------------------------------
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);
  radio.begin();                 
  radio.setChannel(125);
  radio.openWritingPipe(adresses[radioNumber]);     // Ouverture du tunnel en "ÉCRITURE" (avec le "nom" qu'on lui a donné)
  radio.openReadingPipe(1, adresses[!radioNumber]);  // Ouverture du tunnel en mode LECTURE, avec le "nom" qu'on lui a donné
  radio.setPALevel(RF24_PA_MIN);      // Sélection d'un niveau "MINIMAL" d'émission, pour communiquer (car pas besoin d'une forte puissance ici, pour nos essais)
  printf_begin();             // needed only once for printing details


}  // end setup


 // -----------------------------------
 // -----------------------------------
 // -----------------------------------


void loop() {


  // Refresh Web Temperature and humidity  -----------------------------------
  if ((millis() - lastTime) > timerDelay) {
    // Send Events to the client with the Sensor Readings Every 10 seconds
    events.send("ping",NULL,millis());
    events.send(getSensorReadings().c_str(),"new_readings" ,millis());
    lastTime = millis();
  }


  // Buttun and code algorythme  -----------------------------------
  static uint8_t lastPinState = 1;
  uint8_t pinState = digitalRead(0);
  if (!pinState && lastPinState) { donnees.captureESP = 1; }
  else if (donnees.captureESP == 1) { donnees.captureESP = 1; }
  else if (clickCaptureWeb == 1) { clickCaptureWeb = 0; donnees.captureESP = 1; }
  else {donnees.captureESP = 0;}
  lastPinState = pinState;


  // Writting mode  -----------------------------------
  if ( donnees.captureESP == 0 || etape == 0) {
    if (donnees.captureESP == 1) etape = 1;
    radio.stopListening();              // Arrêt de l'écoute du nRF24 (signifiant qu'on va émettre, et non recevoir, ici)
    etatBPjoystick = digitalRead(pinBpJoystick);
    valPotX = analogRead(pinAxeX_joystick);
    valPotY = analogRead(pinAxeY_joystick);
    EnvoyerDonneesRadio();  
  }


  // Lecture alarme  -----------------------------------
  //  for (int k = 0; k < 10; k++) {
  //    if ( donnees.captureESP == 0 ) {
  //      radio.startListening();
  //      if (radio.available()) {
  //          radio.read(&donnees2, sizeof(donnees2));
  //          delayMicroseconds(2);
  //          Serial.print("DistanceSR04 = ");
  //          Serial.println(donnees2.distanceSR04);  }}}


  // Reading mode -----------------------------------
  if ( donnees.captureESP == 1 ) {
      radio.startListening(); 
      delayMicroseconds(2); 
      if (radio.available()) { 

        printLocalTime();
        donnees.captureESP = 0;  etape = 0;  compteur = 0;
        radio.read(&donnees2, sizeof(donnees2)); //radio.read(&donnees2, sizeof(donnees2)); //radio.read(&donnees2, sizeof(donnees2)); 
        delayMicroseconds(5);   
        Serial.print("[Données envoyé] T = "); Serial.print(donnees2.temp);
        Serial.print(" °C | H = "); Serial.print(donnees2.hum); 
        Serial.print( " % | dsr = " );  Serial.print(donnees2.distanceSR04);
        Serial.print(" cm | seagroundDistance = "); Serial.print(donnees2.seagroundDistance); Serial.println( " mm" ); 
        temp2 = donnees2.temp; 
        hum2 = donnees2.hum;

        // ThingSpeak request server ----------------
        if(switchTS == 1) {
          // Temperature 
          int x = ThingSpeak.writeField(myChannelNumber, 1, donnees2.temp, myWriteAPIKey);
          while( x != 200 ) {x = ThingSpeak.writeField(myChannelNumber, 1, donnees2.temp, myWriteAPIKey);}
          if(x == 200){ Serial.println("Channel 1 update successful."); a = 1; }
          else{ Serial.println("Problem updating channel. HTTP error code " + String(x)); } 
          delay(5000); 
          // Humidity
          int y = ThingSpeak.writeField(myChannelNumber, 2, donnees2.hum, myWriteAPIKey);
          while ( y != 200 ) {y = ThingSpeak.writeField(myChannelNumber, 2, donnees2.hum, myWriteAPIKey);}
          if (y == 200){  Serial.println("Channel 2 update successful."); a = 0; }
          else {Serial.println("Problem updating channel. HTTP error code " + String(y));} 
        } 
        // SD request ----------------
        if(switchSD == 1) {
        
        }

      } // radio available endif -------------
      compteur ++;  
      if (compteur == 10000) { compteur = 0; donnees.captureESP = 0; etape = 0; } 
  } // ending mode ending -------------


}  // end loop 


// -----------------------------------
// -----------------------------------
// -----------------------------------


// Fonction send message  ------------------------------
void EnvoyerDonneesRadio() {
  if (valPotX > 2000)         donnees.valDroiteGauche = map(valPotX, 2000, 4095, 90, 180);    // Info : tourner à gauche
  else if (valPotX < 1700)    donnees.valDroiteGauche = 90 - map(valPotX, 1700, 0, 0, 90);        // Info : tourner à droite
  else                      donnees.valDroiteGauche = 90;
  if (valPotY > 2000)         donnees.valAvancerReculer = map(valPotY, 2000, 4095, 0, -255);  // Info : reculer
  else if (valPotY < 1700)    donnees.valAvancerReculer = map(valPotY, 0, 1700, 255, 0);      // Info : avancer
  else                      donnees.valAvancerReculer = 0;
  radio.write(&donnees, sizeof(donnees));
  radio.write(&donnees, sizeof(donnees)); 
  radio.write(&donnees, sizeof(donnees));
}


// Get Sensor Readings and return JSON object  ------------------------------
String getSensorReadings(){
  readings["temperature"] = String(temp2);
  readings["humidity"] =  String(hum2);
  //readings["distanceSR04"] =  String(dsr);
  String jsonString = JSON.stringify(readings);
  return jsonString;
}
