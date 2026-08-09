#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>

// LILYGO T3-S3 H727
#define EPD_CS    15
#define EPD_DC    16
#define EPD_RST   47
#define EPD_BUSY  48

// 2.13" 250x122 display
GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display(
    GxEPD2_213_B74(
        EPD_CS,
        EPD_DC,
        EPD_RST,
        EPD_BUSY
    )
);

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println("NOVA E-PAPER TEST");

    display.init(115200);

    display.setRotation(1);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);

    display.setFullWindow();
    display.firstPage();

    do
    {
        display.fillScreen(GxEPD_WHITE);

        display.setCursor(20, 45);
        display.print("PROJECT NOVA");

        display.setCursor(20, 75);
        display.print("DISPLAY WORKS!");

    } while (display.nextPage());

    Serial.println("Display update complete.");
}

void loop()
{
}