#define ADDRESS 0x60 //defines address of compass
TwoWire I2C1 = TwoWire(1);

int angle_error = 0;
int angle_sat = 90; //180 ?


int compass_error(int set_Point, int current_Point)
{
    int error = 0;

    if (fabs(set_Point - current_Point) > 180)
    {
        if ((set_Point - current_Point) < -180)
        {
            error = (set_Point + 360) - current_Point;
        }
        else if ((set_Point - current_Point) > 180)
        {
            error = (set_Point - 360) - current_Point; // +??
        }
    }
    else
    {
        error = set_Point - current_Point;
    }

    return error;
}



/*int getCompassAngle()
{
    I2C1.beginTransmission(ADDRESS);      //starts communication with cmps03
    I2C1.write(1);                         //Sends the register we wish to read
    I2C1.endTransmission();
    I2C1.requestFrom(ADDRESS, 1);        //requests high byte
    while (I2C1.available() < 1);        //while there is a byte to receive
    highByte = I2C1.read();

    return ((float)highByte)* (360.0 / 255.0);
}*/
