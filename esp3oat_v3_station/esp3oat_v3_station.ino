#include <Arduino.h>
#include "time.h"
#include <Wire.h>
#include "RTClib.h"

const uint8_t led = 25; // pin led for Highlights
const uint8_t led_wifi = 2; // pin led for Highlights
const uint8_t led_iot = 16; // pin led for Highlights

TaskHandle_t ChenHandle = NULL;
TaskHandle_t ECHandle = NULL;
TaskHandle_t DAHandle = NULL;
TaskHandle_t MAHandle = NULL;
bool choixPath = 1;
bool arret = 0;

// Clock  -----------------------------------
RTC_DS3231 rtc;
static char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


// Clock serveur  -----------------------------------
static const char* ntpServer = "pool.ntp.org";
static const uint16_t  gmtOffset_sec = 3600;            //// Long ???????
static const uint16_t   daylightOffset_sec = 3600;      // int


// GPS  -----------------------------------
static float latitude[10], longitude[10];

// Compteur ou "click"  -----------------------------------
static uint8_t protocole;
//unsigned long printTime;
static uint8_t switchMode = 0;
//static uint8_t clickCaptureWebWS = 0;
//static int etape = 0;
//static int compteur = 0;
//static int clickCaptureWeb = 0;    // Voir ce qui sert
//static int clickCaptureWebSub = 0;
//static bool switchTS = 0;
//static bool switchSD = 0;
//static bool initw = 0;
//static int ibearing = 0;

static uint8_t tension = 0;
static float tensionAlim = 0;
const float tensionMax = 3.3 / (1 - 20 / 31.8);
static float tamponAlim = 1;

static uint8_t lastPinState = 1;
static uint8_t pinState;


uint16_t maxPower = 1000;
uint8_t maxAngle = 90;

#include "CarteSD.h"
#include "Joystick.h"
#include "NRFL.h"
#include "Serveur.h"
#include "MQTT.h"
#include "EcranOLED.h"

// ++  -----------------------------------
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

void verification(void *parameter) {
  while (1) {
    // Verification connection mqtt
    if (protocole == 3) {
      if (mqtt.connected()) {
        mqtt.loop();
        digitalWrite(led_iot, HIGH);
      }
      else {
        digitalWrite(led_iot, LOW);
        protocole = 2;
      }
    }

    // Verification connection Wifi
    if (WiFi.status() != WL_CONNECTED) {
      digitalWrite(led_wifi, LOW);
      protocole = 1;
      server.end();
    }
    else {
      // ERREUR ?
      //WebSerial.println(WiFi.RSSI());
      if (mqtt.connected()) {
        mqtt.loop();
        digitalWrite(led_iot, HIGH);
        protocole = 3;
      }
      else {
        digitalWrite(led_iot, LOW);
        protocole = 2;
      }
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void butonWifi(void *parameter) {
  while (1) {
    if ( digitalRead(WIFI_TRIGGER_PIN) == LOW) {
      xTaskCreatePinnedToCore(chenillard, "CH", 4096, NULL, 9, &ChenHandle, app_cpu);

      server.end(); // sans cette ligne, impossible de superposer WM et HTML page esp3oat
      WiFiManager wm;
      wm.setConfigPortalTimeout(timeout);
      if (!wm.autoConnect("OnDemandAP")) {
        protocole = 1;
      }
      else {
        vTaskDelete(ChenHandle);
        digitalWrite(led_wifi, LOW);
        digitalWrite(led_iot, LOW);
        digitalWrite(led, HIGH);

        digitalWrite(led_wifi, HIGH);
        reconnect();
        if (mqtt.connected()) {
          digitalWrite(led_iot, HIGH);
          struct tm timeinfo; // server clock
          if (!getLocalTime(&timeinfo)) {
            //Serial.println("Failed to obtain time");
            //return;
          }
          else {
            //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
            char timeHour[3];
            strftime(timeHour, 3, "%H", &timeinfo);
            uint8_t inttimeHour = atoi(timeHour);
            char timeMinute[3];
            strftime(timeMinute, 3, "%M", &timeinfo);
            uint8_t inttimeMinute = atoi(timeMinute);
            char timeSecond[3];
            strftime(timeSecond, 3, "%S", &timeinfo);
            uint8_t inttimeSecond = atoi(timeSecond);
            char timeYear[3];
            strftime(timeYear, 3, "%Y", &timeinfo);
            uint16_t inttimeYear = atoi(timeYear);
            char timeDay[3];
            strftime(timeDay, 3, "%d", &timeinfo);
            uint8_t inttimeDay = atoi(timeDay);
            char timeMonth[3];
            strftime(timeMonth, 3, "%m", &timeinfo);
            uint8_t inttimeMonth = atoi(timeMonth);
            rtc.adjust(DateTime(inttimeYear, inttimeMonth, inttimeDay, inttimeHour, inttimeMinute, inttimeSecond));
          }
          protocole = 3;
        }
        else {
          digitalWrite(led_iot, LOW);  // si echec mqtt
          protocole = 2;
        }
      }
      delay(100);
      server.begin();
      delay(100);
      tamponAlim = 1;
    }
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

void envoieCommande(void *parameter) {
  while (1) {
    if ( switchMode == 0 ) { // Joystick
      valPotX = analogRead(pinAxeX_joystick);
      valPotY = analogRead(pinAxeY_joystick);
      EnvoyerDonneesRadio();
    }
    else if ( switchMode == 1 ) { // Joystick WEB
      donneesW.swsMode = switchMode;
      if ( angle > 180 ) angle = 360 - angle;
      donneesW.valDroiteGauche = angle;
      //donnees.valAvancerReculer = map(vitesse, 0, 100, 0, 255);  // Info : reculer
      donneesW.valAvancerReculer = map(vitesse, 0, 100, 1000, maxPower);  // Info : reculer
      //Serial.print(donneesW. valDroiteGauche); Serial.print("  "); Serial.println(donneesW.valAvancerReculer); //Serial.print(donneesW.captureESP);
      radioW.write(&donneesW, sizeof(donneesW));
    }
    else if (switchMode == 2) {
      //Serial.println("envoie commande Manu supprimé");
      vTaskDelete(ECHandle);
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void demarrageAuto(void *parameter) {
  while (1) {
    //pinState = digitalRead(0);
    pinState = 1;
    if (!pinState && lastPinState) {
      switchMode = 2;
    }
    lastPinState = pinState;

    if (switchMode == 2) {
      Serial.println("démarrage mode auto");
      donneesW.valAvancerReculer = 1000;
      donneesW.valDroiteGauche = 90;
      donneesW.swsMode = switchMode;
      donneesWA.swsMode = switchMode;
      radioW.write(&donneesW, sizeof(donneesW));
      //delay(1);
      while (1) {
        if (radioR.available()) {
          radioR.read(&confirmation, sizeof(confirmation));
          if (confirmation.conf == 1) {
            xTaskCreatePinnedToCore(modeAuto, "MA", 8192, NULL, 7, &MAHandle, app_cpu);
            Serial.println("confirmation et création mode auto et suppression demarrage auto");
            vTaskDelete(DAHandle);
          }
        }
      }
    }
    vTaskDelay(300 / portTICK_PERIOD_MS);
  }
}

void modeAuto(void *parameter) {
  //Serial.println("mode Auto actif");
  uint8_t i = 0;
  float distance_ref = 0;

  if (choixPath == 0) {
    //Serial.println("choix des coordonnées par WEB SERVEUR");
    //Serial.print("nouveau point de ref");
    donneesWA.latRef = latPath[0];
    donneesWA.longRef = longPath[0];
    donneesWA.latRef = latitude[0];
    donneesWA.longRef = longitude[0];
    while ((switchMode == 2) && (i < 11)) {
      donneesW.swsMode = switchMode;
      distance_ref = get_gps_dist(donneesR.lat_gps, donneesR.lon_gps, donneesWA.latRef, donneesWA.longRef);
      if ((distance_ref < 2)) {
        i++;
        donneesWA.latRef = latitude[i];
        donneesWA.longRef = longitude[i];
        //Serial.print("nouveau point de ref");
        //Serial.print(donneesWA.latRef);
        //Serial.println(donneesWA.longRef);
        //Serial.println(i);
      }
      radioW.write(&donneesWA, sizeof(donneesWA));
      vTaskDelay(330 / portTICK_PERIOD_MS);
    }
  }

  else if (choixPath == 1) {
    Serial.println("choix des coordonnées par CARTE SD");
    Serial.print("nouveau point de ref");
    donneesWA.latRef = latPath[0];
    donneesWA.longRef = longPath[0];
    Serial.print(donneesWA.latRef);
    Serial.println(donneesWA.longRef);
    while ((switchMode == 2) && (i < cp.getRowsCount())) {
      //donneesW.swsMode = switchMode;
      distance_ref = get_gps_dist(donneesR.lat_gps, donneesR.lon_gps, donneesWA.latRef, donneesWA.longRef);
      //Serial.print("distance : ");
      //Serial.println(distance_ref);
      if ((distance_ref < 1)) {
        i++;
        donneesWA.latRef = latPath[i];
        donneesWA.longRef = longPath[i];
        Serial.print("nouveau point de ref");
        Serial.print(donneesWA.latRef);
        Serial.println(donneesWA.longRef);
        Serial.println(i);
      }
      radioW.write(&donneesWA, sizeof(donneesWA));
      vTaskDelay(330 / portTICK_PERIOD_MS);
    }
  }
  //Serial.print("fin mode auto");
  switchMode = 0;
  donneesW.swsMode = switchMode;
  for (uint8_t a = 0 ; a < 10 ; a++) {
    radioW.write(&donneesWA, sizeof(donneesWA));
    delay(20);
  }
  xTaskCreatePinnedToCore(demarrageAuto, "DA", 4096, NULL, 6, &DAHandle, app_cpu);
  xTaskCreatePinnedToCore(envoieCommande, "EC", 4096, NULL, 7, &ECHandle, app_cpu);
  vTaskDelete(MAHandle);
}

void dataBoat(void *parameter) {
  while (1) {
    if (radioR.available()) {
      // printLocalTime();
      radioR.read(&donneesR, sizeof(donneesR)); 

      DateTime now = rtc.now();

      //Serial.print(now.year(), DEC);
      //Serial.print('/');
      //Serial.print(now.month(), DEC);
      //Serial.print('/');
      //Serial.print(now.day(), DEC);
      //Serial.print(" (");
      //Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
      //Serial.print(") ");
//      Serial.print("   ");
//      Serial.print(now.hour(), DEC);
//      Serial.print(':');
//      Serial.print(now.minute(), DEC);
//      Serial.print(':');
//      Serial.print(now.second(), DEC);

      //Serial.println();
      //Serial.print(" T (air) = "); 
      Serial.print(donneesR.lat_gps,7);
      Serial.print("\t");
      //Serial.print( " | Lon = " ); 
      Serial.print(donneesR.lon_gps,7);
      Serial.print("\t");
      Serial.print(donneesR.temp);
      Serial.print("\t");
      //Serial.print(" °C | H = "); 
      Serial.print(donneesR.hum);
      Serial.print("\t");
      //Serial.print( " % | dsr = " );  Serial.print(donneesR.distanceSR04);
      //Serial.print(" % | Profondeur = "); 
      Serial.print(donneesR.seagroundDistance);
      Serial.print("\t");
      //Serial.print( " mm | Angle = " );  
      //Serial.print(donneesR.bearing);
      //Serial.print("\t");
      //Serial.print( " ° | T (eau) = " ); 
      Serial.print(donneesR.tempSub);
      Serial.print("\t");
      //Serial.print( " °C | Vbat = " ); 
      //Serial.print((float)(donneesR.Vbat / 10));
      //Serial.print( " V | Lat = " ); 
      //Serial.print( " | Vitesse = " ); 
      Serial.print(donneesR.vitesse);
      Serial.print("\t");
      //Serial.print( " km/h | Angle = " ); 
      Serial.print(donneesR.heading);
      Serial.println("");
      

      //Serial.println(" ° ");

      temp2 = donneesR.temp; // Gauges
      hum2 = donneesR.hum;

      lat_Str = String(donneesR.lat_gps) + "\t\t";
      appendFile(SD, "/Data.txt", lat_Str.c_str());
      lon_Str = String(donneesR.lon_gps) + "\t\t";
      appendFile(SD, "/Data.txt", lon_Str.c_str());
      depth_Str = String(donneesR.seagroundDistance) + "\t\t";
      appendFile(SD, "/Data.txt", depth_Str.c_str());
      tempSub_Str = String(donneesR.tempSub) + "\t\t";
      appendFile(SD, "/Data.txt", tempSub_Str.c_str());
      temp_Str = String(donneesR.temp) + "\t\t";
      appendFile(SD, "/Data.txt", temp_Str.c_str());
      hum_Str = String(donneesR.hum) + "\t\t";
      appendFile(SD, "/Data.txt", hum_Str.c_str());
      speed_Str = String(donneesR.vitesse) + "\t\t";
      appendFile(SD, "/Data.txt", speed_Str.c_str());
      heading_Str = String(donneesR.heading) + "\n";
      appendFile(SD, "/Data.txt", heading_Str.c_str());

      if (protocole == 3) {
        // mqtt -----------------------------------
        StaticJsonBuffer<500> JSONbuffer;
        JsonObject& JSONencoder = JSONbuffer.createObject();
        JSONencoder["device"] = "ESP3oat";
        JSONencoder["mode"] = "MANU";
        JSONencoder["led"] = donneesW.headlights;
        JSONencoder["air_temp"] = donneesR.temp;
        JSONencoder["humidity"] = donneesR.hum;
        JSONencoder["deapth"] = donneesR.seagroundDistance;
        JSONencoder["bearing"] = donneesR.bearing;
        JSONencoder["water_temp"] = donneesR.tempSub;
        JSONencoder["latitude"] = donneesR.lat_gps;
        JSONencoder["longitude"] = donneesR.lon_gps;
        JSONencoder["vitesse"] = donneesR.vitesse;
        JSONencoder["heading"] = donneesR.heading;
        char JSONmessageBuffer[200]; // a changer si besoin (si manque des caracteres)
        JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
        //Serial.println("Sending message to MQTT topic..");
        //Serial.println(JSONmessageBuffer);
        if (mqtt.publish("esp3oat/data", JSONmessageBuffer) == true) {
          //Serial.println("Success sending message");
        } else {
          //Serial.println("Error sending message");
          protocole = 2;
        }
      }
      events.send("ping", NULL, millis());
      events.send(getSensorReadings().c_str(), "new_readings" , millis());
      //TH_lastTime = millis();
      events.send("ping", NULL, millis());
      events.send(getSensorReadings2().c_str(), "new_readings2" , millis());
    }
    vTaskDelay(800 / portTICK_PERIOD_MS);
  }
}



void chenillard(void *parameter) {
  while (1) {
    digitalWrite(led, LOW);
    digitalWrite(led_wifi, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    digitalWrite(led_wifi, LOW);
    digitalWrite(led_iot, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    digitalWrite(led_iot, LOW);
    digitalWrite(led, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void tensionPiles(void *parameter) {
  while (1) {
    tension = analogRead(34);
    tensionAlim = map(tension, 0, 4095, 0, tensionMax);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void affichage(void *parameter)
{
  int t_protocole, t_tensionAlim, t_tensionBoat, t_switchMode;
  t_protocole = t_tensionAlim = t_tensionBoat = t_switchMode = 0;
  while (1) {
    if ((t_protocole != protocole) || (t_tensionAlim != tensionAlim) || /*(t_tensionBoat != donneesR.tensionBoat) ||*/ (t_switchMode != switchMode))
    {
      if (protocole == 1) {
        u8g2.firstPage();
        do {
          u8g2.setFont(u8g2_font_6x13_tf);
          u8g2.setCursor(46, 13);
          u8g2.print("ESP3oat");
          u8g2.setFont(u8g2_font_5x7_tf);
          u8g2.setCursor(15, 30);
          u8g2.print("WM :");
          u8g2.setCursor(35, 30);
          u8g2.print(" 192.168.4.1");
          u8g2.setCursor(15, 45);
          u8g2.print("S :");
          u8g2.setCursor(35, 45);
          u8g2.print("5.7");
          u8g2.setCursor(55, 45);
          u8g2.print("V");
          u8g2.setCursor(75, 45);
          u8g2.print("B :");
          u8g2.setCursor(95, 45);
          u8g2.print(((float)donneesR.Vbat) / 10, 1);
          u8g2.setCursor(117, 45);
          u8g2.print("V");
          u8g2.setCursor(15, 60);
          u8g2.print("P :");
          u8g2.setCursor(35, 60);
          u8g2.print(protocole);
          u8g2.setCursor(75, 60);
          u8g2.print("M :");
          u8g2.setCursor(95, 60);
          u8g2.print(switchMode);
        } while (u8g2.nextPage());
      }
      else {
        u8g2.firstPage();
        do {
          u8g2.setFont(u8g2_font_5x7_tf);
          u8g2.setCursor(48, 15);
          u8g2.print("ESP3OAT");
          u8g2.setCursor(15, 30);
          u8g2.print("IP :");
          u8g2.setCursor(38, 30);
          u8g2.print(WiFi.localIP());
          u8g2.setCursor(15, 45);
          u8g2.print("S :");
          u8g2.setCursor(35, 45);
          u8g2.print("5.6");
          u8g2.setCursor(55, 45);
          u8g2.print("V");
          u8g2.setCursor(75, 45);
          u8g2.print("B :");
          u8g2.setCursor(95, 45);
          u8g2.print(((float)donneesR.Vbat) / 10, 1);
          u8g2.setCursor(117, 45);
          u8g2.print("V");
          u8g2.setCursor(15, 60);
          u8g2.print("P :");
          u8g2.setCursor(35, 60);
          u8g2.print(protocole);
          u8g2.setCursor(75, 60);
          u8g2.print("M :");
          u8g2.setCursor(95, 60);
          u8g2.print(switchMode);
        } while (u8g2.nextPage());
      }

    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

float get_gps_dist(float lat1, float lon1, float lat2, float lon2)
{

  float x = 69.1 * (lat2 - lat1);
  float y = 69.1 * (lon2 - lon1) * cos(lat1 / 57.3);

  return sqrt(x * x + y * y) * 1609.344;
}


void arretUrgence(void *parameter) {
  while (1) {
    //if (digitalRead(0) == LOW)
    //arret = 1;

    pinState = digitalRead(0);
    if (digitalRead(0) && lastPinState == 0)
      lastPinState = 1;
    if (!pinState && lastPinState)
    {
      arret = 1;
      lastPinState = 0;
    }

    while (stopButton) {
      donneesW.valAvancerReculer = 1000;
      radioW.write(&donneesW, sizeof(donneesW));
    }

    while (arret == 1)
    {
      //Serial.println("while");
      donneesW.valAvancerReculer = 1000;
      radioW.write(&donneesW, sizeof(donneesW));

      pinState = digitalRead(0);
      if (digitalRead(0) && lastPinState == 0)
        lastPinState = 1;
      if (!pinState && lastPinState)
      {
        //Serial.println("arret");
        arret = 0;
        lastPinState =  0;
        break;
      }
    }
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

//------------------------------------------------------------------------------------------------------------------------------------------

void setup() {

  //------------------- Initialisation  -----------------------------------
  Serial.begin(115200); // Serial
  pinMode(WIFI_TRIGGER_PIN, INPUT_PULLUP); // Boutton demarage wifi

  pinMode(0, INPUT_PULLUP); // Button pas a pas

  protocole = 1;
  //printTime = 0;
  mqtt.setServer("test.mosquitto.org", 1883);

  pinMode(led, OUTPUT); // pin led
  pinMode(led_wifi, OUTPUT); // pin led
  pinMode(led_iot, OUTPUT); // pin led
  pinMode(34, INPUT);

  digitalWrite(led, HIGH);
  digitalWrite(led_wifi, HIGH);
  digitalWrite(led_iot, HIGH);
  delay(500);
  //digitalWrite(led, LOW);
  digitalWrite(led_wifi, LOW);
  digitalWrite(led_iot, LOW);

  //---------------- Ecran OLED ---------------------------
  pinMode(13, OUTPUT);

  u8g2.begin();
  u8g2.clearDisplay();
  u8g2.setPowerSave(0);

  // CLOCK (and server at the end) -----------------------------------

  if (! rtc.begin()) {
    //Serial.println("Couldn't find RTC");
    while (1);
  }
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // rtc.adjust(DateTime(2022, 1, 21, 3, 0, 0));
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // Clock server


  // ---------------------- SPIFFS  ----------------------------
  InitSPIFFS();

  // ---------------------- Server ----------------------------
  ServerRequest();

  // --------------------- Joystick  ---------------------------
  InitJoystick();

  // ----------------------- NRF -------------------------------
  InitNRF();

  //----------------------- Carte SD ---------------------------
  InitSD();

  // Creation tâches
  xTaskCreatePinnedToCore(verification, "V", 4096, NULL, 5, NULL, app_cpu);
  xTaskCreatePinnedToCore(butonWifi, "BW", 4096, NULL, 6, NULL, app_cpu);
  xTaskCreatePinnedToCore(envoieCommande, "EC", 4096, NULL, 7, &ECHandle, app_cpu);
  xTaskCreatePinnedToCore(dataBoat, "DB", 16384, NULL, 8, NULL, app_cpu);
  xTaskCreatePinnedToCore(tensionPiles, "TA", 4096, NULL, 3, NULL, app_cpu);
  xTaskCreatePinnedToCore(demarrageAuto, "DA", 4096, NULL, 6, &DAHandle, app_cpu);
  xTaskCreatePinnedToCore(affichage, "AF", 4096, NULL, 4, NULL, app_cpu);
  xTaskCreatePinnedToCore(arretUrgence, "AU", 4096, NULL, 10, NULL, app_cpu);

  // -------------------------------------------------------------------------------------------------------------------------------

}

void loop() {

}
