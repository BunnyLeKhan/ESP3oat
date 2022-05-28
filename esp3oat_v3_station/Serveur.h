#include <WiFiManager.h>
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include <SPI.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>
#include <ArduinoJson.h> // sensors Gauge 
#include <WebSerial.h>

const char* http_username = "ESP3oat";
const char* http_password = "trinatronics";

bool stopButton = 0;

void recvMsg(uint8_t *data, size_t len) {
  String d = "";
  for (int i = 0; i < len; i++) {
    d += char(data[i]);
  }
  WebSerial.println(d);

  if (data[0] == 'C') {
    if (data[1] == 'M') {
      if (data[2] == '0') {
        switchMode = 0;
      }
      if (data[2] == '1') {
        switchMode = 1;
      }
      if (data[2] == '2') {
        switchMode = 2;
      }
    }
    if (data[1] == 'P') {
      if (data[2] == '1') {
        protocole = 1;
      }
      if (data[2] == '2') {
        protocole = 2;
      }
      if (data[2] == '3') {
        protocole = 3;
      }
    }
  }

  if (data[0] == 'M') {
    if (data[1] == 'P') {
      maxPower = 1000 * ((int)(char(data[2])) - 48) + 100 * ((int)(char(data[3])) - 48) + 10 * ((int)(char(data[4])) - 48) + ((int)(char(data[5])) - 48);
      WebSerial.println(maxPower);
    }
    if (data[1] == 'A') {
      maxAngle = 10 * ((int)(char(data[2])) - 48) +  ((int)(char(data[3])) - 48);
      WebSerial.println(maxAngle);
    }
  }

  if (data[0] == 'A') {
    if (data[1] == 'P') {
      donneesWA.Kp = (float)(1000 * ((int)(char(data[2])) - 48) + 100 * ((int)(char(data[3])) - 48) + 10 * ((int)(char(data[4])) - 48) + ((int)(char(data[5])) - 48)) / 100;
      WebSerial.println(donneesWA.Kp);
    }
    if (data[1] == 'I') {
      donneesWA.Ki = (float)(1000 * ((int)(char(data[2])) - 48) + 100 * ((int)(char(data[3])) - 48) + 10 * ((int)(char(data[4])) - 48) + ((int)(char(data[5])) - 48)) / 100;
      WebSerial.println(donneesWA.Ki);
    }
    if (data[1] == 'D') {
      donneesWA.Kd = (float)(1000 * ((int)(char(data[2])) - 48) + 100 * ((int)(char(data[3])) - 48) + 10 * ((int)(char(data[4])) - 48) + ((int)(char(data[5])) - 48)) / 100;
      WebSerial.println(donneesWA.Kd);
    }
  }

  if (d == "MODE?") {
    WebSerial.println(switchMode);
  }
  if (d == "PROTOCOLE?") {
    WebSerial.println(protocole);
  }
  if (d == "KP?") {
    WebSerial.println(donneesWA.Kp);
  }
  if (d == "KI?") {
    WebSerial.println(donneesWA.Ki);
  }
  if (d == "KD?") {
    WebSerial.println(donneesWA.Kd);
  }
  if (d == "LEDON") {
    digitalWrite(led, HIGH);
  }
  if (d == "LEDOFF") {
    digitalWrite(led, LOW);
  }
  if (d == "STOP") {
    stopButton = 1;
  }
  if (d == "GO") {
    stopButton = 0;
  }
  if (d == "DELETEDATA") {
    writeFile(SD, "/Data.txt", "");
  }
}


// Wifi  -----------------------------------
WiFiClient  client;
#define WIFI_TRIGGER_PIN 12
uint8_t timeout = 60; // seconds to run for

// Webserver gestion  -----------------------------------
AsyncWebServer server(80); // Create AsyncWebServer object on port 80
AsyncEventSource events("/events"); // Create an Event Source on /events


// Jauge temperature / humidité
JSONVar readings;
JSONVar readings2;

//unsigned long TH_lastTime = 0; // Timer variables
//unsigned long TH_timerDelay = 2000;
float temp2;
float hum2;

// Get Sensor Readings and return JSON object  ------------------------------
String getSensorReadings() {
  readings["temperature"] = String(temp2);
  readings["humidity"] =  String(hum2);
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

String getSensorReadings2() {
  readings2["sensor1"] = String(temp2);
  readings2["sensor2"] = String(donneesR.tempSub);
  readings2["sensor3"] = String(hum2);
  readings2["sensor4"] = String(donneesR.lat_gps, 7);
  readings2["sensor5"] = String(donneesR.lon_gps, 7);
  readings2["sensor6"] = String(donneesR.seagroundDistance);
  readings2["sensor7"] = String(donneesR.vitesse);

  String jsonString = JSON.stringify(readings2);
  return jsonString;
}

// Initialize SPIFFS  -----------------------------------
void InitSPIFFS() {
  if (!SPIFFS.begin())
  { //Serial.println("Erreur SPIFFS...");
    return;
  }
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file)
  { //Serial.print("File: ");
    //Serial.println(file.name());
    file.close();
    file = root.openNextFile();
  }
}


void ServerRequest()
{
  // Web Server Root URL  -----------------------------------
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    switchMode = 0;
    request->send(SPIFFS, "/MainMenu.html", "text/html");
  });

  server.on("/ManualMode.html", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    switchMode = 1;
    request->send(SPIFFS, "/ManualMode.html", "text/html");
  });

  server.on("/AutomaticMode", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/AutomaticMode.html", "text/html");
  });

  server.on("/CaptureGPS.html", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/CaptureGPS.html", "text/html");
  });

  server.on("/CarteSD.html", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/CarteSD.html", "text/html");
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/style.css", "text/css");
  });

  server.on("/w3.css", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/w3.css", "text/css");
  });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/script.js", "text/javascript");
  });

  server.on("/script-manual.js", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/script-manual.js", "text/javascript");
  });

  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(401);
  });

  server.on("/logged-out", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/logged-out.html", "text/html");
  });

  server.on("/jquery-3.6.0.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/jquery-3.6.0.min.js", "text/javascript");
  });

  server.on("/SendData", HTTP_POST, [](AsyncWebServerRequest * request) { // On r�cup�re la vitesse et l'angle du joystick
    if (request->hasParam("vitesse", true))
    {
      String message1;
      message1 = request->getParam("vitesse", true)->value();
      vitesse = message1.toInt();
    }
    if (request->hasParam("angle", true))
    {
      String message2;
      message2 = request->getParam("angle", true)->value();
      angle = message2.toInt();
    }
    request->send(204);
  });

  server.on("/SendZero", HTTP_POST, [](AsyncWebServerRequest * request)  // On fixe la vitesse � 0 et l'angle � 90 quand on relache le joystick
  {
    if (request->hasParam("vitesse", true))
      vitesse = 0;
    if (request->hasParam("angle", true))
      angle = 90;

    request->send(204);
  });

  server.on("/SendCoord", HTTP_POST, [](AsyncWebServerRequest * request)
  {
    for (uint8_t i = 0; i < 10; i++)
    {
      latitude[i] = 0;
      longitude[i] = 0;
    }
    if (request->hasParam("lat1", true)) {
      String message = request->getParam("lat1", true)->value();
      latitude[1] = message.toFloat();
    }
    if (request->hasParam("lon1", true)) {
      String message = request->getParam("lon1", true)->value();
      longitude[1] = message.toFloat();
    }
    if (request->hasParam("lat2", true)) {
      String message = request->getParam("lat2", true)->value();
      latitude[2] = message.toFloat();
    }
    if (request->hasParam("lon2", true)) {
      String message = request->getParam("lon2", true)->value();
      longitude[2] = message.toFloat();
    }
    if (request->hasParam("lat3", true)) {
      String message = request->getParam("lat3", true)->value();
      latitude[3] = message.toFloat();
    }
    if (request->hasParam("lon3", true)) {
      String message = request->getParam("lon3", true)->value();
      longitude[3] = message.toFloat();
    }
    if (request->hasParam("lat4", true)) {
      String message = request->getParam("lat4", true)->value();
      latitude[4] = message.toFloat();
    }
    if (request->hasParam("lon4", true)) {
      String message = request->getParam("lon4", true)->value();
      longitude[4] = message.toFloat();
    }
    if (request->hasParam("lat5", true)) {
      String message = request->getParam("lat5", true)->value();
      latitude[5] = message.toFloat();
    }
    if (request->hasParam("lon5", true)) {
      String message = request->getParam("lon5", true)->value();
      longitude[5] = message.toFloat();
    }
    if (request->hasParam("lat6", true)) {
      String message = request->getParam("lat6", true)->value();
      latitude[6] = message.toFloat();
    }
    if (request->hasParam("lon6", true)) {
      String message = request->getParam("lon6", true)->value();
      longitude[6] = message.toFloat();
    }
    if (request->hasParam("lat7", true)) {
      String message = request->getParam("lat7", true)->value();
      latitude[7] = message.toFloat();
    }
    if (request->hasParam("lon7", true)) {
      String message = request->getParam("lon7", true)->value();
      longitude[7] = message.toFloat();
    }
    if (request->hasParam("lat8", true)) {
      String message = request->getParam("lat8", true)->value();
      latitude[8] = message.toFloat();
    }
    if (request->hasParam("lon8", true)) {
      String message = request->getParam("lon8", true)->value();
      longitude[8] = message.toFloat();
    }
    if (request->hasParam("lat9", true)) {
      String message = request->getParam("lat9", true)->value();
      latitude[9] = message.toFloat();
    }
    if (request->hasParam("lon9", true)) {
      String message = request->getParam("lon9", true)->value();
      longitude[9] = message.toFloat();
    }
    if (request->hasParam("lat10", true)) {
      String message = request->getParam("lat10", true)->value();
      latitude[10] = message.toFloat();
    }
    if (request->hasParam("lon10", true)) {
      String message = request->getParam("lon10", true)->value();
      longitude[10] = message.toFloat();
    }

    request->send(204);

    //Serial.println("\n\nNouveau parcours\n");
    for (uint8_t i = 1; i < 11; i++) {
      if (latitude[i] != 0 && longitude[i] != 0) {
//        Serial.print("Point ");
//        Serial.print(i);
//        Serial.print(" | ");
//        Serial.print(latitude[i]);
//        Serial.print(" | ");
//        Serial.println(longitude[i]);
      }
    }
  });

  server.on("/SendPID", HTTP_POST, [](AsyncWebServerRequest * request)
  {
    //Serial.println("OK");
    if (request->hasParam("Kp", true)) {
      String message = request->getParam("Kp", true)->value();
      donneesWA.Kp = message.toFloat();
    }
    if (request->hasParam("Ki", true)) {
      String message = request->getParam("Ki", true)->value();
      donneesWA.Ki = message.toFloat();
    }
    if (request->hasParam("Kd", true)) {
      String message = request->getParam("Kd", true)->value();
      donneesWA.Kd = message.toFloat();
    }

    request->send(204);

//    Serial.println("\n\nPID\n");
//    Serial.println(donneesWA.Kp);
//    Serial.println(donneesWA.Ki);
//    Serial.println(donneesWA.Kd);
  });

  server.on("/mode", HTTP_GET, [](AsyncWebServerRequest * request) {
    switchMode = 1;
    //Serial.println(switchMode);
    request->send(200);
  });

  server.on("/modeAuto", HTTP_GET, [](AsyncWebServerRequest * request) {
    //switchMode = 2;
    //Serial.println(switchMode);
    request->send(200);
  });



  server.on("/on", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(led, HIGH);
    donneesW.headlights = 1;
    request->send(200);
  });

  server.on("/off", HTTP_GET, [](AsyncWebServerRequest * request) {
    digitalWrite(led, LOW);
    donneesW.headlights = 0;
    request->send(200);
  });

  server.on("/coord", HTTP_GET, [](AsyncWebServerRequest * request) {
    choixPath = 1;
    request->send(200);
  });

  server.on("/autostart", HTTP_GET, [](AsyncWebServerRequest * request) {
    switchMode = 2;
    //Serial.println("wsh bbew");
    request->send(200);
  });

  server.on("/autostop", HTTP_GET, [](AsyncWebServerRequest * request) {
    switchMode = 0;
    request->send(200);
  });

  server.on("/seagroundDistance", HTTP_GET, [](AsyncWebServerRequest * request) {
    int val = donneesR.seagroundDistance;
    String sgd = String(val);
    request->send(200, "text/plain", sgd);
  });

  server.on("/BatBoat", HTTP_GET, [](AsyncWebServerRequest * request) {
    //int val = 20;
    String bct = String((((float)donneesR.Vbat) / 10) * 100 / 13);
    request->send(200, "text/plain", bct);
  });

  server.on("/BatContr", HTTP_GET, [](AsyncWebServerRequest * request) {

    String bbt = String((tensionAlim) * 100 / tensionMax);
    request->send(200, "text/plain", bbt);
  });

  server.serveStatic("/", SPIFFS, "/");

  // Gauges
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest * request) {
    String json = getSensorReadings();
    request->send(200, "application/json", json);
    json = String();
  });

  server.on("/readings2", HTTP_GET, [](AsyncWebServerRequest * request) {
    String json = getSensorReadings2();
    request->send(200, "application/json", json);
    json = String();
  });

  fs::FS& fs = SD;
  fs.open("/Data.txt");

  server.on("/Data.txt", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    fs::FS& fs = SD;
    request->send(fs, "/Data.txt", "text");
  });

  events.onConnect([](AsyncEventSourceClient * client) {
    if (client->lastId()) {
      //Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  WebSerial.begin(&server);
  WebSerial.msgCallback(recvMsg);
  //  -----------------------------------
}
