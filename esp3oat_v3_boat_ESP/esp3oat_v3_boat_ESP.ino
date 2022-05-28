#include <Arduino.h>
#include <Servo.h>
#include "printf.h"
#include "RF24.h"
#include "nRF24L01.h"
#include <Wire.h>
#include "OneWire.h"
#include <SPI.h>
#include <Stepper.h>

//#define printInterval 1000 // envoyer demande capture
//#define printInterval2 300

// Communication Bateau-Station  -----------------------------------
#define pinCEW_RF24       4     // On associe la broche "CE" du NRF24L01 à la pin D7 de l'arduino
#define pinCSNW_RF24      5

#define pinCER_RF24       14     // On associe la broche "CSN" du NRF24L01 à la pin D8 de l'arduino
#define pinCSNR_RF24      27

RF24 radioW(pinCEW_RF24, pinCSNW_RF24);    // Instanciation du NRF24L01
RF24 radioR(pinCER_RF24, pinCSNR_RF24);

const byte adresses[][6] = {"11111", "22222"};         // Mise au format "byte array" du nom du tunnel

// ---------------
struct DonneesArecevoir {
  uint16_t valAvancerReculer;
  uint8_t valDroiteGauche;
  bool headlights;
  uint8_t swsMode;
}; DonneesArecevoir donneesR = {1000, 90, 0, 0};

struct DonneesArecevoirAuto {
  float latRef;
  float longRef;
  uint8_t swsMode;
  float Kp;
  float Ki;
  float Kd;
}; DonneesArecevoirAuto donneesRA = {0, 0, 0, 0, 0, 0};



// ------------------Communication série Arduino-ESP ---------------------------------------
struct ConfirmationTransfert {
  bool conf;
}; ConfirmationTransfert confirmation;

#define RXD2 16
#define TXD2 17

struct DonneesAenvoyer {
  uint8_t temp;
  uint8_t hum;
  //float profondeurCapteur;
  uint16_t seagroundDistance;
  uint8_t bearing;
  float tempSub;
  //  float pH;
  float lat_gps;
  float lon_gps;
  float vitesse;
  uint16_t heading;
  uint8_t Vbat;
}; DonneesAenvoyer donneesW;

struct DonneesArduino {
  float A_temp;
  float A_hum;
  int A_seagroundDistance;
  float A_tempSub;
  //  float pH;
}; DonneesArduino donneesAM;

//unsigned long printTime, printTime2;

uint16_t tension;
const float tensionMax = 3.3 * 4;


// LED  -----------------------------------
const uint8_t led = 12; // pin led for Highlights

// Wire module  -----------------------------------
#define ADDRESS 0x60 //defines address of compass

// Moteurs -----------------------------------
const uint8_t srv_pin = 33; // analog pin connected to servo
const uint8_t brsh_pin = 32; // analog pin connected to servo
Servo myservo;
Servo brushless;


// MPP
const uint16_t stepsPerRevolution = 2048;  // change this to fit the number of steps per revolution
const uint8_t rolePerMinute = 15;
Stepper myStepper(stepsPerRevolution, 25, 26, 32, 13);


// Mode AUTO
//byte AUTO = 0;
//byte current_wp = 0;

#include "Parcours.h"
#include "GPS.h"
#include "Compass.h"


// ++  -----------------------------------
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

TaskHandle_t LSHandle = NULL;
TaskHandle_t DAHandle = NULL;
TaskHandle_t MAHandle = NULL;

void captureDataLoger(void *parameter) {
  while (1) {

    getGPSData();         // Recupere lat, lon, vitesse, direction du GPS
    //printData();

    donneesW.lat_gps = f_latitude;
    donneesW.lon_gps = f_longitude;
    donneesW.vitesse = f_speed;
    donneesW.heading = i_heading;

    byte highByte;
    //int int_;
    I2C1.beginTransmission(ADDRESS);      //starts communication with cmps03
    I2C1.write(1);                         //Sends the register we wish to read
    I2C1.endTransmission();
    I2C1.requestFrom(ADDRESS, 1);
    while (I2C1.available() < 1);        //while there is a byte to receive
    highByte = I2C1.read();           //reads the byte as an integer
    Serial.println(highByte);
    donneesW.bearing = highByte;

    if (Serial2.available())
    {
      Serial2.readBytes((byte*)&donneesAM, sizeof donneesAM);
      Serial.print(donneesAM.A_temp); Serial.print("\t");
      Serial.print(donneesAM.A_hum); Serial.print("\t");
      Serial.print(donneesAM.A_seagroundDistance); Serial.print("\t");
      Serial.println(donneesAM.A_tempSub);
    }
    donneesW.temp = (int)donneesAM.A_temp;
    donneesW.hum = (int)donneesAM.A_hum;
    donneesW.seagroundDistance = donneesAM.A_seagroundDistance;
    donneesW.tempSub = donneesAM.A_tempSub;
    //printTime = millis();
    radioW.write(&donneesW, sizeof(donneesW));

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void lectureStation(void *parameter) {
  while (1) {
    ////// READ ///////////////////
    if (radioR.available()) {
      radioR.read(&donneesR, sizeof(donneesR));
      Serial.print("Angle : ");
      Serial.print(donneesR.valDroiteGauche);
      Serial.print(" | Puissance : ");
      Serial.println(donneesR.valAvancerReculer);
      if ((donneesR.swsMode == 1) || (donneesR.swsMode == 0)) {
        myservo.write(donneesR.valDroiteGauche);
        brushless.writeMicroseconds(donneesR.valAvancerReculer);
      }
    }

    if (donneesR.headlights == 1 ) digitalWrite(led, HIGH);
    //if (donneesR.swsMode == 2) vTaskDelete(LSHandle);
    else digitalWrite(led, LOW);
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

void demarrageAuto(void *parameter) {
  int a = 0;
  while (1) {
    if (donneesR.swsMode == 2) {
      Serial.println("démarrage mode auto");
      vTaskDelete(LSHandle);
      confirmation.conf = 1;
      for (a = 0; a < 10; a++) {
        radioW.write(&confirmation, sizeof(confirmation));
        delay(20);
      }
      Serial.println("confirmation et création mode auto et suppression demarrage auto");
      xTaskCreatePinnedToCore(modeAuto, "MA", 8192, NULL, 3, &MAHandle, app_cpu);
      donneesRA.swsMode = 2;
      vTaskDelete(DAHandle);
    }
    vTaskDelay(300 / portTICK_PERIOD_MS);
  }
}


void modeAuto(void *parameter) {
  Serial.println("mode Auto actif");
  digitalWrite(led, LOW);
  delay(500);
  digitalWrite(led, HIGH);
  delay(500);
  digitalWrite(led, LOW);
  delay(500);
  digitalWrite(led, HIGH);
  while (1) {
    if (donneesRA.swsMode == 2) {
      if (radioR.available()) {
        radioR.read(&donneesRA, sizeof(donneesRA));
        Serial.print(donneesRA.latRef); Serial.println(donneesRA.longRef);
      }
      wp_angle = get_gps_angle(f_latitude, f_longitude, donneesRA.latRef, donneesRA.longRef);

      //    byte highByte;
      //    int int_;
      //    I2C1.beginTransmission(ADDRESS);      //starts communication with cmps03
      //    I2C1.write(1);                         //Sends the register we wish to read
      //    I2C1.endTransmission();
      //    I2C1.requestFrom(ADDRESS, 1);
      //    while (I2C1.available() < 1);        //while there is a byte to receive
      //    highByte = I2C1.read();           //reads the byte as an integer
      //    wp_angle = 360 * highByte / 255;
      //    Serial.println(wp_angle);

      angle_error = compass_error(wp_angle, i_heading);   // A faire avec boussole pr plus de précision
      Serial.println(angle_error);

      if (angle_error > angle_sat)              // Saturation (angle_sat = angle de braquage max)
        angle_error = angle_sat;
      if (angle_error < -angle_sat)
        angle_error = -angle_sat;

      myservo.write(90 + donneesRA.Kp*angle_error);
      
      brushless.writeMicroseconds(1300);
      //Serial.println(1150 + abs(90-angle_error)*100/90);

      vTaskDelay(20 / portTICK_PERIOD_MS);
    }
    else {
      Serial.print("fin mode auto");
      donneesRA.swsMode = 0;
      donneesR.swsMode = 0;
      myservo.write(90);
      brushless.writeMicroseconds(1000);
      xTaskCreatePinnedToCore(demarrageAuto, "DA", 4096, NULL, 3, &DAHandle, app_cpu);
      xTaskCreatePinnedToCore(lectureStation, "LS", 4096, NULL, 2, &LSHandle, app_cpu);
      vTaskDelete(MAHandle);
    }
  }
}

void tensionBatterie(void *parameter) {
  while (1) {
    tension = analogRead(34);
    donneesW.Vbat = (uint8_t)(map(tension, 0, 4095, 0, tensionMax*10));
    Serial.println(donneesW.Vbat);
    vTaskDelay(4000 / portTICK_PERIOD_MS);
  }
}



void setup() {

  //printTime = 0;

  ////// Serial init
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, 15, 2);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);

  I2C1.begin(25, 26, 400000); //conects I2C
  pinMode(2, OUTPUT);
  pinMode(0, OUTPUT);
  digitalWrite(2, HIGH);
  digitalWrite(0, HIGH);

  /// LED
  pinMode(led, OUTPUT); // pin led
  digitalWrite(led, HIGH);

  /// Wire
  Wire.begin(); //conects I2C

  //// servo
  myservo.attach(srv_pin);
  myservo.write(90);// move servos to center position -> 90°
  brushless.attach(brsh_pin);
  brushless.write(0);// move servos to center position -> 90°

  radioR.begin();                      // Initialisation du module nRF24
  radioR.setChannel(125); //30
  radioR.openReadingPipe(1, adresses[0]);  // Ouverture du tunnel en mode LECTURE, avec le "nom" qu'on lui a donné
  radioR.setPALevel(RF24_PA_HIGH);      // Sélection d'un niveau "MINIMAL" pour communiquer (pas besoin de forte puissance, pour nos essais
  radioR.startListening();
  radioR.printDetails();

  radioW.begin();                      // Initialisation du module nRF24
  radioW.setChannel(125);   //20
  radioW.openWritingPipe(adresses[1]);     // Ouverture du tunnel en "ÉCRITURE" (avec le "nom" qu'on lui a donné)
  radioW.setPALevel(RF24_PA_HIGH);      // Sélection d'un niveau "MINIMAL" pour communiquer (pas besoin de forte puissance, pour nos essais
  radioW.stopListening();
  radioW.printDetails();

  // Mode AUTO
  wpts_nmbr = sizeof(parcours) / sizeof(waypoint);
  InitGPS();

  // ++  -----------------------------------

  xTaskCreatePinnedToCore(  // Use xTaskCreate() in vanilla FreeRTOS
    captureDataLoger,  // Function to be called
    "DL",   // Name of task
    16384,         // Stack size (bytes in ESP32, words in FreeRTOS)
    NULL,         // Parameter to pass to function
    4,            // Task priority (0 to configMAX_PRIORITIES - 1)
    NULL,         // Task handle
    app_cpu);     // Run on one core for demo purposes (ESP32 only)

  xTaskCreatePinnedToCore(  // Use xTaskCreate() in vanilla FreeRTOS
    lectureStation,  // Function to be called
    "LS",   // Name of task
    4096,         // Stack size (bytes in ESP32, words in FreeRTOS)
    NULL,         // Parameter to pass to function
    2,            // Task priority (0 to configMAX_PRIORITIES - 1)
    &LSHandle,         // Task handle
    app_cpu);     // Run on one core for demo purposes (ESP32 only)

  xTaskCreatePinnedToCore(demarrageAuto, "DA", 4096, NULL, 3, &DAHandle, app_cpu);
  xTaskCreatePinnedToCore(tensionBatterie, "TB", 4096, NULL, 1, NULL, app_cpu);
}


void loop() {

  /////////////// WRITE ///////////

}
