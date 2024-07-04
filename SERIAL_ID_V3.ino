#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <MCUFRIEND_kbv.h>

MCUFRIEND_kbv tft;

//define colors
#define BLACK 0x0000
#define WHITE 0xFFFF
#define DARKGREEN 0x03E0
#define DARKCYAN 0x03EF
#define MAROON 0x7800
#define PURPLE 0x780F
#define YELLOW 0xfbd23
#define ORANGE 0xFD20
#define GREENYELLOW 0xAFE5
#define PINK 0xF81F

#define BUFFPIXEL 20


// Pins for the TFT screen
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

#define SD_CS 53  // Chip Select for SD Module


void displayImage(const char *filename, int x, int y);
void bmpDraw(const char *filename, int x, int y);
uint16_t read16(File f);
uint32_t read32(File f);

int IDLength;
String clockAction;
String hashes;
String clockDetails;
const char* clockTime;
const char* clockID;
int currentMembers = 0;

String time = "12:31:24";


void setup() {
  Serial.begin(9600);
  Serial1.begin(115200);  // for the esp8266 nodemcu
  pinMode(53, OUTPUT);

//for displaying picture on TFT
  initializeTFTandSD();
  displayImage("woof.bmp", 0,0);
  displayImage("fondNoir.bmp", 0,0);
  displayImage("wifi.bmp", 0,0);
//  delay(1000);
  displayImage("noWifi.bmp", 0,0);
  createNewFile();
}

void loop() {

  GetFromEsp();

}


void createNewFile(){
  if (!SD.begin(53)) {
    Serial.println("SD initialization failed!");
    return;
  }
  Serial.println("Initialization done.");

  if (!SD.exists("member.csv")) {
    File memberFile = SD.open("member.csv", FILE_WRITE);
    if (memberFile) {
      memberFile.println("CARD_UID;CLOCK_TIME");
      Serial.println("SD_OK");
    } else {
      Serial.println("Error creating member.csv");
      }
      memberFile.close();
    }
    if (!SD.exists("member2.csv")) {
    File enrollMemberFile = SD.open("member2.csv", FILE_WRITE);
    if (enrollMemberFile) {
      //enrollMemberFile.println("E9A6F39\nFACFC768\nE9A6F39\nFACFC768");
      Serial.println("SD_OK");
    } else {
      Serial.println("Error creating enrollMember.csv");
    }
    enrollMemberFile.close();
  }
  if (!SD.exists("index.csv")) {
      File indexFile = SD.open("index.csv", FILE_WRITE);
      if (indexFile) {
        indexFile.println("CARD_UID;INDEX");
        Serial.println("SD_OK");
      } else {
        Serial.println("Error creating index.csv");
        }
        indexFile.close();
      }
}

//--------------------- LCD FONCTIONS ---------------------

void initializeTFTandSD() {
  tft.reset();
  uint16_t identifier = tft.readID();
  Serial.println(identifier);
  
  if (identifier) {
    tft.begin(identifier);
    Serial.println(F("LCD driver initialized"));
  } else {
    Serial.print(F("Unknown LCD driver chip: "));
    Serial.println(identifier, HEX);
    return;
  }

  Serial.print(F("Initializing SD card..."));
  if (!SD.begin(SD_CS)) {
    Serial.println(F("failed!"));
    return;
  }
  Serial.println(F("OK!"));
}

void displayImage(const char *filename, int x, int y) {
  bmpDraw(filename, x, y);
  Serial.print("Getting image ....");
}

void bmpDraw(const char *filename, int x, int y) {
  File bmpFile;
  int bmpWidth, bmpHeight; // Largeur et hauteur en pixels
  uint8_t bmpDepth; // Profondeur de bits (doit être 24)
  uint32_t bmpImageoffset; // Début des données d'image dans le fichier
  uint32_t rowSize; // Pas toujours égal à bmpWidth; peut avoir un padding
  uint8_t sdbuffer[3 * BUFFPIXEL]; // Tampon de pixel en entrée (R+G+B par pixel)
  uint16_t lcdbuffer[BUFFPIXEL]; // Tampon de pixel en sortie (16 bits par pixel)
  uint8_t buffidx = sizeof(sdbuffer); // Position actuelle dans sdbuffer
  boolean goodBmp = false; // Passé à true lors de l'analyse d'en-tête valide
  boolean flip = true; // BMP est stocké de bas en haut
  int w, h, row, col;
  uint8_t r, g, b;
  uint32_t pos = 0, startTime = millis();
  uint8_t lcdidx = 0;
  boolean first = true;

  //if ((x >= ) || (y >= )) return;
  Serial.println(tft.width());
  Serial.println(tft.height());
  Serial.println();
  Serial.print(F("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');

  // Ouvrir le fichier demandé sur la carte SD
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.println(F("File not found"));
    return;
  }

  // Analyser l'en-tête BMP
  if (read16(bmpFile) == 0x4D42) { // Signature BMP
    Serial.println(F("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Lire et ignorer les octets du créateur
    bmpImageoffset = read32(bmpFile); // Début des données d'image
    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Lire l'en-tête DIB
    Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if (read16(bmpFile) == 1) { // # plans -- doit être '1'
      bmpDepth = read16(bmpFile); // bits par pixel
      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = non compressé

        goodBmp = true; // Format BMP supporté -- continuer!
        Serial.print(F("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // Les lignes BMP sont rembourrées (si nécessaire) à la limite de 4 octets
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // Si bmpHeight est négatif, l'image est en ordre descendant.
        // Ce n'est pas canon mais a été observé dans la nature.
        if (bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip = false;
        }

        // Recadrer la zone à charger
        w = bmpWidth;
        h = bmpHeight;
        if ((x + w - 1) >= tft.width())  w = tft.width() - x;
        if ((y + h - 1) >= tft.height()) h = tft.height() - y;

        // Définir la fenêtre d'adresse TFT aux limites de l'image recadrée
        tft.setAddrWindow(x, y, x + w - 1, y + h - 1);

        for (row = 0; row < h; row++) { 
          if (flip) 
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     
            pos = bmpImageoffset + row * rowSize;
          if (bmpFile.position() != pos) { 
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); 
          }

          for (col = 0; col < w; col++) { 
            if (buffidx >= sizeof(sdbuffer)) {
              if (lcdidx > 0) {
                tft.pushColors(lcdbuffer, lcdidx, first);
                lcdidx = 0;
                first = false;
              }
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; 
            }

            // Convertir le pixel du format BMP au format TFT
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            lcdbuffer[lcdidx++] = tft.color565(r, g, b);
          } // fin pixel
        } // fin ligne de scan
        // Écrire les données restantes sur l'écran
        if (lcdidx > 0) {
          tft.pushColors(lcdbuffer, lcdidx, first);
        }
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // fin goodBmp
    }
  }
  bmpFile.close();
  if (!goodBmp) Serial.println(F("BMP format not recognized."));
}


uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}


void displayTime(String Time){
  tft.setTextColor(BLACK);
  tft.setRotation(3);
  tft.setTextSize(5);
  tft.setCursor(40, 100);
  tft.println(Time);
}

//-------------------------------------------------

void GetFromEsp() {

  hashes = "";
  IDLength = 0;
  clockTime = "";
  clockID = "";
  String clockDetails = "";

  if (Serial1.available() > 0) {
    clockDetails = Serial1.readString();
    Serial.println(clockDetails);
    DynamicJsonDocument doc(128);
    DeserializationError error = deserializeJson(doc, clockDetails);
    clockID = doc["cardID"];
    String clID = UpperCase(String(clockID));
    clockTime = doc["time"];
      Serial.println("Received from ESP8266: " + clID + " at " + String(clockTime) + "  Saving on card...");
      fillMember(clID, clockTime);
      addMember(clID);
      //DelIndex("2");
//      displayImage("clockDevice1.bmp",0,0);
  }
}


void fillMember(String ID, String Time) {                      //clock in a member
  if (ID.length() > 0) {
    IDLength = ID.length();
    for (int i = 0; i < IDLength; i++) {
      hashes += "0";
    }
  }
  if (Time.length() == 0) {
    Time = hashes;
  }
  File memberFile = SD.open("member.csv", FILE_WRITE);
  if ((ID.length() > 0 )) {
    String rowData = ID + ";" + Time + "\n";
    memberFile.print(rowData);
    memberFile.close();
  }
}

void addMember(String ID) {                                                 //adding a member ID
  File memberFile = SD.open("member2.csv", FILE_WRITE);
  //if(!findStringInBigString(ID, memberFile.readStringUntil("\n"))){
    if ((ID.length() > 0 )) {
      String rowData = ID + ";" + "\n";
      memberFile.print(rowData);
      memberFile.close();
      indexMember(ID); 
  }
//  }
  
}


int countRows() {
  int rowCount = 0;
  File memberFile = SD.open("member2.csv");
  if (memberFile) {
    while (memberFile.available()) {
      String line = memberFile.readStringUntil('\n');
      if (line.length() > 0) {
        rowCount++;
      }
    }
    memberFile.close();
  } else {
    Serial.println("Error opening member2.csv");
  }
  return rowCount;
}

void indexMember(String ID) {
  File idxFile = SD.open("index.csv", FILE_WRITE);
  if (idxFile) {
    int num_rows = countRows();
    if (ID.length() > 0) {
      String rowData = ID + ";" + num_rows + "\n";
      idxFile.print(rowData);
    }
    idxFile.close();
  } else {
    Serial.println("Error opening index.csv");
  }
}


String delOnIndex(String index) {                                     
  File idxFile = SD.open("index.csv");
  if (idxFile) {
    String line;
    bool indexFound = false;
    while (idxFile.available()) {
      line = idxFile.readStringUntil('\n');
      int semicolonPos = line.indexOf(';');
      if (semicolonPos != -1) {
        String id = line.substring(0, semicolonPos);
        int lineIndex = line.substring(semicolonPos + 1).toInt();
        if (String(lineIndex) == index) {
          idxFile.close();
          return id;
        }
      }
    }
    idxFile.close();
    if (!indexFound) {
      return "nan";
    }
  } else {
    Serial.println("Error opening index.csv");
    return "nan";
  }
}

void DelIndex(String Ind){                                                //delete a member on index
  if(delOnIndex(Ind) != "nan"){
    delInBigString(Ind);
  }else{
    Serial.println("Data deletion Error! ");
  }

}

void checkMember(String checkID){                                         //grant member access
  String ReadDetails = "";
  File myFile = SD.open("member2.csv");
  while (myFile.available()) {
    ReadDetails = myFile.readStringUntil("\n");
    if (findStringInBigString(checkID,ReadDetails)) {
         Serial.println("member is enrolled");
    } else {

      Serial.println("member is not enrolled");
    }
  
    myFile.close();
}
}

void reading(String ScannedCard) {                                   //print all members enrolled
  String ReadDetails = "";
  File myFile = SD.open("member2.csv");
  while (myFile.available()) {
    ReadDetails = myFile.readStringUntil('\n');
    Serial.println(ReadDetails);
  }
  myFile.close();
}



String UpperCase(String str) {
    for (int i = 0; i < str.length(); i++) {
        str[i] = toupper((unsigned char)str[i]);
    }
    return str;
}


bool findStringInBigString(String searchString, String bigString) {

  int startIndex = 0;
  int endIndex = bigString.indexOf('\n', startIndex);

  while (endIndex != -1) {
    String subString = bigString.substring(startIndex, endIndex);
    subString.trim();
    Serial.println(subString + "and" + searchString);
   
    if (subString.equals(searchString)) {
      Serial.println("found");
      return true;
   
    }
      Serial.println("not found");
      startIndex = endIndex + 1;
      endIndex = bigString.indexOf('\n', startIndex);
    
  }

  // Check the last substring after the last newline
  String subString = bigString.substring(startIndex);
  if (subString.equals(searchString)) {
    return true;

  }

  return false;
}

void delInBigString(String searchString) {
  File originalFile = SD.open("member2.csv");
  if (!originalFile) {
    Serial.println("Error opening member2.csv");
    return;
  }

  File tempFile = SD.open("temp.csv", FILE_WRITE);
  if (!tempFile) {
    Serial.println("Error creating temp.csv");
    originalFile.close();
    return;
  }

  while (originalFile.available()) {
    String line = originalFile.readStringUntil('\n');
    line.trim();

    if (line != searchString) {
      tempFile.println(line);
    } else {
      Serial.println("Deleting: " + line);
    }
  }

  originalFile.close();
  tempFile.close();

  // Delete the original file
  SD.remove("member2.csv");

  // Copy temp file contents back to the original file
  tempFile = SD.open("temp.csv");
  originalFile = SD.open("member2.csv", FILE_WRITE);

  if (tempFile && originalFile) {
    while (tempFile.available()) {
      originalFile.write(tempFile.read());
    }
    tempFile.close();
    originalFile.close();

    // Delete the temp file
    SD.remove("temp.csv");
  } else {
    Serial.println("Error in file copy process");
  }
}




