#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_2_SW_I2C u8g2(U8G2_R0, 33, 13);


void printESP3OAT() {
  //Wire.endTransmission(0x68);
  Wire.endTransmission();
  Wire.beginTransmission(0x3C);
  //Wire.begin(0x3C);

  u8g2.begin();
  u8g2.setPowerSave(0);

  u8g2.clearDisplay();

  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.setCursor(50, 15);
    u8g2.print("ESP3OAT");
  } while (u8g2.nextPage());

  //Wire.endTransmission(0x3C);
  Wire.endTransmission();
  Wire.beginTransmission(0x68);
  Wire.begin(0x68);
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void printIP() {
  Wire.endTransmission();
  Wire.beginTransmission(0x3C);
  Wire.begin(0x3C);

  u8g2.begin();
  u8g2.setPowerSave(0);

  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.setCursor(5, 30);
    u8g2.print("IP : ");
    u8g2.setCursor(35, 30);
    u8g2.print(WiFi.localIP());
  } while (u8g2.nextPage());

  Wire.endTransmission();

  Wire.begin();
  Wire.beginTransmission(0x68);
  
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // Clock server
}
