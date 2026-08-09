#include <Arduino.h>

#include "utilities.h"

#include <SPI.h>
#include <GxEPD.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxDEPG0213BN/GxDEPG0213BN.h>

#include <Fonts/FreeMonoBold12pt7b.h>

SPIClass SDSPI(HSPI);

GxIO_Class io(
    SDSPI,
    EDP_CS_PIN,
    EDP_DC_PIN,
    EDP_RSET_PIN
);

GxEPD_Class display(
    io,
    EDP_RSET_PIN,
    EDP_BUSY_PIN
);

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println("NOVA DISPLAY TEST");

    // Same SPI setup used by LILYGO
    SDSPI.begin(
        EDP_CLK_PIN,
        EDP_MISO_PIN,
        EDP_MOSI_PIN,
        EDP_CS_PIN
    );

    Serial.println("SPI started.");

    // Initialize display
    display.init();

    Serial.println("Display initialized.");

    display.setTextColor(GxEPD_BLACK);

    // Use the same rotation as the official example
    display.setRotation(2);

    display.fillScreen(GxEPD_WHITE);

    display.setFont(&FreeMonoBold12pt7b);

    display.setCursor(10, 45);
    display.println("PROJECT NOVA");

    display.setCursor(10, 80);
    display.println("HELLO!");

    Serial.println("Updating display...");

    display.update();

    Serial.println("Display update finished.");
}

void loop()
{
}