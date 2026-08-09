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
    delay(2000);

    Serial.println();
    Serial.println("====================");
    Serial.println("   SD CARD TEST");
    Serial.println("====================");

    // Start SPI with the SD card pins
    sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    Serial.println("Initializing SD card...");

    if (!SD.begin(SD_CS, sdSPI)) {
        Serial.println("❌ SD CARD FAILED!");
        Serial.println("Check that the microSD card is inserted.");
        return;
    }

    Serial.println("✅ SD CARD INITIALIZED!");

    // Check card type
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE) {
        Serial.println("❌ No SD card detected.");
        return;
    }

    Serial.print("Card type: ");

    if (cardType == CARD_MMC) {
        Serial.println("MMC");
    }
    else if (cardType == CARD_SD) {
        Serial.println("SDSC");
    }
    else if (cardType == CARD_SDHC) {
        Serial.println("SDHC");
    }
    else {
        Serial.println("UNKNOWN");
    }

    // Card size
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);

    Serial.print("Card size: ");
    Serial.print(cardSize);
    Serial.println(" MB");

    // Create a test file
    File file = SD.open("/test.txt", FILE_WRITE);

    if (!file) {
        Serial.println("❌ Could not create test.txt");
        return;
    }

    file.println("Project NOVA SD card test!");
    file.close();

    Serial.println("✅ Created /test.txt");

    // Read the file
    file = SD.open("/test.txt");

    if (!file) {
        Serial.println("❌ Could not open test.txt");
        return;
    }

    Serial.println("Contents of test.txt:");

    while (file.available()) {
        Serial.write(file.read());
    }

    file.close();

    Serial.println();
    Serial.println("====================");
    Serial.println("SD CARD TEST PASSED!");
    Serial.println("====================");
}

void loop() {
}