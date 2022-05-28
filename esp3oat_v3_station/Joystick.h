#define pinBpJoystick   36     // [Entr�e] Lorsqu'on appuie au centre du joystick, ce BP est activ� (actif � l'�tat bas)
#define pinAxeX_joystick  39    // [Entr�e] Mesure la tension du potentiom�tre d'axe X du joystick
#define pinAxeY_joystick  35    // [Entr�e] Mesure la tension du potentiom�tre d'axe Y du joystick
uint16_t etatBPjoystick;         // Variable 16 bits, indiquant l'�tat du BP du joystick (valeur : HIGH ou LOW, soit 1 ou 0)
uint16_t valPotX;                // Variable 16 bits, qui contiendra une lecture 10 bits (0..1023), repr�sentant la tension du point milieu du potentiom�tre d'axe X
uint16_t valPotY;                // Variable 16 bits, qui contiendra une lecture 10 bits (0..1023), repr�sentant la tension du point milieu du potentiom�tre d'axe Y
uint8_t vitesse = 0; // Web
uint8_t angle = 90;  // Web


void InitJoystick()
{
  pinMode(pinBpJoystick, INPUT);
  //digitalWrite(pinBpJoystick, HIGH);
}
