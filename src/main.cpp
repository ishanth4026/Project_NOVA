#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>

#include "utilities.h"

#include <GxEPD.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxDEPG0213BN/GxDEPG0213BN.h>

#include <Fonts/FreeMonoBold9pt7b.h>
int BUTTON_PIN = 38;


// ========================================
// SPI
// ========================================

SPIClass SDSPI(HSPI);


// ========================================
// E-PAPER
// ========================================

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


// ========================================
// SETUP
// ========================================

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println();
    Serial.println("======================");
    Serial.println(" PROJECT NOVA TASKS");
    Serial.println("======================");


    // ====================================
    // START SPI
    // ====================================

    Serial.println("Starting SPI...");

    SDSPI.begin(
        SDCARD_SCLK,
        SDCARD_MISO,
        SDCARD_MOSI,
        SDCARD_CS
    );

    Serial.println("SPI started.");


    // ====================================
    // INITIALIZE DISPLAY
    // ====================================

    Serial.println("Initializing display...");

    display.init();

    display.setRotation(1);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);

    Serial.println("Display initialized.");


    // ====================================
    // IMPORTANT:
    // DESELECT DISPLAY BEFORE SD ACCESS
    // ====================================

    pinMode(EDP_CS_PIN, OUTPUT);
    digitalWrite(EDP_CS_PIN, HIGH);


    // ====================================
    // INITIALIZE SD CARD
    // ====================================

    Serial.println("Initializing SD card...");

    pinMode(SDCARD_CS, OUTPUT);
    digitalWrite(SDCARD_CS, HIGH);

    if (!SD.begin(SDCARD_CS, SDSPI))
    {
        Serial.println("SD CARD FAILED!");

        display.fillScreen(GxEPD_WHITE);

        display.setCursor(10, 45);
        display.println("SD CARD ERROR");

        display.update();

        return;
    }

    Serial.println("SD card initialized!");


    // ====================================
    // OPEN TASKS.JSON
    // ====================================

    File file = SD.open("/tasks.json");

    if (!file)
    {
        Serial.println("Could not open tasks.json");

        display.fillScreen(GxEPD_WHITE);

        display.setCursor(10, 45);
        display.println("TASK FILE ERROR");

        display.update();

        return;
    }

    Serial.println("tasks.json opened.");


    // ====================================
    // LOAD JSON INTO RAM
    // ====================================

    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, file);

    file.close();

    if (error)
    {
        Serial.print("JSON error: ");
        Serial.println(error.c_str());

        display.fillScreen(GxEPD_WHITE);

        display.setCursor(10, 45);
        display.println("JSON ERROR");

        display.update();

        return;
    }

    Serial.println("JSON loaded into RAM!");


    // ====================================
    // GET TASK ARRAY
    // ====================================

    JsonArray tasks = doc["tasks"];

    int taskCount = tasks.size();

    Serial.print("Tasks found: ");
    Serial.println(taskCount);


    // ====================================
    // DRAW TASK PAGE
    // ====================================

    display.fillScreen(GxEPD_WHITE);

    // Title
    display.setCursor(10, 18);
    display.println("TASKS");

    // Line underneath title
    display.drawLine(
        10,
        23,
        240,
        23,
        GxEPD_BLACK
    );


    // ====================================
    // DRAW TASKS
    // ====================================

    int y = 43;

    for (int i = 0; i < taskCount; i++)
    {
        const char* title = tasks[i]["title"];
        bool completed = tasks[i]["completed"];


        // Checkbox
        display.drawRect(
            10,
            y - 10,
            10,
            10,
            GxEPD_BLACK
        );


        // Draw X if completed
        if (completed)
        {
            display.drawLine(
                10,
                y - 10,
                20,
                y,
                GxEPD_BLACK
            );

            display.drawLine(
                20,
                y - 10,
                10,
                y,
                GxEPD_BLACK
            );
        }


        // Task title
        display.setCursor(27, y);
        display.println(title);


        // Next task
        y += 28;


        // Don't draw outside the screen
        if (y > 115)
        {
            break;
        }
    }


    // ====================================
    // UPDATE DISPLAY
    // ====================================

    Serial.println("Updating display...");

    display.update();

    Serial.println("Display updated!");

    Serial.println("======================");
    Serial.println("       COMPLETE");
    Serial.println("======================");
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    
}


void loop()
{
    static bool lastState = HIGH;
    bool currentState = digitalRead(BUTTON_PIN);

    // Only print when the state changes (avoids spamming Serial)
    if (currentState != lastState)
    {
        if (currentState == LOW)
        {
            Serial.println("Button PRESSED");
        }
        else
        {
            Serial.println("Button RELEASED");
        }

        lastState = currentState;
    }
}