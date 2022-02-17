//// BunnyBoat ArduiBoat 
//// v.3.0 
//// WIFI   SPI:nRF24l01  DHT11   WEB:LED   ThingSpeak  Joystick
//// idea : FreeRTOS or an other interrupt/operating system


// Bibliotheque  -----------------------------------
#include <Servo.h>
#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#include "dht_nonblocking.h"
#include "SR04.h"


// Ultrasonic module instanciation  -----------------------------------
#define TRIG_PIN 12
#define ECHO_PIN 11
SR04 sr04 = SR04(ECHO_PIN, TRIG_PIN);


// SubWater distance module  -----------------------------------
#define TRIGPIN 3
#define ECHOPIN 5
float duration; float sgd;


// Temperature Sensor  -----------------------------------
#define DHT_SENSOR_TYPE DHT_TYPE_11
static const int DHT_SENSOR_PIN = 2; //36;
DHT_nonblocking dht_sensor( DHT_SENSOR_PIN, DHT_SENSOR_TYPE );

static bool measure_environment( float *temperature, float *humidity ) {
  static unsigned long measurement_timestamp = millis( );
  if ( millis( ) - measurement_timestamp > 3000ul ) {
    if ( dht_sensor.measure( temperature, humidity ) == true ) {
      measurement_timestamp = millis( );
      return ( true );
    }
  }
  return ( false );
}


// Servo  -----------------------------------
const int srv_pin = 10; // analog pin connected to servo
Servo myservo;


// Communication arduino-Esp nRF24l01  -----------------------------------
#define pinCE_RF24        7     // On associe la broche "CE" du NRF24L01 à la pin D7 de l'arduino
#define pinCSN_RF24       8     // On associe la broche "CSN" du NRF24L01 à la pin D8 de l'arduino
RF24 radio(pinCE_RF24, pinCSN_RF24);    // Instanciation du NRF24L01
bool radioNumber = 0;
bool role = false;  // true = TX role, false = RX role
const byte adresses[][6] = {"00001", "00002"};         // Mise au format "byte array" du nom du tunnel      // Mise au format "byte array" du nom du tunnel
struct DonneesArecevoir {
  long int valAvancerReculer;     // int = 2 octets (0..65535), utilisés de -255 à +255  (-255 signifie ici reculer à vitesse maximale, et +255, avancer à vitesse maximale)
  long int valDroiteGauche;       // int = 2 octets (0..65535), utilisés de -255 à +255  (-255 signifie ici tourner à gauche pleinement, et +255, signifie tourner pleinement à droite)
  long int captureESP;
}; DonneesArecevoir donnees;
struct DonneesAenvoyer {
  long int temp;     // int = 2 octets (0..65535), utilisés de -255 à +255  (-255 signifie ici reculer à vitesse maximale, et +255, avancer à vitesse maximale)
  long int hum;       // int = 2 octets (0..65535), utilisés de -255 à +255  (-255 signifie ici tourner à gauche pleinement, et +255, signifie tourner pleinement à droite)
  long int distanceSR04;
  long int seagroundDistance;
}; DonneesAenvoyer donnees2;


// MCC  -----------------------------------
//const int ENABLE = 4;
//const int DIRA = 41;
//const int DIRB = 40;


// Compteur et algorythme  -----------------------------------
int choix = 1;
bool alarm = 0;


//  -----------------------------------
//  -----------------------------------
//  -----------------------------------


void setup() {


  // Serial init
  Serial.begin(9600);
  Serial.println("Récepteur NRF24L01 de test");
  Serial.println("");

  // subwater detection pin 
  pinMode(ECHOPIN, INPUT); 
  pinMode(TRIGPIN, OUTPUT);

  // servo
  myservo.attach(srv_pin);
  myservo.write(90);// move servos to center position -> 90°

  //  mcc
  //  pinMode(ENABLE,OUTPUT); 
  //  pinMode(DIRA,OUTPUT);
  //  pinMode(DIRB,OUTPUT);

  // SPI default 
  pinMode(53, OUTPUT); 
  digitalWrite(53, HIGH);

  // NRF24l01 
  radio.begin();                      // Initialisation du module nRF24
  radio.setChannel(125);
  radio.openReadingPipe(1, adresses[!radioNumber]);  // Ouverture du tunnel en mode LECTURE, avec le "nom" qu'on lui a donné
  radio.openWritingPipe(adresses[radioNumber]);     // Ouverture du tunnel en "ÉCRITURE" (avec le "nom" qu'on lui a donné)
  radio.setPALevel(RF24_PA_MIN);      // Sélection d'un niveau "MINIMAL" pour communiquer (pas besoin de forte puissance, pour nos essais
  printf_begin();             // needed only once for printing details


} // end setup


//  -----------------------------------
//  -----------------------------------
//  -----------------------------------

void loop() {


  // Writting interruption  -----------------------------------
  if (choix == 0) {
    radio.stopListening();
    float temperature;
    float humidity;
    if ( measure_environment( &temperature, &humidity ) == true ) {
      
      donnees2.temp = temperature;
      donnees2.hum = humidity;
      
      // subwater distance reading 
      //        sgd = 0 ;
      //        digitalWrite(TRIGPIN, LOW);
      //        delayMicroseconds(2);
      //        digitalWrite(TRIGPIN, HIGH);
      //        delayMicroseconds(20);
      //        digitalWrite(TRIGPIN, LOW);
      //        duration = pulseIn(ECHOPIN, HIGH);
      //        sgd = (duration / 2) * 1.482;
      //        delayMicroseconds(50);
      donnees2.seagroundDistance = sgd;

      radio.write(&donnees2, sizeof(donnees2));
      radio.write(&donnees2, sizeof(donnees2));

      Serial.print("[Données envoyé] T = "); Serial.print(donnees2.temp);
      Serial.print(" deg. C | H = "); Serial.print(donnees2.hum);
      Serial.print( "% | dsr = " ); Serial.print(donnees2.distanceSR04);
      Serial.print(" cm | seagroundDistance = "); Serial.print(donnees2.seagroundDistance);
      Serial.println( "mm" );
      
      choix = 1;
      donnees.captureESP = 0;
    } // radioAvalable endif 
  } // fin choix 0


  // Listening routine  -----------------------------------
  if ( choix == 1 ) {
    radio.startListening();
    if (radio.available()) {
      radio.read(&donnees, sizeof(donnees));                        // Si des données viennent d'arriver, on les charge dans la structure nommée "donnees"
      //        Serial.print("[Données reçues] valAvancerReculer = ");        // … et on affiche tout ça sur le port série !
      //        Serial.print(donnees.valAvancerReculer);
      //        Serial.print(" | valDroiteGauche = ");
      //        Serial.print(donnees.valDroiteGauche);
      //        Serial.print(" | captureESP = ");
      //        Serial.println(donnees.captureESP);
    }
    
    myservo.write(donnees.valDroiteGauche);       // Servo command
    
    if (donnees.captureESP == 1) { choix = 0; }
    donnees2.distanceSR04 = sr04.Distance();
    if (donnees2.distanceSR04 <= 100) { alarm = 1; }
    else alarm = 0;
  }

  //    if ( alarm == 1 && choix == 1 ) {
  //      radio.stopListening();
  //      radio.write(&donnees2, sizeof(donnees2));
  //      delayMicroseconds(2);
  //      radio.write(&donnees2, sizeof(donnees2));
  //      delayMicroseconds(2);
  //      radio.write(&donnees2, sizeof(donnees2));
  //      delayMicroseconds(2);
  //      radio.write(&donnees2, sizeof(donnees2));
  //      delayMicroseconds(2);
  //      Serial.print("dsr = ");
  //      Serial.println(donnees2.distanceSR04);
  //    }
}


//  // MCC command in the loop
//     if(donnees.valAvancerReculer<0)
//    {
//        analogWrite(ENABLE,abs(donnees.valAvancerReculer));
//        digitalWrite(DIRA,LOW);
//        digitalWrite(DIRB,HIGH);
//    }
//    if(donnees.valAvancerReculer == 0)
//    {
//        analogWrite(ENABLE,0);
//        digitalWrite(DIRA,HIGH);
//        digitalWrite(DIRB,LOW);
//    }
//    if(donnees.valAvancerReculer>0)
//    {
//        analogWrite(ENABLE,donnees.valAvancerReculer);
//        digitalWrite(DIRA,HIGH);
//        digitalWrite(DIRB,LOW);
//    }
