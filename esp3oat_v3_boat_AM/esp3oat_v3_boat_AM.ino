// Bibliotheque  -----------------------------------
#include <Servo.h>
#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#include "dht_nonblocking.h"
//#include "SR04.h"
#include <Wire.h>
#include "OneWire.h"
#include "DallasTemperature.h"
#include "Arduino_FreeRTOS.h"


// Sonde Temperature -----------------------------------
#define WATER_TEMP_PIN A0
OneWire oneWire(WATER_TEMP_PIN);
DallasTemperature sensors(&oneWire);


// SubWater distance module  -----------------------------------
#define TRIGPIN 3
#define ECHOPIN 5
float duration = 0;
float sgd = 0;
double dTempWater;

// Temperature Sensor  -----------------------------------
#define DHT_SENSOR_TYPE DHT_TYPE_11
static const int DHT_SENSOR_PIN = 2; //36;
DHT_nonblocking dht_sensor( DHT_SENSOR_PIN, DHT_SENSOR_TYPE );


struct DonneesAenvoyer {
  float temp;
  float hum;
  long int seagroundDistance;
  float tempSub;
}; DonneesAenvoyer donnees2;


void setup() {

  Serial.begin(115200);
  Serial1.begin(115200);

  // subwater detection pin
  pinMode(ECHOPIN, INPUT);
  pinMode(TRIGPIN, OUTPUT);

  xTaskCreate(TaskSendData, "senddata", 2048, NULL, 2, NULL);
  xTaskCreate(TaskGetSensors, "getsensors", 2048, NULL, 3, NULL);
  xTaskCreate(TaskGetTemp, "gettemp", 2048, NULL, 1, NULL);

}


static bool measure_environment( float *temperature, float *humidity )
{
  static unsigned long measurement_timestamp = millis( );
  if ( millis( ) - measurement_timestamp > 2ul )
  {
    if ( dht_sensor.measure( temperature, humidity ) == true )
    {
      measurement_timestamp = millis( );
      return ( true );
    }
  }
  return ( false );
}

//  -----------------------------------

void loop() {

}

void TaskGetTemp( void *pvParameters __attribute__((unused)) )  // This is a Task.
{
  while (1) {
    float temperature;
    float humidity;
    while (1) {
      if ( measure_environment( &temperature, &humidity ) == true ) {
        donnees2.temp =  temperature;
        donnees2.hum =  humidity;
        Serial.println("temp valid");
        break;
      }
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void TaskGetSensors( void *pvParameters __attribute__((unused)) )  // This is a Task.
{
  while (1) {

    sensors.requestTemperatures();
    dTempWater = sensors.getTempCByIndex(0);
    donnees2.tempSub = (float)dTempWater;

    digitalWrite(TRIGPIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIGPIN, HIGH);
    delayMicroseconds(20);
    digitalWrite(TRIGPIN, LOW);
    duration = pulseIn(ECHOPIN, HIGH);
    sgd = (duration / 2) * 1.482;
    //sgd = (duration / 2) * 0.34;
    delayMicroseconds(5);
    donnees2.seagroundDistance = sgd;

    Serial.println("sgd valid");
    vTaskDelay(300 / portTICK_PERIOD_MS);
  }
}


void TaskSendData( void *pvParameters __attribute__((unused)) )  // This is a Task.
{
  while (1) {
    Serial1.write((byte *)&donnees2, sizeof donnees2);
    Serial.println("data Send");
    vTaskDelay(300 / portTICK_PERIOD_MS);
  }
}
