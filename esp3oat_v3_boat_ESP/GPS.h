#include <SparkFun_u-blox_GNSS_Arduino_Library.h>

SFE_UBLOX_GNSS myGNSS;

long latitude, longitude;
float f_latitude, f_longitude;

long speed, heading;
int i_heading;
float f_speed;

//float wp_distance;
int wp_angle;


float get_gps_dist(float lat1, float lon1, float lat2, float lon2)
{

  float x = 69.1 * (lat2 - lat1);
  float y = 69.1 * (lon2 - lon1) * cos(lat1 / 57.3);

  return sqrt(x * x + y * y) * 1609.344;
}


int get_gps_angle(float lat1, float lon1, float lat2, float lon2)
{
  float angle;
  int angle_deg;

  float x = 69.1 * (lat2 - lat1);
  float y = 69.1 * (lon2 - lon1) * cos(lat1 / 57.3);

  angle = atan2(y, x);

  angle_deg = (int)(degrees(angle));

  if (angle_deg <= 1) {
    angle_deg = 360 + angle_deg;
  }
  return angle_deg;
}

void InitGPS()
{
  Wire.begin();

  if (myGNSS.begin() == false)
  {
    Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing."));
    while (1);
  }

  myGNSS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  //myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); //Save (only) the communications port settings to flash and BBR
  myGNSS.setNavigationFrequency(25);

}

int GDOP;
long head, pitch, roll;
int i_head,i_pitch,i_roll;

void getGPSData()
{
  latitude = myGNSS.getLatitude();
  longitude = myGNSS.getLongitude();
  speed = myGNSS.getGroundSpeed();
  heading = myGNSS.getHeading();

  f_latitude = (float)latitude / 10000000.0;
  f_longitude = (float)longitude / 10000000.0;
  i_heading = (int)(heading / 100000);
  f_speed = (float)speed * 0.0036;   // 1 Noeud = 1.852 km/h

  GDOP = myGNSS.getGeometricDOP();    // Indique la fiabilité de la mesure

  /*head = myGNSS.getVehicleHeading();
  pitch = myGNSS.getVehiclePitch();
  roll = myGNSS.getVehicleRoll();
  i_head = (int)head / 10000000;
  i_pitch = (int)pitch / 10000000;
  i_roll = (int)roll / 10000000;*/
}


void printData()
{
  /*Serial.print(myGNSS.getYear());
    Serial.print("-");
    Serial.print(myGNSS.getMonth());
    Serial.print("-");
    Serial.print(myGNSS.getDay());
    Serial.print(" ");
    Serial.print(myGNSS.getHour());
    Serial.print(":");
    Serial.print(myGNSS.getMinute());
    Serial.print(":");
    Serial.print(myGNSS.getSecond());
    Serial.print("\t");*/

  Serial.print(f_latitude, 7);
  Serial.print("\t");
  Serial.print(f_longitude, 7);
  Serial.print("\t");
  Serial.print(i_heading);
  Serial.print("\t");
  Serial.print(f_speed, 1);

  Serial.print("\t");
  Serial.print(GDOP/100.0);
  /*Serial.print("\t");
  Serial.print(i_head);
  Serial.print("\t");
  Serial.print(i_roll);
  Serial.print("\t");
  Serial.print(i_pitch);*/


  Serial.println();
}
