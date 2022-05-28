#include "printf.h"
#include "nRF24L01.h"
#include "RF24.h"


// Communication Station-Bateau  -----------------------------------
#define pinCER_RF24     14
#define pinCSNR_RF24    27
#define pinCEW_RF24     4
#define pinCSNW_RF24    5

RF24 radioR(pinCER_RF24, pinCSNR_RF24);
RF24 radioW(pinCEW_RF24, pinCSNW_RF24);

const byte adresses[][6] = { "11111", "22222" };

// ---------------
struct DonneesAenvoyer {
  uint16_t valAvancerReculer;
  uint8_t valDroiteGauche;
  bool headlights;
  uint8_t swsMode;
}; DonneesAenvoyer donneesW = {1000,90,1,0};

struct DonneesAenvoyerAuto {
  float latRef;
  float longRef;
  uint8_t swsMode;
  float Kp;
  float Ki;
  float Kd;
}; DonneesAenvoyerAuto donneesWA = {0,0,0,1.0,0.0,0.0};

// ----------------
struct DonneesArecevoir {
  uint8_t temp;
  uint8_t hum;
  //float profondeurCapteur;
  //uint16_t tensionBoat;
  uint16_t seagroundDistance;
  uint8_t bearing;
  float tempSub;
  //float pH;
  float lat_gps;
  float lon_gps;
  float vitesse;
  uint16_t heading;
  uint8_t Vbat;
}; DonneesArecevoir donneesR;

struct ConfirmationTransfert {
  bool conf;
}; ConfirmationTransfert confirmation;


// Fonction send message  ------------------------------
void EnvoyerDonneesRadio() {
  if (valPotY >= 2050)         donneesW.valDroiteGauche = map(valPotY, 2000, 4095, 90, 90 + maxAngle);  // Info : tourner � gauche
  else if (valPotY <= 1650)    donneesW.valDroiteGauche = 90 - map(valPotY, 1700, 0, 0, maxAngle);        // Info : tourner � droite
  else if ((1650 < valPotY) && (valPotY < 2050))
    donneesW.valDroiteGauche = 90;
  //  if (valPotY > 2000)         donneesW.valAvancerReculer = map(valPotY, 2000, 4095, 0, -255);  // Info : reculer
  //  else if (valPotY < 1700)    donneesW.valAvancerReculer = map(valPotY, 0, 1700, 255, 0);      // Info : avancer
  //  else                      donneesW.valAvancerReculer = 0;
  if (valPotX > 2050)         donneesW.valAvancerReculer = map(valPotX, 2000, 4095, 1000, maxPower);  // Info : reculer
  else if (valPotX < 1650)    donneesW.valAvancerReculer = map(valPotX, 0, 1700, maxPower, 1000);      // Info : avancer
  else                      donneesW.valAvancerReculer = 1000;
  donneesW.swsMode = switchMode;
  donneesWA.swsMode = switchMode;
  //Serial.print(donneesW. valDroiteGauche); Serial.print("  "); Serial.print("  "); Serial.println(donneesW.valAvancerReculer); //Serial.print(donneesW.captureESP);
  radioW.write(&donneesW, sizeof(donneesW));
}


void InitNRF()
{
  //  pinMode(15, OUTPUT);
  //  digitalWrite(15, HIGH);

  radioR.begin();
  radioR.setChannel(125);
  radioR.openReadingPipe(1, adresses[1]);  // Ouverture du tunnel en mode LECTURE, avec le "nom" qu'on lui a donné
  radioR.setPALevel(RF24_PA_HIGH);      // Sélection d'un niveau "MINIMAL" d'émission, pour communiquer (car pas besoin d'une forte puissance ici, pour nos essais)
  radioR.printDetails();
  printf_begin();             // needed only once for printing details

  radioW.begin();                      // Initialisation du module nRF24
  radioW.setChannel(125);
  radioW.openWritingPipe(adresses[0]);     // Ouverture du tunnel en "ÉCRITURE" (avec le "nom" qu'on lui a donné)
  radioW.setPALevel(RF24_PA_HIGH);      // Sélection d'un niveau "MINIMAL" pour communiquer (pas besoin de forte puissance, pour nos essais
  radioW.printDetails();

  radioR.startListening();
  radioW.stopListening();
}
