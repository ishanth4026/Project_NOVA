#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <GxEPD.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxDEPG0213BN/GxDEPG0213BN.h>
#include <utilities.h> // LILYGO pin definitions

// --- Encoder Pin Definitions ---
#define ENCODER_CLK   39
#define ENCODER_DT    40
#define ENCODER_SW    38

// --- Global Objects ---
SPIClass SDSPI(HSPI);
GxIO_Class io(SDSPI, EDP_CS_PIN, EDP_DC_PIN, EDP_RSET_PIN);
GxEPD_Class display(io, EDP_RSET_PIN, EDP_BUSY_PIN);

// --- Task Data Structure ---
struct TaskItem { String title; bool completed; };
#define MAX_TASKS 10
TaskItem taskList[MAX_TASKS];
int totalTasks = 0;
int selectedTaskIndex = 0;

// --- State ---
int lastStateCLK = HIGH;
unsigned long lastButtonPress = 0;
unsigned long lastEncoderChange = 0;

// --- Prototypes ---
void ensureTasksFile();
void loadTasksToRAM();
void saveTasksToSD();
void drawUI();
void updateSelection(int oldIndex, int newIndex);
void updateCheckbox(int index);

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n--- PROJECT NOVA BOOTING ---");

    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);
    
    // Secure SPI Bus Pins high on boot so nothing transmits early
    pinMode(EDP_CS_PIN, OUTPUT); digitalWrite(EDP_CS_PIN, HIGH);
    pinMode(SDCARD_CS, OUTPUT); digitalWrite(SDCARD_CS, HIGH);

    // Initialize SPI bus exactly once
    SDSPI.begin(SDCARD_SCLK, SDCARD_MISO, SDCARD_MOSI, SDCARD_CS);
    
    display.init();
    display.setRotation(1);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(); // Default small font

    // Explicitly lock out the display before mounting the SD card
    digitalWrite(EDP_CS_PIN, HIGH); 
    
    if (SD.begin(SDCARD_CS, SDSPI, 4000000, "/sd", 5, true)) {
        Serial.println("SD Card mounted successfully.");
        ensureTasksFile();
        loadTasksToRAM();
    } else {
        Serial.println("ERROR: SD Mount Failed");
        display.fillScreen(GxEPD_WHITE);
        display.setCursor(10, 10);
        display.print("SD Mount Failed");
        display.update();
    }
}

void loop() {
    // --- Scroll Logic (Using your exact working template) ---
    int currentStateCLK = digitalRead(ENCODER_CLK);
    if (currentStateCLK != lastStateCLK && currentStateCLK == LOW) {
        if (millis() - lastEncoderChange > 5) {
            int oldIndex = selectedTaskIndex;
            if (digitalRead(ENCODER_DT) != currentStateCLK) selectedTaskIndex--;
            else selectedTaskIndex++;
            
            if (selectedTaskIndex < 0) selectedTaskIndex = 0;
            if (selectedTaskIndex >= totalTasks) selectedTaskIndex = totalTasks - 1;
            
            if (oldIndex != selectedTaskIndex) updateSelection(oldIndex, selectedTaskIndex);
            lastEncoderChange = millis();
        }
    }
    lastStateCLK = currentStateCLK;

    // --- Button Logic ---
    if (digitalRead(ENCODER_SW) == LOW && (millis() - lastButtonPress > 250)) {
        taskList[selectedTaskIndex].completed = !taskList[selectedTaskIndex].completed;
        
        // 1. Update the E-Paper display
        updateCheckbox(selectedTaskIndex);
        
        // 2. Give the hardware a moment to settle
        delay(50); 
        
        // 3. Save to SD Card
        saveTasksToSD();
        
        lastButtonPress = millis();
    }
}

void ensureTasksFile() {
    File file = SD.open("/tasks.json", FILE_READ);
    if (!file || file.size() == 0) {
        if (file) file.close();
        
        file = SD.open("/tasks.json", FILE_WRITE);
        if (file) {
            file.print("{\"tasks\":[{\"title\":\"Study\",\"completed\":false},{\"title\":\"Project NOVA\",\"completed\":false}]}");
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

    totalTasks = 0;
    JsonArray tasks = doc["tasks"];
    for (JsonObject task : tasks) {
        if (totalTasks >= MAX_TASKS) break;
        taskList[totalTasks].title = task["title"].as<String>();
        taskList[totalTasks].completed = task["completed"].as<bool>();
        totalTasks++;
    }
    drawUI();
}

void saveTasksToSD() {
    // 1. Force the display to let go of the SPI bus!
    digitalWrite(EDP_CS_PIN, HIGH);
    delay(10);

    // 2. NUCLEAR OPTION: Unmount the SD card completely to clear the coma
    SD.end();
    delay(20);
    
    // 3. Wake it back up fresh with full parameters
    if (!SD.begin(SDCARD_CS, SDSPI, 4000000, "/sd", 5, true)) {
        Serial.println("ERROR: Failed to wake up SD card after E-Paper update!");
        return;
    }

    // 4. Safely overwrite the file
    if (SD.exists("/tasks.json")) {
        SD.remove("/tasks.json");
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
    
    serializeJson(doc, file);
    file.flush();
    file.close();
    
    Serial.println("SD Card update successful. Survived the SPI coma!");
}

void drawUI() {
    display.fillScreen(GxEPD_WHITE);

    // --- Header ---
    display.setCursor(15, 5);
    display.print("PROJECT NOVA : TASKS");
    display.drawLine(5, 14, 240, 14, GxEPD_BLACK);

    // --- Task List View (Shifted to Left Side, Full Screen Area) ---
    for (int i = 0; i < totalTasks; i++) {
        int yOffset = 28 + (i * 18);
        
        if (i == selectedTaskIndex) {
            display.setCursor(5, yOffset);
            display.print(">");
        }
        
        display.drawRect(15, yOffset - 8, 9, 9, GxEPD_BLACK);
        if (taskList[i].completed) {
            display.drawLine(15, yOffset - 8, 24, yOffset + 1, GxEPD_BLACK);
            display.drawLine(24, yOffset - 8, 15, yOffset + 1, GxEPD_BLACK);
        }
        
        display.setCursor(30, yOffset);
        display.print(taskList[i].title);
    }

    display.update();
}

void updateSelection(int oldIndex, int newIndex) {
    display.fillRect(0, 28 - 10, 12, (totalTasks * 18), GxEPD_WHITE);
    int newY = 28 + (newIndex * 18);
    display.setCursor(5, newY);
    display.print(">");
    display.updateWindow(0, 28 - 10, 12, (totalTasks * 18));
}

void updateCheckbox(int index) {
    int yOffset = 28 + (index * 18);
    display.fillRect(15, yOffset - 8, 9, 9, GxEPD_WHITE);
    display.drawRect(15, yOffset - 8, 9, 9, GxEPD_BLACK);
    if (taskList[index].completed) {
        display.drawLine(15, yOffset - 8, 24, yOffset + 1, GxEPD_BLACK);
        display.drawLine(24, yOffset - 8, 15, yOffset + 1, GxEPD_BLACK);
    }
    display.updateWindow(15, yOffset - 8, 9, 9);
}