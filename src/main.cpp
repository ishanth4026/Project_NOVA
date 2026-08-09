#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

// LILYGO T3-S3 TF card pins
#define SD_CS    13
#define SD_MOSI  11
#define SD_MISO  2
#define SD_SCK   14

SPIClass sdSPI(FSPI);

void setup() {
    Serial.begin(115200);
    delay(3000);

    Serial.println("NOVA starting...");

    // Start SPI
    sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    // Initialize SD card
    if (!SD.begin(SD_CS, sdSPI)) {
        Serial.println("SD card initialization failed!");
        return;
    }

    Serial.println("SD card initialized.");

    // Check whether tasks.json already exists
    if (SD.exists("/tasks.json")) {
        Serial.println("tasks.json already exists.");
        Serial.println("Leaving existing tasks untouched.");
        return;
    }

    // File doesn't exist, so create it
    Serial.println("tasks.json does not exist.");
    Serial.println("Creating tasks.json...");

    File file = SD.open("/tasks.json", FILE_WRITE);

    if (!file) {
        Serial.println("Could not create tasks.json");
        return;
    }

    file.println("{");
    file.println("  \"tasks\": [");

    file.println("    {");
    file.println("      \"title\": \"Finish NOVA project\",");
    file.println("      \"completed\": false");
    file.println("    },");

    file.println("    {");
    file.println("      \"title\": \"Study physics\",");
    file.println("      \"completed\": false");
    file.println("    }");

    file.println("  ]");
    file.println("}");

    file.close();

    Serial.println("tasks.json created successfully!");
}

void loop() {
}