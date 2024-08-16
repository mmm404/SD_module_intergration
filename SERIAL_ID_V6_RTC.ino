#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <MCUFRIEND_kbv.h>
#include <CircularBuffer.hpp>
#include <Fonts/FreeSerifBold24pt7b.h>
#include <Fonts/FreeSerifBold18pt7b.h>
#include <Fonts/FreeSerifBold12pt7b.h>
#include <Keypad.h> //for the 4x4 keypad



int pos;
String curHr = "";
String curMin = "";
String CardData1 = "";
String CardData2 = "";
bool receiveTime = true;
bool masterCalled = false;
uint16_t c_color;
const int BUFFER_SIZE = 10;
CircularBuffer<String, BUFFER_SIZE> dataBuffer;
bool enrolling = false;
bool deleting = false;
MCUFRIEND_kbv tft;
int cursorposInd = 160;
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

// bool useMaster = false;
void displayImage(const char *filename, int x, int y);
void bmpDraw(const char *filename, int x, int y);
uint16_t read16(File f);
uint32_t read32(File f);
bool animate = true;
bool u_ind;
const byte ROWS = 4;
const byte COLS = 4;
String ind_UID = "";
String ind_UID1 = "";
String masterPin = "";
String masterPin1 = "";
String keypadPin = "9002";
String newPin = ""; //for password reset
String D_ata = "";
String rfidUid;
int blue=21,green=23, red=25; //RGB pins
int solenoidlock = 32;    //A13
// int sdModule = 14;
int buzzer = 20;
int cursorpos = 20;
char customKey;
// bool notQuit = false ;
String masterTag = "39E1D456"; //A-UID for the Master Tag
String masterTag2 = "59D7A218";
bool showStart = false;
//int pos; //menus for display
char choice; //Menu displayed after Mastertag is scanned
String action;
String scannedTag = "" ; //To store value of the scanned tag
String newEntry = "";
// bool entryPar;
String cardToDelete;
String codeToDelete;
int IDLength;
String clockAction;
String hashes;
String clockDetails;
const char* clockTime;
const char* clockID;
int currentMembers = 0;
unsigned long lastUpdateTime = 0;
const byte interruptPin = 20;
volatile byte state = LOW;
String Time = "12:31:24";
bool getReady = false;
// bool useWindow = false;
//void Print(){Serial.println("interrupt ok");return;}
int i;
int rest = 0;
volatile bool dataReadyFlag = false;
long int day = 86400000;//number of millisecond in one day
bool dataStored = false;
String currTime = "00:00";
String currT = "";
int currHr ;
int currMin ;
int currSec ;
long int t1 = 0;
long int t2 = 0;
long int t3 = 0;
long int passedTime = 0;
String receivedTag = "";
String receivedTagDel = "";
bool keyIn = false;
bool deleteMember;
String IndexToDelete = "";
 
// Array to represent keys on keypad
char hexaKeys[ROWS][COLS] = {
  {'*', '0', '#', 'D'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'1', '2', '3', 'A'},
};

byte rowPins[ROWS] = {A9,A6,A8,A7};  //connect to the row pinouts of the keypad
byte colPins[COLS] = {A13,A12,A11,A10}; //connect? toA the column pinouts of the keypad

// Create keypad object
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);


void setup() {
  i=0;
  Serial.begin(9600);
  Serial1.begin(115200);  // for the esp8266 nodemcu
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(blue, OUTPUT);

  pinMode(53, OUTPUT);
  //digitalWrite(53,HIGH);
  pinMode(interruptPin, INPUT);
  
//  digitalWrite(interruptPin,LOW);
  //attachInterrupt(digitalPinToInterrupt(interruptPin), dataReady, RISING);
  //displayLoadingAnimation();
  attachInterrupt(digitalPinToInterrupt(interruptPin), GetFromEsp, RISING);

//for displaying picture on TFT
  initializeTFTandSD();
 
  // displayLoadingAnimation();
   tft.setRotation(1);
  displayImage("cded.bmp",0,0);
  tft.setRotation(1);
  startWindow();

  
  createNewFile();
  
}


void loop() {

  t1 = millis();
  
    // if(!useWindow){
  if (dataReadyFlag) {
    handleSerialData();
  }
  
  else{
    // Serial.println("keyIn : " + keyIn);
    if(keyIn){
    animate = false ; 
    Keypadvalue();
    }else if(animate){
      slideAnimation();
    }

  }



  t2 = millis() + (t3-t2);

  // t2 = millis();


  sortTime((t2-t1));

  t3 = millis();

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




//---------------------------------------------------------------LCD FUNCTIONS--------


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

// Cette fonction ouvre un fichier Bitmap (BMP) et l'affiche aux coordonnées données.
#define BUFFPIXEL 20

void bmpDraw(const char *filename, int x, int y) {
  if (animate){

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
  uint32_t post = 0, startTime = millis();
  uint8_t lcdidx = 0;
  boolean first = true;

  //if ((x >= ) || (y >= )) return;
  // Serial.println(tft.width());
  // Serial.println(tft.height());
  // Serial.println();
  // Serial.print(F("Loading image '"));
  // Serial.print(filename);
  // Serial.println('\'');

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
//    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Lire l'en-tête DIB
    Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if (read16(bmpFile) == 1) { // # plans -- doit être '1'
      bmpDepth = read16(bmpFile); // bits par pixel
//      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = non compressé

        goodBmp = true; // Format BMP supporté -- continuer!
        // Serial.print(F("Image size: "));
        // Serial.print(bmpWidth);
        // Serial.print('x');
        // Serial.println(bmpHeight);

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

        for (row = 0; row < h; row++) { // Pour chaque ligne de scan...
          // Rechercher le début de la ligne de scan.  Cela peut sembler laborieux
          // de faire cela à chaque ligne, mais cette méthode couvre beaucoup
          // de détails comme le recadrage et le rembourrage des lignes de scan.
          // De plus, la recherche ne se fait que si la position du fichier doit
          // réellement changer (évite beaucoup de mathématiques de clusters dans
          // la bibliothèque SD).
          
            if (dataReadyFlag) {
              handleSerialData();
              return;
            }
          

          if (flip) // Bitmap est stocké de bas en haut (ordre normal BMP)
            post = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap est stocké de haut en bas
            post = bmpImageoffset + row * rowSize;
          if (bmpFile.position() != post) { // Besoin de chercher?
            bmpFile.seek(post);
            buffidx = sizeof(sdbuffer); // Forcer le rechargement du tampon
          }

          for (col = 0; col < w; col++) { // Pour chaque colonne...
            // Temps de lire plus de données de pixels?

            if (dataReadyFlag) {
              handleSerialData();
              return;
            }

            if (buffidx >= sizeof(sdbuffer)) { // En effet
              // Pousser le tampon LCD sur l'écran en premier
              if (lcdidx > 0) {
                tft.pushColors(lcdbuffer, lcdidx, first);
                lcdidx = 0;
                first = false;
              }
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Définir l'index au début
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
        // Serial.print(F("Loaded in "));
        // Serial.print(millis() - startTime);
        // Serial.println(" ms");
      } // fin goodBmp
    }
  }

  bmpFile.close();
  if (!goodBmp) Serial.println(F("BMP format not recognized."));

  }
}

// Ces fonctions lisent des types 16 et 32 bits à partir du fichier de la carte SD.
// Les données BMP sont stockées en little-endian, Arduino est également en little-endian.
// Peut nécessiter d'inverser l'ordre des sous-scripts si vous portez cela ailleurs.

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




void startWindow(){
  animate = true;
  keyIn = false;
  dataReadyFlag = false;
  tft.setRotation(1);
  tft.fillScreen(DARKGREEN);
  displayImage("savanne.bmp", 0,0);



  if(!masterCalled){

  tft.fillRoundRect(5,40,310,50,5,YELLOW );
  tft.drawRoundRect(5,40,310,50,5,BLACK);
  tft.setCursor(10,67); 
  tft.setTextColor(DARKGREEN);
  tft.setFont(&FreeSerifBold12pt7b);  
  tft.println("     Please Tap Your Card");

  }

  Serial.println("startwindow called");
  masterCalled = false ; 
  return;
}

void slideAnimation(){
  // character();
  Serial.println("keyin : "+ keyIn);
  
  tft.setRotation(1);
  displayImage("t0D.bmp", 1,168);
  displayImage("savHr.bmp", 125,0);


  if(animate){
  tft.setCursor(125,30);
  tft.setFont(&FreeSerifBold18pt7b);
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.print(currT);
  }

  displayImage("t2D.bmp", 0,168);
  displayImage("t4D.bmp", 0,168);
  displayImage("t6D.bmp",0,168);
  displayImage("t8D.bmp", 0,165);
  
  // if(keyIn || !animate){
  //   masterTagMenu();
  // }

}


void displayLoadingAnimation() {
  int count = 0;
  int centerX = 150; // X coordinate of the center of the circle
  int centerY = 120; // Y coordinate of the center of the circle
  int radius = 90;   // Radius of the circle
  int angle = 0;     // Current angle of rotation
  tft.setRotation(1);
  // Animation loop
  while (count < 365 ) {
    count++;
    int x = centerX + radius * cos(angle * PI / 180);
    int y = centerY + radius * sin(angle * PI / 180);
    tft.fillCircle(x, y, 4, BLACK); // Draw a white dot
    angle += 1; // Adjust speed of rotation as needed
    int prevX = centerX + radius * cos((angle - 10) * PI / 180);
    int prevY = centerY + radius * sin((angle - 10) * PI / 180);
    tft.fillCircle(prevX, prevY, 6, c_color); // Erase previous dot
    delay(25); // Adjust as needed
    if(count==200){
      c_color = WHITE;
      tft.fillRoundRect(80,90,140,40,10,YELLOW); //draws a filled Rectangle at x,y,width,height,radius,colour
      tft.setCursor(100,100); //sets cursor at x,y position
      tft.setTextColor(DARKGREEN);
      tft.println("WELCOME! ");
      delay(1500);
    }
  }
}

unsigned long character(){

  tft.fillScreen(DARKGREEN); // Black background on tft
  tft.setRotation(1); //to make text upright in landscape position
  tft.setFont(&FreeSerifBold12pt7b);
  tft.setTextColor(DARKGREEN);
  tft.setCursor(0, 0); //sets cursor at x,y position
}


unsigned long menu() {
  pos = 2;
  character();
  tft.fillScreen(DARKGREEN);
  tft.setTextColor(DARKGREEN);
  //tft.fillRoundRect(0,0,380,50,10,BLACK);
 // tft.drawRoundRect(0,0,380,50,10,WHITE);
  tft.fillRoundRect(10,10,300,220,10,YELLOW);
  tft.drawRoundRect(10,10,300,220,10,BLACK);
  tft.setCursor(85, 40);
  tft.println("Select Option:");
  for (uint16_t i=0; i<140; i++){
    tft.drawFastHLine(70+i, 50, 30, DARKGREEN);
  }
  tft.setCursor(30, 85);
  tft.println("1. Single Access");
  tft.setCursor(30, 125);
  tft.println("2. Multiple Access");
  tft.setCursor(30, 165);
  tft.println("3. Enroll/Delete");
  tft.setCursor(30, 205);
  tft.println("4. Back");
   
}


unsigned long enroll_success(int memberId) {
   character();
   tft.fillScreen(DARKGREEN);
   tft.fillRoundRect(10,70,300,80,5,YELLOW);
   tft.drawRoundRect(10,70,300,80,5,BLACK);
   tft.setCursor(50,110); //sets cursor at x,y position
   tft.setTextColor(DARKGREEN);
   tft.println("ENROLLED AS ");
   tft.setCursor(240,110); 
   tft.setTextColor(MAROON);
   tft.print(memberId);
}


void flushingAnimation(){
  tft.setCursor(80, 210);
  tft.setTextColor(MAROON); 
  tft.setTextSize(2); 
  tft.println("DELETING"); 
  int dotX1 = 190; 
  int dotY = 220; 
  for (int j = 0; j < 5; j++) { 
    for (int i = 0; i < 3; i++) {
      tft.fillCircle(dotX1 + i * 10, dotY, 4, MAROON);
      delay(200);
      tft.fillCircle(dotX1, dotY, 4, WHITE);
      tft.fillCircle(dotX1 + 10, dotY, 4, WHITE);
      tft.fillCircle(dotX1 + 20, dotY, 4, WHITE);
      delay(100);
    }
  }
}

void confirmFlush(){
  character();
  pos = 9;
  tft.fillScreen(DARKGREEN);
  tft.fillRoundRect(20,20,280,180,10,YELLOW );
  tft.drawRoundRect(20,20,280,180,10,BLACK);
  tft.setCursor(10,100); 
  tft.setTextColor(DARKGREEN);
  tft.setCursor(40, 50);
  tft.println(" CONFIRM DELETE? ");
  for (uint16_t i=0; i<200; i++){
    tft.drawFastHLine(55+i, 65, 10, DARKGREEN);
  }
  tft.setCursor(100, 100);
  tft.println("1. Yes");
  tft.setCursor(100, 150);
  tft.println("2. No");
}



void confirmDeleteCard(String codeval){
  Serial.println("confirm delete card called");
  character();
  cardToDelete = codeval;
  pos = 11;
  tft.fillScreen(DARKGREEN);
  tft.fillRoundRect(20,20,280,180,10,YELLOW );
  tft.drawRoundRect(20,20,280,180,10,BLACK);
  tft.setCursor(10,100); 
  tft.setTextColor(DARKGREEN);
  tft.setCursor(55, 50);
  tft.println("CONFIRM DELETE? ");
  for (uint16_t i=0; i<175; i++){
    tft.drawFastHLine(65+i, 68, 10, DARKGREEN);
  }
  tft.setTextColor(DARKGREEN);
  tft.setCursor(50, 100);
  tft.println(" CARD : ");
  tft.setTextColor(MAROON);
  tft.setCursor(145, 100);
  tft.println(codeval);
  for (uint16_t i=0; i<135; i++){
    tft.drawFastHLine(80+i, 110, 10, DARKGREEN);
  }
  tft.setTextColor(DARKGREEN);
  tft.setCursor(100, 140);
  tft.println("1. Yes");
  tft.setCursor(100, 170);
  tft.println("2. No");
}


void confirmDeleteCode(String code){
  pos = 14;
  Serial.println("confirm delete card called");
  character();
  cardToDelete = code;
  tft.fillScreen(DARKGREEN);
  tft.fillRoundRect(20,20,280,180,10,YELLOW );
  tft.drawRoundRect(20,20,280,180,10,BLACK);
  tft.setCursor(10,100); 
  tft.setTextColor(DARKGREEN);
  tft.setCursor(55,45);
  tft.println("CONFIRM DELETE? ");
  for (uint16_t i=0; i<200; i++){
    tft.drawFastHLine(60+i, 68, 10, DARKGREEN);
  }
  tft.setTextColor(DARKGREEN);
  tft.setCursor(50, 100);
  tft.println(" CODE : ");
  tft.setTextColor(MAROON);
  tft.setCursor(145, 100);
  tft.println(code);
  for (uint16_t i=0; i<30; i++){
    tft.drawFastHLine(145+i, 110, 10, DARKGREEN);
  }
  tft.setTextColor(DARKGREEN);
  tft.setCursor(100, 140);
  tft.println("1. Yes");
  tft.setCursor(100, 170);
  tft.println("2. No");

}



void delOption(){
  character();
  pos = 10;
  tft.fillScreen(DARKGREEN);
  tft.fillRoundRect(20,20,280,180,10,YELLOW );
  tft.drawRoundRect(20,20,280,180,10,BLACK);
  tft.setCursor(10,100); 
  tft.setTextColor(DARKGREEN);
  tft.setCursor(55, 50);
  tft.println(" DELETE OPTION ");
  for (uint16_t i=0; i<160; i++){
    tft.drawFastHLine(60+i, 70, 10, DARKGREEN);
  }
  tft.setCursor(60, 100);
  tft.println("1. By Card ");
  tft.setCursor(60, 150);
  tft.println("2. By Member No");
  codeToDelete = "";
  cardToDelete = "";
}



void delUID() {
  character();
  tft.fillScreen(DARKGREEN);
  tft.fillRoundRect(10, 70, 300, 80, 5, YELLOW);
  tft.drawRoundRect(10, 70, 300, 80, 5, BLACK);
  tft.setCursor(30, 110); 
  tft.setTextColor(DARKGREEN);
  tft.println("     Tap Card to Delete");

}

void DisplayDBempty(){
  character();
   tft.fillScreen(DARKGREEN);
   tft.fillRoundRect(10,70,300,80,5,YELLOW);
   tft.drawRoundRect(10,70,300,80,5,BLACK);
   tft.setCursor(80,100); 
   tft.setTextColor(MAROON);
   //tft.setTextSize(5);
   tft.println("DATABASE EMPTY");
}

/*
void delIndex() {
  String delTag = "";
  pos = 8;
  character();
  tft.fillScreen(DARKGREEN);
  tft.fillRoundRect(20,20,280,180,10,YELLOW );
  tft.drawRoundRect(20,20,280,180,10,BLACK);
  tft.setCursor(10,100); 
  tft.setTextColor(DARKGREEN);
  tft.setCursor(40, 50);
  tft.println(" CONFIRM DELETE? ");
  for (uint16_t i=0; i<200; i++){
    tft.drawFastHLine(55+i, 65, 10, DARKGREEN);
  }
  tft.setCursor(100, 100);
  tft.println("1. Yes");
  tft.setCursor(100, 150);
  tft.println("2. No");

  tft.fillScreen(DARKGREEN);
  tft.fillRoundRect(10, 50, 300, 150, 5, YELLOW);
  tft.drawRoundRect(10, 50, 300, 150, 5, BLACK);
  tft.setCursor(50, 80); 
  tft.setTextColor(DARKGREEN);
  tft.println("Enter Member Number"); 
  for (uint16_t i=0; i<220; i++){
    tft.drawFastHLine(50+i, 100, 10, DARKGREEN);
  }
  for (uint16_t i=0; i<55; i++){
    if (i%2 == 0){
    tft.drawFastHLine(120+i, 135, 25, MAROON);
    }
  }
  tft.setCursor(80, 145);
  tft.setTextColor(DARKGREEN);
  tft.println("  D = ENTER ");

}
*/

unsigned long detailsDeleted() {
  character();
  tft.fillScreen(DARKGREEN);
   tft.fillRoundRect(10,70,300,80,5,YELLOW);
   tft.drawRoundRect(10,70,300,80,5,BLACK);
   tft.setCursor(50,100); 
   tft.setTextColor(DARKGREEN);
   tft.println("DELETED SUCCESSFULLY");
   delay(1500);
   //enrollNew();
}

//detailnotfound
unsigned long detailsNotFound() {
   character();
   tft.fillScreen(DARKGREEN);
   tft.fillRoundRect(10,70,300,80,5,YELLOW);
   tft.drawRoundRect(10,70,300,80,5,BLACK);
   tft.setCursor(70,100); 
   tft.setTextColor(MAROON);
   tft.println("Card Not Found");
}


unsigned long flush_success() {
   character();
   tft.fillScreen(DARKGREEN);
   tft.fillRoundRect(10,70,300,80,5,YELLOW);
   tft.drawRoundRect(10,70,300,80,5,BLACK);
   tft.setCursor(60,110); 
   tft.setTextColor(DARKGREEN);
   //tft.setTextSize(5);
   tft.println("DELETE SUCCESS");
   delay (2000);
}

unsigned long flush_fail() {
   character();
   tft.fillScreen(DARKGREEN);
   tft.fillRoundRect(10,70,300,80,5,YELLOW);
   tft.drawRoundRect(10,70,300,80,5,BLACK);
   tft.setCursor(60,110); 
   tft.setTextColor(MAROON);
   //tft.setTextSize(5);
   tft.println("DELETE FAILED");
   delay (2000);
}

void enrollNew(){
  character();
  pos = 6;
  tft.fillScreen(DARKGREEN);
  tft.setTextColor(DARKGREEN);
  //tft.fillRoundRect(0,0,380,50,10,BLACK);
 // tft.drawRoundRect(0,0,380,50,10,WHITE);
  tft.fillRoundRect(10,10,300,220,10,YELLOW);
  tft.drawRoundRect(10,10,300,220,10,BLACK);
  tft.setCursor(85, 40);
  tft.println("Select Option:");
  for (uint16_t i=0; i<140; i++){
    tft.drawFastHLine(70+i, 50, 30, DARKGREEN);
  }

  tft.setCursor(30, 85);
  tft.println("1. Enroll Member");
  tft.setCursor(30, 125);
  tft.println("2. Delete All Members");
  tft.setCursor(30, 165);
  tft.println("3. Delete Member");
  tft.setCursor(30, 205);
  tft.println("4. Back");
  tft.setCursor(30, 50);
  ind_UID = "";
  ind_UID1 = "";
}

unsigned long stopMultiple(){
 pos = 4;
   character();
   tft.fillScreen(DARKGREEN);
   tft.fillRoundRect(5,70,310,80,5,YELLOW );
   tft.drawRoundRect(5,70,310,80,5,BLACK);
   tft.setCursor(20,115); 
   tft.setTextColor(DARKGREEN);
    //tft.setTextSize(3);
   tft.println("     PRESS 1 TO STOP  ");
}





unsigned long MasterError() {
  character();
   tft.fillScreen(DARKGREEN);
   tft.fillRoundRect(10,70,300,80,5,YELLOW);
   tft.drawRoundRect(10,70,300,80,5,BLACK);
   tft.setCursor(50,120); //sets cursor at x,y position
   tft.setTextColor(MAROON);
   tft.println("WRONG PASSWORD");//displays the string
}


void denied_sequence()
{
  MasterError();
  //denied();
  digitalWrite(red,HIGH);
  for(int i=0; i<=5; i++){
   //digitalWrite(buzzer,HIGH);
   delay(100);
   //digitalWrite(buzzer,LOW);
   delay(100);
    }
  digitalWrite(red,LOW);
}




void invalidChoice(){
  enrolling = false ;
  character();
  tft.fillScreen(DARKGREEN);
  tft.fillRoundRect(10,70,300,80,5,YELLOW);
  tft.drawRoundRect(10,70,300,80,5,BLACK);
  tft.setCursor(70,115);  
  tft.setTextColor(MAROON);
  tft.println("INVALID CHOICE!");
  delay(2000);
       
}



//Master tag Menu
unsigned long masterTagMenu(){
  // useWindow = true;
  character();
  pos = 20;
  tft.setCursor(90,80);
  tft.setTextColor(DARKGREEN); 
  tft.fillRoundRect(40,30,240,180,10,YELLOW);
  tft.drawRoundRect(40,30,240,180,10,BLACK);
  tft.setFont(&FreeSerifBold12pt7b);

  tft.println("Enter the pin");
  for (uint16_t i=0; i<150; i++){
    tft.drawFastHLine(80+i,135,10,DARKGREEN);
  }
  tft.setCursor(105,170);
  tft.setTextColor(DARKGREEN);
  tft.println("B = BACK");
  animate = false;
}


unsigned long delIndexWindow(){
  character();
  pos = 13;
  IndexToDelete = "";
  tft.fillScreen(DARKGREEN);
  tft.setCursor(65,80);
  tft.setTextColor(DARKGREEN); 
  tft.fillRoundRect(40,30,240,180,10,YELLOW);
  tft.drawRoundRect(40,30,240,180,10,BLACK);
  tft.setFont(&FreeSerifBold12pt7b);
  tft.println("Key-in ID to delete");
  for (uint16_t i=0; i<150; i++){
    tft.drawFastHLine(80+i,135,10,DARKGREEN);
  }

  tft.setCursor(105,170);
  tft.setTextColor(DARKGREEN);
  tft.println("D = ENTER");
}


unsigned long welcome(){
  pos = 0;
   character();
   tft.fillScreen(DARKGREEN);
   tft.fillRoundRect(80,90,140,40,10,YELLOW); //draws a filled Rectangle at x,y,width,height,radius,colour
   //tft.drawRoundRect(93,128,303,53,10,DARKCYAN);//draws a rounded Rectangle at x,y,width,height,radius,colour
   tft.setCursor(100,100); //sets cursor at x,y position
   //tft.setTextSize(5);//text size 1-5
   tft.setTextColor(DARKGREEN);
   tft.println("LOADING ");
   c_color = YELLOW;
   displayLoadingAnimation();
}




void enroll(){
 // DBempty = false;
  enrolling = true;
  character();
  int feed;
  String tagname = "";
  tft.fillScreen(DARKGREEN);  
  tft.fillRoundRect(5,70,310,80,5,YELLOW);
  tft.drawRoundRect(5,70,310,80,5,BLACK);
  tft.setCursor(10,110); //sets cursor at x,y position
  tft.setTextColor(DARKGREEN);
  tft.println("     Tap Card ID to enroll");//displays the string
  tagname = receivedTag;
  Serial.println("Found a tag to enroll: ");
  Serial.println(tagname);
  // enrolling = true;
  // if(tagname.length() != 0){
  //   feed = storeUid(tagname);
  // }
  // if (feed != -1){
  //   enroll_success(feed);
  //   //printStoredEntries();
  //   delay(1500);
  // }
  // else{
  //   Serial.println("Error with storing UID in EEPROM");
}









//-----------------------------------------------------------------MAIN FUNCTIONS--------------


void GetFromEsp() {
  dataReadyFlag = true;
  animate = false;
}


void handleSerialData() {
  // tft.fillScreen(DARKGREEN);
  dataReadyFlag = false;
  receivedTagDel = "";
  // dataReadyFlag = false;
  while (Serial1.available()) {
    String data = Serial1.readStringUntil('\n');
    data.trim();

    if (data.length() > 0) {
      dataBuffer.push(data);
      Serial.println("Data added to buffer: " + data);
    }
  }

  // Process data in the buffer
  while (!dataBuffer.isEmpty()) {
    String clockDetails = dataBuffer.shift();
    Serial.println("Processing data from buffer: " + clockDetails);

    DynamicJsonDocument doc(256);  
    DeserializationError error = deserializeJson(doc, clockDetails);
    // dataReadyFlag = false;
    if (error) {
      Serial.print("JSON parsing failed: ");
      Serial.println(error.c_str());

    }else if(doc.containsKey("c")){   
      const char* clockID = doc["c"];
      String clID = UpperCase(String(clockID));

      if(!doc.containsKey("t")){
        currTime = "12:43";
      }else{
        const char* clockTime =  doc["t"];
        currTime = String(clockTime);
      }

        
      if (clID == masterTag || clID == masterTag2) {
        keyIn = true;
        masterCalled = true;
        masterTagMenu();
      }else{                                        //  receive card ID and tiùe

        Serial.println("card ID : " + clID);
        Serial.println("time : " + currTime);


        if(enrolling) {
          Serial.println("enrolling = true /!");
          // We're in enrollment mode
          if (checkMember(clID)) {
            Serial.println("Member is already enrolled");
            alreadyEnrolled();
            delay(500);
            enrolling = false;
            enrollNew();
          }else{
            addMember(clID);
            enrolling = false;
            enrollNew();
            }      
          }
        else if(deleting){
          Serial.println("deleting=true");
          deleting = false;
          receivedTagDel = clID;
          confirmDeleteCard(receivedTagDel);
        }       
        else {
        Serial.println(" normal operation ");
        grantAcces(clID,currTime);
        }   

    }        
    }
  }
      // animate = false;
      // dataReadyFlag = false; 
      masterCalled = false;
}  

  
 
   
void grantAcces(String IDCard,String CardTime){
  // dataReadyFlag = false;
  animate = true ; 
  if (checkMember(IDCard)) {
    Serial.println("Member is enrolled");
    open_sequence();  
    delay(300);      
  } else {
    Serial.println("Member is NOT enrolled");
    ClockDenied();
    digitalWrite(red,HIGH);
    delay(300);
    digitalWrite(red,LOW);

  }
    clockSequence(IDCard,CardTime);
    startWindow();
}





void clockSequence(String clockID,String clockTime){
  if (clockID && clockTime) {
        String clk = String(clockTime);
        Serial.println("Received from ESP8266: " + clockID + " at " + String(clockTime) + "  Saving on card...");
        receivedTag = clockID;

          fillMember(clockID, clockTime);

          if (clk.length() > 4){
            int colonIndex1 = clk.indexOf(':');
            int colonIndex2 = clk.indexOf(':', colonIndex1 + 1);
            currHr = clk.substring(0, colonIndex1).toInt() ;
            currMin = clk.substring(colonIndex1 + 1, colonIndex2).toInt() ;
            currSec = clk.substring(colonIndex2 + 1).toInt() ;
            // passedTime = clockTime;
          }
        //addMember(clID);

      } else {
        Serial.println("Missing required fields in JSON data");
      }
      // startWindow();
}

unsigned long ClockDenied() {
  character();
   tft.setFont(&FreeSerifBold18pt7b);
   tft.fillScreen(DARKGREEN);
   tft.fillRoundRect(10,70,300,80,5,YELLOW);
   tft.drawRoundRect(10,70,300,80,5,BLACK);
   tft.setCursor(60,120); //sets cursor at x,y position
   tft.setTextColor(MAROON);
   tft.println("Clock Denied");//displays the string
}

unsigned long alreadyEnrolled() {
   character();
   tft.fillScreen(DARKGREEN);
   tft.fillRoundRect(10,70,300,80,5,YELLOW);
   tft.drawRoundRect(10,70,300,80,5,BLACK);
   tft.setCursor(60,120); //sets cursor at x,y position
   tft.setTextColor(MAROON);
   tft.println("Already enrolled !");//displays the string
}


unsigned long ClockAccess() {
  character();
   tft.setFont(&FreeSerifBold18pt7b);
   tft.fillScreen(DARKGREEN);
   tft.fillRoundRect(10,70,300,80,5,YELLOW);
   tft.drawRoundRect(10,70,300,80,5,BLACK);
   tft.setCursor(39,120); //sets cursor at x,y position
   tft.setTextColor(DARKGREEN);
   tft.println("Access Granted");//displays the string
}


void sortTime(int elapsedTime) {

    // t2 = millis();
    passedTime = abs(elapsedTime)%day + rest;

    int Hr = passedTime / 3600000;
    int Min = (passedTime % 3600000) / 60000;
    int Sec = (passedTime % 60000) / 1000;
    rest = passedTime % 1000;

      Serial.print("Hr : ");
      Serial.println(Hr);
      Serial.print("Min : ");
      Serial.println(Min);
      Serial.print("Sec : ");
      Serial.println(Sec);

      currHr += Hr;
      currMin += Min;
      currSec += Sec;
   
    if (currSec >= 60) {
        currMin += currSec / 60;
        currSec = currSec % 60;
    }
    if (currMin >= 60) {
        currHr += currMin / 60;
        currMin = currMin % 60;
    }
    if (currHr >= 24) {
        currHr = currHr % 24;
    }
    if(String(currHr).length() == 1){
      curHr = "0"+String(currHr);
    }else{curHr=String(currHr);}
    if(String(currMin).length() == 1){
      curMin = "0"+String(currMin);
    }else{curMin=String(currMin);}


    currT = String(curHr) + ":" + String(curMin) ;
    Serial.println(currT);
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
  }
    memberFile.close();
}

void addMember(String ID) {
  File memberFile = SD.open("member2.csv", FILE_WRITE);
  if (memberFile) {
    if (ID.length() > 0) {
      String rowData = ID + ";\n";
      memberFile.print(rowData);
      memberFile.close();
      indexMember(ID);
    }
  } else {
    Serial.println("Error opening member2.csv");
  }
  enrolling = false;  // Reset enrolling flag after adding a member
  enroll_success(countRows());
  delay(1500);
  // enrollNew();
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
  int num_rows = countRows();
  if (idxFile) {
    if (ID.length() > 0) {
      String rowData = ID + ";" + num_rows + "\n";
      idxFile.print(rowData);
    }
    idxFile.close();
  } else {
    Serial.println("Error opening index.csv");
  }
  // enroll_success(num_rows);
  delay(1500);
  // enrollNew();
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
  Serial.println("DelIndex called  ");
  bool deleteSuccess = delInBigString(Ind);
  if(deleteSuccess){
    flush_success(); 
    delay(400);
    delOption();
  }else{
    flush_fail();
    delay(400);
    delOption();
  }
}

bool checkMember(String checkID){                                         //grant member access
  String ReadDetails = "";
  File myFile = SD.open("member2.csv");
  while (myFile.available()) {
    ReadDetails = myFile.readStringUntil("\n");
    if (findStringInBigString(checkID,ReadDetails)) {
      myFile.close();
      return true;
    }
    if(ReadDetails.length()<=2){
      myFile.close();
      return false;
    }
  }
}
/*
void reading(String ScannedCard) {                                   //print all members enrolled
  String ReadDetails = "";
  File myFile = SD.open("member2.csv");
  while (myFile.available()) {
    ReadDetails = myFile.readStringUntil('\n');
    Serial.println(ReadDetails);
  }
  myFile.close();
}
*/


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
    String subString = bigString.substring(startIndex, endIndex-1);
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


void deleteCSV(){
  if (!SD.begin(53)) {
    Serial.println("SD initialization failed!");
    return;
  }
  Serial.println("Initialization done.");

    if (SD.exists("member2.csv")) {
      SD.remove("member2.csv");
    File enrollMemberFile = SD.open("member2.csv", FILE_WRITE);
    if (enrollMemberFile) {
      //enrollMemberFile.println("E9A6F39\nFACFC768\nE9A6F39\nFACFC768");
      Serial.println("SD_OK");
    } else {
      Serial.println("Error creating enrollMember.csv");
    }
    enrollMemberFile.close();
  }
  if (SD.exists("index.csv")) {
    SD.remove("index.csv");
      File indexFile = SD.open("index.csv", FILE_WRITE);
      if (indexFile) {
        indexFile.println("CARD_UID;INDEX");
        Serial.println("SD_OK");
      } else {
        Serial.println("Error creating index.csv");
        }
        indexFile.close();
      }
      flush_success();
      delay(400);
}


bool delIndexInCSV(String Index){
  Serial.println("delIndex called  ");
  File originalFile = SD.open("index.csv");
  if (!originalFile) {
    Serial.println("Error opening index.csv");
    return false;
  }else{
        Serial.println("original file open");
       }   
  
  // File tempFile = SD.open("temp.csv", FILE_WRITE);
  // if (!tempFile) {
  //   Serial.println("Error creating temp.csv");
  //   originalFile.close();
  //   return false;
  // }else{
  //       Serial.println("Temp file open");
  //      }   

  int startIndex1 = 0;
  int endIndex1 = 8 ;

  while (originalFile.available()) {
    String line = originalFile.readStringUntil('\n');
    Serial.println("line = "+ line);
  //to remove the ";" of the string "line"

  endIndex1 = line.indexOf(';', startIndex1);

  Serial.println("endIndex1" + endIndex1);
  
    String indexFromLine = line.substring(endIndex1+1);
    String IDfromLine =  line.substring(startIndex1,endIndex1);
    indexFromLine.trim();
    IDfromLine.trim();
    
    Serial.println(indexFromLine + "and" + Index);
    Serial.println(IDfromLine + " at " + Index);
    if(indexFromLine == Index){
      return delInBigString(IDfromLine);
    }
    else{
      Serial.println(indexFromLine + " not " + Index);
    }
    
    
  //   if (indexFromLine.equals(Index)) {
  //   Serial.println("Deleting: " + line);   
  //   }else{
  //     tempFile.println(line);
  //     Serial.println("copy succes on temp file");      
  //   }
  // }
  // originalFile.close();
  // tempFile.close();
  // SD.remove("index.csv");
  // tempFile = SD.open("temp.csv");
  // originalFile = SD.open("index.csv", FILE_WRITE);

  // if (tempFile && originalFile) {
  //   while (tempFile.available()) {
  //     originalFile.write(tempFile.read());
  //   }
  //   tempFile.close();
  //   originalFile.close();
  //   SD.remove("temp.csv");
  //   return true;

  // } else {
  //   Serial.println("Error in file copy process");
  //   return false;
  // }
}
}


bool delInBigString(String searchString) {
  Serial.println("delInBigString called  ");
  File originalFile = SD.open("member2.csv");
  if (!originalFile) {
    Serial.println("Error opening member2.csv");
    return false;
  }else{
        Serial.println("original file open");
       }   
  
  File tempFile = SD.open("temp.csv", FILE_WRITE);
  if (!tempFile) {
    Serial.println("Error creating temp.csv");
    originalFile.close();
    return false;
  }else{
        Serial.println("Temp file open");
       }   

  int startIndex2 = 0;
  int endIndex2 = 8;

  while (originalFile.available()) {
    String line = originalFile.readStringUntil('\n');
    Serial.println("line is" + line);

  //to remove the ";" of the string "line"
  endIndex2 = line.indexOf(';', startIndex2);

  Serial.println("endIndex" + endIndex2);
    line = line.substring(startIndex2, endIndex2-1);
    line.trim();
    Serial.println(line + "and" + searchString);
   
    if (line.equals(searchString)) {
    Serial.println("Deleting: " + line);   
    }else{
      tempFile.println(line);
      Serial.println("copy succes on temp file");      
    }
  }

  //   endIndex = line.indexOf(';', startIndex);
  //   Serial.println("endindex : "+ endIndex);

  //   line = line.substring(startIndex, endIndex);

  //   line.trim();
  
  // Serial.println(line + " and " + searchString);

  //   if (line != searchString) {
  //     tempFile.println(line);
  //     Serial.println("copy succes on temp file");
  //   } else {
  //     Serial.println("Deleting: " + line);
  //   }
  // }
  originalFile.close();
  tempFile.close();
  SD.remove("member2.csv");
  tempFile = SD.open("temp.csv");
  originalFile = SD.open("member2.csv", FILE_WRITE);

  if (tempFile && originalFile) {
    while (tempFile.available()) {
      originalFile.write(tempFile.read());
    }
    tempFile.close();
    originalFile.close();
    SD.remove("temp.csv");
    return true;

  } else {
    Serial.println("Error in file copy process");
    return false;
  }
}

//-----------------------------------------------------------------------------------------------------




String enrolltag(bool isEnrollMode = true) {
  Serial.println("Scanning Tag...");
  /*
  while (true) {
    if (!mfrc522.PICC_IsNewCardPresent()) {
      continue;
    }
    if (!mfrc522.PICC_ReadCardSerial()) {
      continue;
    }
    
    digitalWrite(blue, HIGH); //Blue RGB LED lights to show that card has been scanned
    scannedTag = "";
    for (int i = 0; i < mfrc522.uid.size; i++) {
      scannedTag += String(mfrc522.uid.uidByte[i], HEX); // Adds the 4 bytes in a single String variable
      scannedTag.toUpperCase();
    }
    
    if (scannedTag.length() == 13) {
      scannedTag = "0" + scannedTag;
    }
    digitalWrite(blue, LOW);
    Serial.print("Scanned a tag : ");
    Serial.println(scannedTag);

    if (isEnrollMode) {
      return scannedTag;
    } else {
      ind_UID1 = scannedTag;
      scanEEPROM(ind_UID1);
      ind_UID1 = "";
      //delUID;
    }
  }
  */
}



// void access_denial_sequence()
// {
//   //ClockDenied();

//   digitalWrite(red,HIGH);
//   for(int i=0; i<=5; i++){
//    digitalWrite(buzzer,HIGH);
//    delay(100);
//    digitalWrite(buzzer,LOW);
//    delay(100);
//     }
//   digitalWrite(red,LOW);
// }



/*
String enrolltag(bool isEnrollMode = true) {
  Serial.println("Scanning Tag...");
  
  while (true) {
    if (!mfrc522.PICC_IsNewCardPresent()) {
      continue;
    }
    if (!mfrc522.PICC_ReadCardSerial()) {
      continue;
    }
    
    digitalWrite(blue, HIGH); //Blue RGB LED lights to show that card has been scanned
    scannedTag = "";
    for (int i = 0; i < mfrc522.uid.size; i++) {
      scannedTag += String(mfrc522.uid.uidByte[i], HEX); // Adds the 4 bytes in a single String variable
      scannedTag.toUpperCase();
    }
    
    if (scannedTag.length() == 13) {
      scannedTag = "0" + scannedTag;
    }
    digitalWrite(blue, LOW);
    Serial.print("Scanned a tag : ");
    Serial.println(scannedTag);

    if (isEnrollMode) {
      return scannedTag;
    } else {
      ind_UID1 = scannedTag;
      scanEEPROM(ind_UID1);
      ind_UID1 = "";
      //delUID;
    }
  }
}
*/

void open_sequence()
{
  digitalWrite(green,HIGH);
//  digitalWrite(buzzer,HIGH);
    ClockAccess();
  digitalWrite(solenoidlock,LOW);
//  digitalWrite(buzzer,LOW);

  int solenoid = digitalRead(solenoidlock); 
  Serial.println(" solenoidlockval : "+ solenoid);
  delay(500);
  digitalWrite(green,LOW);
  digitalWrite(solenoidlock,HIGH);
  //digitalWrite(solenoidlock,HIGH);
  // while(solenoid == 0)
 // {
    //int contact = digitalRead(contactsensor);
    //if(contact == 0)
    //{
    //digitalWrite(solenoidlock,HIGH);

   // solenoid = 1;
    //}
    //else {
    // digitalWrite(solenoidlock,LOW);
   // }
  //}

}



void Keypadvalue()
{
  customKey = customKeypad.getKey();
  if (customKey)
  {
    if(pos == 0)
    {
      Serial.println(customKey);
      if(customKey == 'A'){
       digitalWrite(blue,HIGH);
       delay(500); 
       digitalWrite(blue,LOW);
       masterTagMenu();   
      }
      loop();
    }
    else if(pos == 20)
    {
      if (customKey == 'B'){
        customKey = ' ';
        cursorpos = 20;
        masterPin = "";
        masterPin1 = "";
        //  useMaster = false;
        startWindow();
        
      }
      else if ((masterPin1.length() >= 0) && (masterPin1.length() <= 4))
      {
        cursorpos += 40;
        masterPin = String(customKey);
        masterPin1 +=  masterPin;
        tft.setCursor(40+cursorpos,120);
        tft.setTextColor(DARKGREEN); 
        tft.print("*");
        //Serial.println(masterPin1.length());
//        Serial.println(masterPin1);
        //Serial.print(customKey);
        Serial.println(masterPin1);
      if (masterPin1.length() == 4)
      {
        if (masterPin1 == keypadPin)
        {
          //access ();
          //delay (800);
          cursorpos = 20;
          masterPin = "";
          masterPin1 =  "";   
          menu();
        }
        else
        {
          cursorpos = 20;
          masterPin = "";
          masterPin1 =  "";
          //masterTagMenu();
          denied_sequence(); //for the master error
          delay(100);
          startWindow();
          return;
        }
        //  useMaster = false;
      }
      }
      // else if (masterPin1.length() > 3 )
      // {
      //   masterPin1 = "";
      // }
    }
    else if (pos == 2) {
      choice = (customKey);
      //Serial.println(choice);
      if(choice == '1'){
          open_sequence();        
          delay(400);
          startWindow();
        }
        else if(choice == '2'){
          digitalWrite(green,HIGH);
       //   digitalWrite(buzzer,HIGH);
          delay(1200);
          digitalWrite(solenoidlock,LOW);
       //   digitalWrite(buzzer,LOW);
          stopMultiple();
        }
        else if(choice == '3'){

          enrollNew();

        }
        else if(choice == '4'){
          startWindow();
        }
        else if (choice == '5'){
          enrollMaster();
        }
        else{
          invalidChoice();
          menu();
        }
     }
    else if(pos == 3){
      if (masterPin1.length() < 4)
      {
        cursorpos += 40;
   //       Serial.println(masterPin1.length());
        masterPin = String(customKey);
        masterPin1 +=  masterPin;
  //        Serial.println(masterPin1);
  //        Serial.println(customKey);
        tft.setCursor(40+cursorpos,120);
        tft.print("*");
        }
        if (masterPin1.length() == 4 )
        {
          //confirmPassword();
          cursorpos = 20;
         }
      if (masterPin1.length() > 3 )
      {
        masterPin1 = "";
      }
   }  
       else if(pos == 4){
          choice = (customKey);
          if(choice == '1')
          {
          digitalWrite(green,LOW);
          digitalWrite(solenoidlock,HIGH);
          startWindow();
          }
        }
  //       else if(pos == 5){
  //         if (masterPin1.length() < 4)
  //         {
  //         cursorpos += 40;
  //  //       Serial.println(masterPin1.length());
  //         masterPin = String(customKey);
  //         masterPin1 +=  masterPin;
  // //        Serial.println(masterPin1);
  // //        Serial.println(customKey);
  //         tft.setCursor(40+cursorpos,120);
  //         tft.print("*");
  //         }
  //       if (masterPin1.length() == 4 )
  //       {
  //         if ( keypadPin == masterPin1)
  //          {          
  //             //passwordReset();
  //             delay(3000);
  //             startWindow();
  //          }
  //         else
  //          {
  //           //notMatch();
  //           delay(3000);
  //           startWindow();
  //           }
  //         cursorpos = 20;
  //        }
             
  //     if (masterPin1.length() > 3 )
  //     {
  //       masterPin1 = "";
  //     }
  //  }
   else if (pos == 6) {
      choice = (customKey);
      if(choice == '1'){
          enroll();
        }
        else if(choice == '2'){
          confirmFlush();
          delay(1500);
        }
        else if(choice == '3'){
          delOption();
        }
        else if(choice == '4'){
          menu();
        }
        else
        {
          invalidChoice(); 
          enrollNew();
        }
     }

     else if (pos == 10) {
      choice = (customKey);
      if(choice == '1'){
        Serial.println("pos10 choice 1");
          deleting = true;
          delUID();
        }
        else if(choice == '2'){
          delIndexWindow();
        }
        else
        {
          invalidChoice(); 
          enrollNew();
        }
     }
     else if (pos == 7) {
     // Serial.println("Keypad started");
      choice = (customKey);
      Serial.println(choice);
      if(choice == '1'){
          action = "IN";
          //bool proceed = true;
          //verify();
        }
        else if(choice == '2'){
          action = "OUT";
          //bool proceed = true;
          //verify();
        }
        else if(choice == '3'){
          //bool proceed = true;
          startWindow();
        }
        else
        {
          invalidChoice();
          bool proceed = true;
          startWindow();
          
          
        }
     }
    
    else if (pos == 8) {
      char quit = 'd';
      customKey = char(toLowerCase(customKey));
      Serial.println(customKey);
      
      if (customKey == quit) {
        //confirmDeleteCode();
        Serial.println("--Done--");
        if (ind_UID1.length() > 0) {
          confirmDeleteCode(ind_UID1);
          
        } else {
          enrollNew(); 
        }
      }
      else if (customKey != quit) {
        ind_UID = String(customKey);
        ind_UID1 += ind_UID;
        tft.setCursor(130, 120); //sets cursor at x,y position
        tft.setTextColor(MAROON);
        tft.print(ind_UID1);
        if (ind_UID1.length() > 3) {
 //         scanIndex(ind_UID1);
        } 
      } 
      
    }
    else if(pos == 11){
      if(customKey == '1'){
        Serial.println("pos = 11 key = 1");
        DelIndex(receivedTagDel);
        //confirmDeleteCode(receivedTagDel);
      }
      else if (customKey == '2'){
        delOption();
      }
      else{
        invalidChoice();
      }
        
    }
    //scanIndex(ind_UID1);
    else if(pos == 12){
      if(customKey == '1'){
 //       scanIndex(codeToDelete);
      }
      else if (customKey == '2'){
        delOption();
      }
      else{
        invalidChoice();
      }
        
    }

     else if (pos == 9) {
      choice = (customKey);
      if(choice == '1'){
        deleteMember = true;
        deleteCSV();
        enrollNew();
        }
      else if(choice == '2'){
        enrollNew();
      }
      else
      {
        enrollNew();
      }
    }

    else if(pos == 13)
    {
      if (customKey == 'D' || IndexToDelete.length() >= 6){
        confirmDeleteCode(IndexToDelete);
      }
      else
      {
        IndexToDelete +=  String(customKey);
        if(IndexToDelete.length() == 1){
          cursorposInd = 155;
        }else if(IndexToDelete.length() == 2){
            cursorposInd = 150;
        }else if(IndexToDelete.length() == 3){
            cursorposInd = 145;
        }else if(IndexToDelete.length() == 4){
            cursorposInd = 140;
        }else if(IndexToDelete.length() == 5){
            cursorposInd = 135;
        }
        tft.setCursor(cursorposInd+5,120);
        tft.setTextColor(YELLOW);
        tft.print(IndexToDelete);
        tft.setCursor(cursorposInd,120);
        tft.setTextColor(MAROON);
        tft.print(IndexToDelete);
        //Serial.println(masterPin1.length());
//        Serial.println(masterPin1);
        //Serial.print(customKey);
        Serial.println(masterPin1);
      
     
      }
      }
      else if(pos == 14){

      if(customKey == '1'){
        if(delIndexInCSV(IndexToDelete))
        {
        flush_success();
        ;
        }
        else{
          flush_fail();
        }
        delay(100);
        delOption();
      }
      else if(customKey == '2'){
        delOption();
      }
      else{
      ;
      }
    }
   
    else
    {
      return;
    }
  }
}

void enrollMaster(){

  character();
  tft.fillScreen(DARKGREEN);
  tft.fillRoundRect(10, 70, 300, 80, 5, YELLOW);
  tft.drawRoundRect(10, 70, 300, 80, 5, BLACK);
  tft.setCursor(30, 110); 
  tft.setTextColor(DARKGREEN);
  tft.println("Tap card enroll as Master");

}


