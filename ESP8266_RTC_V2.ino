#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>


#define SS_PIN  D8 
#define RST_PIN D2 

#include <TimeLib.h>
int rest ;
bool noTime = true ; 
String UpdatedTime = "00:00";
bool tapSequence = false;
String tappedCard;
unsigned long timeOffset = 0;
String lastReturnedDate = "";
int lastReturnedHour = -1;
bool tagMaster = false ; 
String masterTag = "39e1d456";
String masterTag2 = "59d7a218";
String cardUID;
MFRC522 rfid(SS_PIN, RST_PIN);
bool dataSent = false;
bool internetWaiting = false;
bool internetPresent = true;
const byte interruptPin = 5;//for pin D1 ESP
const char* ssid = "CDED";
const char* password = "CDED2024.";
const char* TimeAPI = "http://worldtimeapi.org/api/timezone/Africa/Nairobi";
String utcTime = "00:00";
int t1 = 0;
int t2 = 0;
int t3 = 0;
int dayy = 86400000;//number of millisecond in one day
int currHour;
int currMinute;
int currSecond;
String currMinute1 = "00";
String currHour1 = "00";
String currT = "00:00";


int passedTime;
int Hr;
int Min;
int Sec;



void setup() {
  Serial.begin(9600);
  WiFi.disconnect(); 
  WiFi.begin(ssid, password);
  pinMode(interruptPin,OUTPUT);
  digitalWrite(interruptPin,LOW);
  Serial.println("Connecting to WiFi");
  internetPresent = ConnectWIFI(10);
  if(internetPresent == false){
    Serial.println("\n not Connected to WiFi network");
    currT = "12:33";
    noTime = true;
  }else{
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP()); 
    checkTime();
  }
  
  SPI.begin(); 
  rfid.PCD_Init();
  Serial1.begin(115200);
}


void loop() {

  t1 = millis();


  if(checkTapping()){
    sendToMEGA(tappedCard);
    tappedCard = "";
  }

  
  t2 = millis() + (t3-t2);

  sortTime((t2-t1));

  t3 = millis();

}




void sortTime(int elapsedTime) {

Serial.println("elapsedTime : "+ elapsedTime);

      if(noTime){checkTime();}

      passedTime = abs(elapsedTime)%dayy + rest;

      Serial.print("elapsedTime : ");
      Serial.println(elapsedTime);
      Serial.print("passedTime : ");
      Serial.println(passedTime);

      Hr = passedTime / 3600000;
      Min = (passedTime % 3600000) / 60000;
      Sec = (passedTime % 60000) / 1000;
      rest = passedTime % 1000;

      Serial.print("Hr : ");
      Serial.println(Hr);
      Serial.print("Min : ");
      Serial.println(Min);
      Serial.print("Sec : ");
      Serial.println(Sec);
      Serial.print("rest : ");
      Serial.println(rest);

      // if (Sec < 1){rest += elapsedTime;}



        currHour += Hr;
        currMinute += Min;
        currSecond += Sec;
    
      if (currSecond >= 60) {
          currMinute += currSecond / 60;
          currSecond = currSecond % 60;
      }
      if (currMinute >= 60) {
          currHour += currMinute / 60;
          currMinute = currMinute % 60;
          checkTime();
      }
      if (currHour >= 24) {
          currHour = currHour % 24;
      }

      Serial.print("currHour : ");
      Serial.println(currHour);
      Serial.print("currMinute : ");
      Serial.println(currMinute);
      Serial.print("currSecond : ");
      Serial.println(currSecond);

      if(String(currHour).length() == 1){
        currHour1 = "0"+String(currHour);
      }else{currHour1=String(currHour);}
     
      if(String(currMinute).length() == 1){
        currMinute1 = "0"+String(currMinute);
      }else{currMinute1=String(currMinute);}


      Serial.print("currHour1 : ");
      Serial.println(currHour1);
      Serial.print("currMinute1 : ");
      Serial.println(currMinute1);

      
      currT = currHour1 + ":" + currMinute1 ;
      Serial.println("currT : " + currT);



}

bool checkTapping(){
  tappedCard = readTag();
  if(tappedCard.length() > 0){
    return true;
  }
  return false;
}

bool ConnectWIFI(int buffer){
  for(int k = 0; k < buffer; k++){
    if(WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(200);
    }
    else{
      return true;
    }
  }
  return false; 

}
String readTag(){
  //dataSent = false;
//  internetPresent = ConnectWIFI(1);
  if (rfid.PICC_IsNewCardPresent()) {
    if (rfid.PICC_ReadCardSerial()) {
      cardUID = ""; 
      for (int i = 0; i < rfid.uid.size; i++) {
        //Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
        cardUID +=  String(rfid.uid.uidByte[i], HEX);
      }
      cardUID.remove(cardUID.indexOf(' '));
      //Serial.print(cardUID);
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      return cardUID;
    }
  }
  return "";  
}


bool sendToMEGA(String cardNo) {
  if (cardNo != "") {
    if (cardNo == masterTag || cardNo == masterTag2 ){tagMaster = true;}
    DynamicJsonDocument doc(128); 
    if(!tagMaster){
      doc["c"] = cardNo;
      doc["t"] = currT;
    }else{
      doc["c"] = cardNo;
      doc["t"] = currT;
    }

    String jsonData;
    serializeJson(doc, jsonData);
    Serial.print(jsonData);
    digitalWrite(interruptPin,HIGH);
    Serial1.print(jsonData);
    Serial.print(jsonData);
    delay(100);
    digitalWrite(interruptPin,LOW); 
    return true;
  }
  return false;
}


// void setupTime(int hr, int min, int sec, int day, int month, int yr) {
//   setTime(hr, min, sec, day, month, yr);
//   timeOffset = now() - millis() / 1000;
// }

// String getDateAtSetTimes() {
//   unsigned long currentTime = millis() / 1000 + timeOffset;
//   setTime(currentTime);
//   int currentHour = hour();
//   int currentMinute = minute();
//   if (currentMinute == 0 && (currentHour == 0 || currentHour == 6 || currentHour == 18)) {
//     if (currentHour != lastReturnedHour) {
//       char dateStr[11];
//       sprintf(dateStr, "%04d-%02d-%02d", year(), month(), day());
//       String currentDate = String(dateStr);
      
//       lastReturnedHour = currentHour;
//       lastReturnedDate = currentDate;
      
//       String timeOfDay;
//       switch(currentHour) {
//         case 0:  timeOfDay = "Midnight"; break;
//         case 6:  timeOfDay = "Morning";  break;
//         case 18: timeOfDay = "Evening";  break;
//       }
      
//       return timeOfDay + ": " + currentDate;
//     }
//   }
  
//   return "";
// }

String checkTime() {
  
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;
    http.begin(client, TimeAPI);
    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
      String response = http.getString();
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, response);
      if (!error) {
        String currentTime = doc["utc_datetime"].as<String>();
        int firstColonIndex = currentTime.indexOf(':');
        int secondColonIndex = currentTime.lastIndexOf(':');
        currHour = (currentTime.substring(firstColonIndex - 2, firstColonIndex)).toInt() + 3;
        currMinute = (currentTime.substring(firstColonIndex + 1, secondColonIndex)).toInt();
        currSecond = (currentTime.substring(secondColonIndex + 1, secondColonIndex + 3)).toInt();
        // utcTime = String(hour.toInt()+3) + ":" + minute;
        http.end();
        Serial.println(utcTime + "utcTIME");
        noTime = false ; 
         return utcTime;
      } else {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.print("Error on HTTP request. Response code: ");
      Serial.println(httpResponseCode);
    }  
  } else {
    Serial.println("Error in WiFi connection");

  }
   return utcTime; 
}

String prepTime(){
  String timeStem = checkTime();
  if (timeStem.length() > 4) {
    Serial.println("checktime lengh > 4");
    int pointIndex = timeStem.indexOf(".");
    timeStem.substring(0, pointIndex);
    return timeStem;
  }
  Serial.println("error in prepTime");
  return "00:01";
}
