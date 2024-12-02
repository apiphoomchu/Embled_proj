#include <WebServer.h>
#include <WiFi.h>
#include <esp32cam.h>
#include <esp_now.h>

// WiFi credentials
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASS = "YOU_PASS";

// ESP-NOW Receiver MAC Address
uint8_t receiverMAC[] = {0xD0, 0xEF, 0x76, 0x57, 0x0F, 0xA8};

// Web server
WebServer server(80);

// Camera resolution
static auto loRes = esp32cam::Resolution::find(320, 240);
static auto midRes = esp32cam::Resolution::find(350, 530);
static auto hiRes = esp32cam::Resolution::find(800, 600);

// Structure for ESP-NOW data
struct QRData {
  char message[32];
};
QRData qrMessage;

// Function to send data via ESP-NOW
void sendQRData(const char* data) {
  strcpy(qrMessage.message, data);
  esp_err_t result = esp_now_send(receiverMAC, (uint8_t*)&qrMessage, sizeof(qrMessage));
  if (result == ESP_OK) {
    Serial.println("Data sent successfully");
  } else {
    Serial.println("Error sending data");
  }
}

// Serve a JPEG image
void serveJpg() {
  auto frame = esp32cam::capture();
  if (frame == nullptr) {
    Serial.println("CAPTURE FAIL");
    server.send(503, "", "");
    return;
  }
  Serial.printf("CAPTURE OK %dx%d %db\n", frame->getWidth(), frame->getHeight(),
                static_cast<int>(frame->size()));

  server.setContentLength(frame->size());
  server.send(200, "image/jpeg");
  WiFiClient client = server.client();
  frame->writeTo(client);
}

// Handle different resolution requests
void handleJpgLo() {
  if (!esp32cam::Camera.changeResolution(loRes)) {
    Serial.println("SET-LO-RES FAIL");
  }
  serveJpg();
}

void handleJpgMid() {
  if (!esp32cam::Camera.changeResolution(midRes)) {
    Serial.println("SET-MID-RES FAIL");
  }
  serveJpg();
}

void handleJpgHi() {
  if (!esp32cam::Camera.changeResolution(hiRes)) {
    Serial.println("SET-HI-RES FAIL");
  }
  serveJpg();
}

// Handle QR code data submission
void handleQRData() {
  if (server.hasArg("plain")) {
    String decodedData = server.arg("plain");
    Serial.println("Received QR Data: " + decodedData);

    // Send data via ESP-NOW
    sendQRData(decodedData.c_str());

    // Check if the data matches "password"
    if (decodedData == "password") {
      Serial.println("Access Granted: Welcome");
    } else {
      Serial.println("Access Denied");
    }

    server.send(200, "text/plain", "QR Data Processed");
  } else {
    server.send(400, "text/plain", "No QR Data Received");
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize the camera
  using namespace esp32cam;
  Config cfg;
  cfg.setPins(pins::AiThinker);
  cfg.setResolution(hiRes);
  cfg.setBufferCount(2);
  cfg.setJpeg(80);

  bool ok = Camera.begin(cfg);
  Serial.println(ok ? "CAMERA OK" : "CAMERA FAIL");

  // Connect to WiFi
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

  // Set up HTTP server routes
  server.on("/cam-lo.jpg", handleJpgLo);
  server.on("/cam-mid.jpg", handleJpgMid);
  server.on("/cam-hi.jpg", handleJpgHi);
  server.on("/receive-data", HTTP_POST, handleQRData);

  server.begin();
  Serial.println("HTTP server started");

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register ESP-NOW peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  
  Serial.println("ESP-NOW initialized");
}

void loop() {
  server.handleClient();
}
