#include "SD.h"
#include "FS.h"
#include <CSV_Parser.h>
//#include <Vector.h>

SPIClass SPISD(HSPI);
CSV_Parser cp(/*format*/ "ff", /*has_header*/ true, /*delimiter*/ ',');

// GPS  -----------------------------------
float *latPath;
float *longPath;
String lat_Str, lon_Str, depth_Str, tempSub_Str, temp_Str, hum_Str, speed_Str, heading_Str;

// --------------- Fonction pour cr�er et ecrire dans un fichier --------------------------------------------

void writeFile(fs::FS& fs, const char* path, const char* message) {
  File file = fs.open(path, FILE_WRITE);
  file.print(message);
  file.close();
}

// ------------------- Fonction pour ecrire a la suite d'un fichier ------------------------------------------

void appendFile(fs::FS& fs, const char* path, const char* message) {
  File file = fs.open(path, FILE_APPEND);
  file.print(message);
  file.close();
}


// Lecture du chemin auto ----------------------



void InitSD()
{
  SPISD.begin(26, 32, 17, 15);

  if (!SD.begin(15, SPISD)) {
    //Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    //Serial.println("No SD card attached");
    return;
  }

  // The line below (readSDfile) wouldn't work if SD.begin wasn't called before.
  // readSDfile can be used as conditional, it returns 'false' if the file does not exist.
  if (cp.readSDfile("/path.csv")) {
    latPath = (float*)cp["latitude"];
    longPath = (float*)cp["longitude"];

    if (longPath && latPath) {
      for (int row = 0; row < cp.getRowsCount(); row++) {
        Serial.print("row = ");
        Serial.print(row, DEC);
        Serial.print(", lat = ");
        Serial.print(latPath[row], 7);
        Serial.print(", long = ");
        Serial.println(longPath[row], 7);
      }
    } else {
      //Serial.println("ERROR: At least 1 of the columns was not found, something went wrong.");
    }

    // output parsed values (allows to check that the file was parsed correctly)
    cp.print(); // assumes that "Serial.begin()" was called before (otherwise it won't work)

  } else {
    //Serial.println("ERROR: File called '/file.csv' does not exist...");
  }
}
