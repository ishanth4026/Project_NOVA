#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>

// --- E-Paper Libraries ---
#include <GxEPD.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxDEPG0213BN/GxDEPG0213BN.h> // 2.13" b/w display
#include <Fonts/FreeMonoBold9pt7b.h>

// --- Pin Definitions ---
#define EDP_BUSY_PIN  48
#define EDP_RSET_PIN  47
#define EDP_DC_PIN    16
#define EDP_CS_PIN    15

#define SDCARD_MOSI   11
#define SDCARD_SCLK   14
#define SDCARD_MISO   2
#define SDCARD_CS     13

#define ENCODER_CLK   39
#define ENCODER_DT    40
#define ENCODER_SW    38

// --- Global Objects ---
SPIClass SDSPI(HSPI);
GxIO_Class io(SDSPI, EDP_CS_PIN, EDP_DC_PIN, EDP_RSET_PIN);
GxEPD_Class display(io, EDP_RSET_PIN, EDP_BUSY_PIN);

// --- Task Data Structure (RAM) ---
struct TaskItem {
    String title;
    bool completed;
};

#define MAX_TASKS 10
TaskItem taskList[MAX_TASKS];
int totalTasks = 0;
int selectedTaskIndex = 0;

// --- Encoder State & Timers ---
int lastStateCLK = HIGH;
unsigned long lastButtonPress = 0;
unsigned long lastEncoderChange = 0; 

// --- Function Prototypes ---
void ensureTasksFile();
void loadTasksToRAM();
void saveTasksToSD();
void drawAllTasks();
void updateSelection(int oldIndex, int newIndex);
void updateCheckbox(int index);

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n--- PROJECT NOVA BOOTING ---");

    // 1. Initialize Encoder Pins
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);
    lastStateCLK = digitalRead(ENCODER_CLK);

    // 2. SECURE THE SPI BUS FIRST
    pinMode(EDP_CS_PIN, OUTPUT);
    digitalWrite(EDP_CS_PIN, HIGH);
    pinMode(SDCARD_CS, OUTPUT);
    digitalWrite(SDCARD_CS, HIGH);

    // 3. Initialize SPI Bus
    SDSPI.begin(SDCARD_SCLK, SDCARD_MISO, SDCARD_MOSI, SDCARD_CS);

    // 4. Initialize E-Paper
    display.init();
    display.setRotation(1); 
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
    display.fillScreen(GxEPD_WHITE);

    // 5. Initialize SD Card
    if (!SD.begin(SDCARD_CS, SDSPI)) {
        Serial.println("ERROR: SD CARD FAILED TO MOUNT!");
        display.setCursor(10, 30);
        display.print("SD Card Error!");
        display.update();
        return; 
    }
    Serial.println("SD CARD MOUNTED SUCCESSFULLY.");

    // 6. Check JSON & Load Data
    ensureTasksFile();
    loadTasksToRAM();
}

void loop() {
    // --- Rotary Encoder Polling (Scrolling) ---
    int currentStateCLK = digitalRead(ENCODER_CLK);
    
    if (currentStateCLK != lastStateCLK && currentStateCLK == LOW) {
        if (millis() - lastEncoderChange > 5) {
            int oldIndex = selectedTaskIndex;
            
            if (digitalRead(ENCODER_DT) != currentStateCLK) {
                selectedTaskIndex--; // Scroll Up
            } else {
                selectedTaskIndex++; // Scroll Down
            }
            
            if (selectedTaskIndex < 0) selectedTaskIndex = 0;
            if (selectedTaskIndex >= totalTasks) selectedTaskIndex = totalTasks - 1;
            
            if (oldIndex != selectedTaskIndex) {
                updateSelection(oldIndex, selectedTaskIndex);
            }
            
            lastEncoderChange = millis();
        }
    }
    lastStateCLK = currentStateCLK;

    // --- Encoder Button Polling (Toggle Task) ---
    if (digitalRead(ENCODER_SW) == LOW) {
        if (millis() - lastButtonPress > 250) { // 250ms debounce
            
            // 1. Toggle state in RAM immediately
            taskList[selectedTaskIndex].completed = !taskList[selectedTaskIndex].completed;
            
            Serial.print("Task '");
            Serial.print(taskList[selectedTaskIndex].title);
            Serial.print("' marked as ");
            Serial.println(taskList[selectedTaskIndex].completed ? "COMPLETED" : "PENDING");
            
            // 2. Perform partial refresh IMMEDIATELY for instant visual feedback
            updateCheckbox(selectedTaskIndex);
            
            // 3. Save to SD card instantly so it survives a power cut
            saveTasksToSD();
            
            lastButtonPress = millis();
        }
    }
}

// --- Helper Functions ---

void ensureTasksFile() {
    File file = SD.open("/tasks.json", FILE_READ);
    if (!file || file.size() == 0) {
        if (file) file.close();
        Serial.println("tasks.json missing/empty. Generating default...");
        
        file = SD.open("/tasks.json", FILE_WRITE);
        if (file) {
            const char* defaultJSON = "{\"tasks\":[{\"title\":\"Study\",\"completed\":false},{\"title\":\"Project NOVA\",\"completed\":false}]}";
            file.print(defaultJSON);
            file.flush();
            file.close();
        }
    } else {
        file.close();
    }
}

void loadTasksToRAM() {
    File file = SD.open("/tasks.json", FILE_READ);
    if (!file) return;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.print("JSON Parse Error: ");
        Serial.println(error.c_str());
        return;
    }

    Serial.println("\n--- LOADING TASKS FROM SD CARD ---");
    totalTasks = 0;
    JsonArray tasks = doc["tasks"];
    for (JsonObject task : tasks) {
        if (totalTasks >= MAX_TASKS) break;
        
        taskList[totalTasks].title = task["title"].as<String>();
        taskList[totalTasks].completed = task["completed"].as<bool>();
        
        // Diagnostic Print to prove what the SD card actually saved
        Serial.print("Loaded Task: ");
        Serial.print(taskList[totalTasks].title);
        Serial.print(" -> Completed State: ");
        Serial.println(taskList[totalTasks].completed ? "TRUE (Should draw X)" : "FALSE (Should be empty)");
        
        totalTasks++;
    }
    Serial.println("----------------------------------\n");
    drawAllTasks();
}

void saveTasksToSD() {
    Serial.println("\n--- SAVING TO SD CARD ---");
    digitalWrite(EDP_CS_PIN, HIGH);
    
    // BULLETPROOF FIX: Aggressively delete the old file so the ESP32 can't append to it
    if (SD.exists("/tasks.json")) {
        SD.remove("/tasks.json");
        Serial.println("Deleted old tasks.json");
    }
    
    File file = SD.open("/tasks.json", FILE_WRITE);
    if (!file) {
        Serial.println("ERROR: Failed to open tasks.json for writing.");
        return;
    }

    JsonDocument doc;
    JsonArray tasksArray = doc["tasks"].to<JsonArray>();
    
    for (int i = 0; i < totalTasks; i++) {
        JsonObject taskObj = tasksArray.add<JsonObject>();
        taskObj["title"] = taskList[i].title;
        taskObj["completed"] = taskList[i].completed;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.println("ERROR: Failed to write JSON to file.");
    } else {
        file.flush(); // Force hardware push
        Serial.println("New tasks.json created and saved successfully!");
    }
    file.close();
    Serial.println("--- SAVE COMPLETE ---\n");
}

void drawAllTasks() {
    display.fillScreen(GxEPD_WHITE);
    
    display.setCursor(20, 20);
    display.print("TASKS");
    display.drawLine(20, 25, 210, 25, GxEPD_BLACK);

    for (int i = 0; i < totalTasks; i++) {
        int yOffset = 50 + (i * 25);
        
        if (i == selectedTaskIndex) {
            display.setCursor(0, yOffset);
            display.print(">");
        }
        
        display.drawRect(20, yOffset - 12, 12, 12, GxEPD_BLACK);
        
        // THIS is the drawing logic that runs on boot.
        if (taskList[i].completed) {
            display.drawLine(20, yOffset - 12, 32, yOffset, GxEPD_BLACK);
            display.drawLine(32, yOffset - 12, 20, yOffset, GxEPD_BLACK);
        }
        
        display.setCursor(40, yOffset);
        display.print(taskList[i].title);
    }

    display.update(); 
}

void updateSelection(int oldIndex, int newIndex) {
    int topY = 50 - 16;  
    int boxHeight = (totalTasks * 25); 

    display.fillRect(0, topY, 16, boxHeight, GxEPD_WHITE);

    int newY = 50 + (newIndex * 25);
    display.setCursor(0, newY);
    display.print(">");

    display.updateWindow(0, topY, 16, boxHeight);
}

void updateCheckbox(int index) {
    int yOffset = 50 + (index * 25);
    int boxX = 20;
    int boxY = yOffset - 12;
    int boxSize = 12;

    display.fillRect(boxX, boxY, boxSize, boxSize, GxEPD_WHITE);
    display.drawRect(boxX, boxY, boxSize, boxSize, GxEPD_BLACK);
    
    if (taskList[index].completed) {
        display.drawLine(boxX, boxY, boxX + boxSize, boxY + boxSize, GxEPD_BLACK);
        display.drawLine(boxX + boxSize, boxY, boxX, boxY + boxSize, GxEPD_BLACK);
    }

    display.updateWindow(boxX, boxY, boxSize, boxSize);
}