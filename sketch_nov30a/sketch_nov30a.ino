#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <esp_now.h>
#include <WiFi.h>

// RFID pins
#define SS_PIN 5
#define RST_PIN 2

// Servo pin
#define SERVO_PIN 25

// Button pin
#define BUTTON_PIN 4

MFRC522 rfid(SS_PIN, RST_PIN);
Servo servo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

byte authorizedUID[] = {0xB3, 0x5E, 0x0D, 0x30};

// Structure for received ESP-NOW message
struct QRData {
  char message[32];
};

// Structure for light and distance
struct Message {
  int distance;
  int light;
} receivedMessage = {200, 0}; // Default values

QRData receivedQR;
bool qrReceived = false; // Flag to indicate if QR data is received

String previousLCDLine1 = "";
String previousLCDLine2 = "";

// Timing variables
unsigned long welcomeStartTime = 0;
bool showingWelcome = false;

// Function to update the LCD only when data changes
void updateLCD(String line1, String line2) {
  if (line1 != previousLCDLine1 || line2 != previousLCDLine2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
    previousLCDLine1 = line1;
    previousLCDLine2 = line2;
  }
}

// Callback function for ESP-NOW
void onReceive(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (len == sizeof(Message)) {
    memcpy(&receivedMessage, data, sizeof(receivedMessage));
    Serial.print("Received distance: ");
    Serial.print(receivedMessage.distance);
    Serial.print(" cm, Light Intensity: ");
    Serial.println(receivedMessage.light);
  } else if (len == sizeof(QRData)) {
    memcpy(&receivedQR, data, sizeof(receivedQR));
    qrReceived = true; // Mark QR data as received
    Serial.print("Received QR Message: ");
    Serial.println(receivedQR.message);

    // Handle QR message
    if (String(receivedQR.message) == "password") {
      updateLCD("Welcome", "");
      servo.write(0); // Open door
      welcomeStartTime = millis(); // Start welcome display timer
      showingWelcome = true;      // Indicate welcome is being displayed
    } else {
      updateLCD("", ""); // Clear the display if the QR code does not match
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize RFID
  SPI.begin();
  rfid.PCD_Init();

  // Initialize LCD
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  updateLCD("Your Card please", "");

  // Initialize Servo
  servo.attach(SERVO_PIN);
  servo.write(90);

  // Initialize Button
  pinMode(BUTTON_PIN, INPUT);

  // Initialize WiFi in station mode
  WiFi.mode(WIFI_IF_AP);
  WiFi.disconnect();

  Serial.print("ESP32 Receiver MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register the ESP-NOW receive callback
  esp_now_register_recv_cb(onReceive);

  delay(2000);
}

void loop() {
  // Check if "Welcome" message should be cleared
  if (showingWelcome && millis() - welcomeStartTime >= 3000) {
    showingWelcome = false; // Reset the flag
    servo.write(180);       // Close door
    delay(1000);            // Ensure the servo movement is completed
    servo.write(90);        // Reset servo
    updateLCD("", "");      // Clear LCD
  }

  // Display light and distance data on LCD
  String line1 = "";
  String line2 = "";

  if (receivedMessage.light > 900) {
    line1 = "Sunny Day!";
    line2 = "Light: " + String(receivedMessage.light);
  } else if (receivedMessage.distance <= 150) {
    line1 = "Someone Approaching";
    line2 = "Distance: " + String(receivedMessage.distance) + " cm";
  } else {
    line1 = "Light: " + String(receivedMessage.light);
    line2 = "Distance: " + String(receivedMessage.distance) + " cm";
  }

  // Update the LCD only if the lines change and not showing welcome
  if (!showingWelcome) {
    updateLCD(line1, line2);
  }

  // RFID handling
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Serial.print("UID tag: ");
    String content = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(rfid.uid.uidByte[i], HEX);
      content.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
      content.concat(String(rfid.uid.uidByte[i], HEX));
    }
    Serial.println();

    // Check if the RFID tag is authorized
    bool authorized = true;
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] != authorizedUID[i]) {
        authorized = false;
        break;
      }
    }

    if (authorized) {
      updateLCD("Welcome Home", "");
      servo.write(0); // Open door
      welcomeStartTime = millis(); // Start welcome display timer
      showingWelcome = true;      // Indicate welcome is being displayed
    } else {
      updateLCD("Wrong identity...", "");
    }

    delay(2000);
  }

  // Check for button press
  if (digitalRead(BUTTON_PIN) == HIGH) { // Button pressed
    updateLCD("Ring a Bell!", "");
    Serial.println("Button Pressed! Ringing Bell...");
    delay(2000);
  }

  delay(500);
}