#include <espnow.h>
#include <ESP8266WiFi.h>
// Ultrasonic sensor pins
const int pingPin = D1;
const int inPin = D2;
// LDR sensor pin
const int ldrPin = A0;
// MAC address of the receiver ESP32
uint8_t receiverMAC[] = { 0xD0, 0xEF, 0x76, 0x57, 0x0F, 0xA8 };
// Callback when data is sent
void onSent(uint8_t *macAddr, uint8_t sendStatus) {

}
void setup() {
  Serial.begin(115200);
  // Initialize WiFi in station mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  // Initialize ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  // Set up the send callback
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(onSent);
  // Register the receiver
  if (esp_now_add_peer(receiverMAC, ESP_NOW_ROLE_SLAVE, 1, NULL, 0) != 0) {
    Serial.println("Failed to add peer");
    return;
  }
}
void loop() {
  long duration, cm;
  // Trigger the ultrasonic sensor
  pinMode(pingPin, OUTPUT);
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(pingPin, LOW);
  // Read the echo response
  pinMode(inPin, INPUT);
  duration = pulseIn(inPin, HIGH);
  // Calculate distance in cm
  cm = microsecondsToCentimeters(duration);
  // Read LDR data
  int lightValue = analogRead(ldrPin);
  // Debugging sensor 
  
  Serial.println("li" + String(lightValue) + "di" + String(cm));
  // Prepare the 
  
  struct Message {
    int distance;  // Distance in cm
    int light;     // Light intensity value
  } message;

  message.distance = cm;
  message.light = lightValue;

  // Send the message via ESP-NOW
  if (esp_now_send(receiverMAC, (uint8_t *)&message, sizeof(message)) != 0) {
  } else {
  }
  delay(1000);  // 1-second delay
}
long microsecondsToCentimeters(long microseconds) {
  return microseconds / 29 / 2;
}