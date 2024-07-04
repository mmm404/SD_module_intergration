#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>


#define SS_PIN  D8 
#define RST_PIN D2 

#include <TimeLib.h>

unsigned long timeOffset = 0;
String lastReturnedDate = "";
int lastReturnedHour = -1;

String cardUID;
MFRC522 rfid(SS_PIN, RST_PIN);
bool dataSent = false;
bool internetPresent = true;
const byte interruptPin = 5;//for pin D1 ESP
const char* ssid = "CDED";
const char* password = "CDED2024.";
const char* TimeAPI = "http://worldtimeapi.org/api/timezone/Africa/Nairobi";
String utcTime;

void setup() {
  Serial.begin(9600);
  WiFi.disconnect(); 
  WiFi.begin(ssid, password);
  pinMode(interruptPin,OUTPUT);
  digitalWrite(interruptPin,LOW);
  Serial.println("Connecting to WiFi");
  internetPresent = ConnectWIFI(20);
  if(internetPresent == false){
    Serial.println("\n not Connected to WiFi network");
  }else{
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP()); 
  }
  
  SPI.begin(); 
  rfid.PCD_Init();
  Serial1.begin(115200);
}


void loop() {

  sendToMEGA(readTag());
        //  digitalWrite(interruptPin,HIGH);
        //  delay(2000);
        //  digitalWrite(interruptPin,LOW);
        //  delay(2000);
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
  internetPresent = ConnectWIFI(3);
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
    
    if (checkTime().length() > 5  && internetPresent) {
        int pointIndex = checkTime().indexOf(".");
        String timeStem = checkTime().substring(0, pointIndex);
        DynamicJsonDocument doc(128); 
        doc["cardID"] = cardNo;
        doc["time"] = timeStem;
        String jsonData;
        serializeJson(doc, jsonData);
        Serial.print(jsonData);
        digitalWrite(interruptPin,HIGH);
        Serial1.print(jsonData);
        dataSent = true;
        delay(150);
        digitalWrite(interruptPin,LOW);
        return dataSent;
      }
      else{
        DynamicJsonDocument doc(128); 
        doc["cardID"] = cardNo;
        doc["time"] = "------";
        String jsonData;
        serializeJson(doc, jsonData);
        Serial.print(jsonData);
        digitalWrite(interruptPin,HIGH);
        Serial1.print(jsonData);
        internetPresent = ConnectWIFI(2);
        dataSent = true;
        delay(150);
        digitalWrite(interruptPin,LOW);
        return dataSent;
      }
   
  }
  return false;
}


void setupTime(int hr, int min, int sec, int day, int month, int yr) {
  setTime(hr, min, sec, day, month, yr);
  timeOffset = now() - millis() / 1000;
}

String getDateAtSetTimes() {
  unsigned long currentTime = millis() / 1000 + timeOffset;
  setTime(currentTime);
  int currentHour = hour();
  int currentMinute = minute();
  if (currentMinute == 0 && (currentHour == 0 || currentHour == 6 || currentHour == 18)) {
    if (currentHour != lastReturnedHour) {
      char dateStr[11];
      sprintf(dateStr, "%04d-%02d-%02d", year(), month(), day());
      String currentDate = String(dateStr);
      
      lastReturnedHour = currentHour;
      lastReturnedDate = currentDate;
      
      String timeOfDay;
      switch(currentHour) {
        case 0:  timeOfDay = "Midnight"; break;
        case 6:  timeOfDay = "Morning";  break;
        case 18: timeOfDay = "Evening";  break;
      }
      
      return timeOfDay + ": " + currentDate;
    }
  }
  
  return "";
}

String checkTime() {
  utcTime = "";
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
        int lastColonIndex = currentTime.lastIndexOf(':');
        String hour = currentTime.substring(firstColonIndex - 2, firstColonIndex);
        String minute = currentTime.substring(firstColonIndex + 1, lastColonIndex);
        utcTime = String(hour.toInt()+3) + ":" + minute;
        http.end();
        Serial.println(utcTime);
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
  return ""; 
}

