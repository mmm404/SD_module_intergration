#include <SPI.h>
#include <SD.h>

const int chipSelect = 10; // Adjust this according to your shield

void setup() {
    Serial.begin(9600);
    pinMode(chipSelect, OUTPUT);
    if (!SD.begin(chipSelect)) {
        Serial.println("Initialization failed!");
        return;
    }
    Serial.println("Initialization done.");
}

void writeData() {
    File dataFile = SD.open("datalog.txt", FILE_WRITE);
    if (dataFile) {
        dataFile.println("Hello, World!");
        dataFile.close();
        Serial.println("Data written.");
    } else {
        Serial.println("Error opening datalog.txt");
    }
}

void readData() {
    File dataFile = SD.open("datalog.txt");
    if (dataFile) {
        while (dataFile.available()) {
            Serial.write(dataFile.read());
        }
        dataFile.close();
    } else {
        Serial.println("Error opening datalog.txt");
    }
}

void loop() {
    writeData();
    delay(1000);
    readData();
    delay(1000);
}
