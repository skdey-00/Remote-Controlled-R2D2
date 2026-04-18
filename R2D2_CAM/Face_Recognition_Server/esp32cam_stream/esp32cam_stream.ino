/*
 * ESP32-CAM Face Recognition Streaming Server (Access Point Mode)
 * The ESP32-CAM hosts its own WiFi network - no external WiFi needed!
 *
 * Connect to WiFi: "R2D2_FaceRec" with password "recognize"
 * Then visit: http://192.168.4.1/stream
 *
 * Based on AI-Thinker ESP32-CAM board
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClient.h>

// ================================
// ACCESS POINT SETTINGS
// ================================

// ESP32-CAM will create this WiFi network
const char* ap_ssid = "R2D2_FaceRec";     // WiFi network name
const char* ap_password = "recognize";     // WiFi password (min 8 chars)

// Camera settings
#define CAMERA_MODEL_AI_THINKER
#define FRAME_SIZE FRAMESIZE_SVGA  // 800x600 (good balance)
#define JPEG_QUALITY 12            // Lower = better quality (10-20)
#define CLOCK_FREQUENCY 20000000   // 20MHz (stable, works on most modules)

// ================================
// CAMERA PIN DEFINITIONS
// ================================

#if defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#else
#error "Camera model not defined"
#endif

// ================================
// SERVER CONFIGURATION
// ================================

WiFiServer server(80);  // Web server on port 80
String header;

// ================================
// SETUP
// ================================

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("==================================");
  Serial.println("R2D2 Face Recognition Camera");
  Serial.println("==================================");
  Serial.println();

  // Configure camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = CLOCK_FREQUENCY;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAME_SIZE;
  config.jpeg_quality = JPEG_QUALITY;
  config.fb_count = 2;

  // Initialize camera
  Serial.println("Initializing camera...");
  esp_err_t err = esp_camera_init(&config);

  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  Serial.println("Camera OK!");

  // Start Access Point
  Serial.println();
  Serial.println("Starting Access Point...");
  Serial.print("Network Name: ");
  Serial.println(ap_ssid);
  Serial.print("Password: ");
  Serial.println(ap_password);
  Serial.println();

  WiFi.softAP(ap_ssid, ap_password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP Address: ");
  Serial.println(IP);
  Serial.println();

  // Print instructions
  Serial.println("==================================");
  Serial.println("SETUP COMPLETE!");
  Serial.println("==================================");
  Serial.println();
  Serial.println("Steps to connect:");
  Serial.println("1. On your phone/laptop, connect to WiFi: '" + String(ap_ssid) + "'");
  Serial.println("2. Password: '" + String(ap_password) + "'");
  Serial.println("3. Open browser to: http://" + IP.toString() + "/stream");
  Serial.println();
  Serial.println("For Python face recognition server:");
  Serial.println("Use this URL in Python: http://" + IP.toString() + "/stream");
  Serial.println("==================================");
  Serial.println();

  // Start web server
  server.begin();
  Serial.println("Server started! Waiting for connections...");
}

// ================================
// MAIN LOOP
// ================================

void loop() {
  WiFiClient client = server.available();

  if (client) {
    Serial.println("New client!");
    String currentLine = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        header += c;

        if (c == '\n') {
          if (currentLine.length() == 0) {
            // Send response
            if (header.indexOf("GET /stream") >= 0) {
              sendStream(client);
            } else if (header.indexOf("GET /capture") >= 0) {
              sendCapture(client);
            } else {
              sendHomePage(client);
            }
            break;
          } else {
            currentLine = "";
          }
        }
      }
    }

    header = "";
    client.stop();
    Serial.println("Client disconnected");
  }
}

// ================================
// HELPER FUNCTIONS
// ================================

void sendStream(WiFiClient& client) {
  Serial.println("Stream starting...");

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println("Access-Control-Allow-Origin: *");
  client.println();

  while (client.connected()) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Frame buffer failed");
      break;
    }

    client.println("--frame");
    client.println("Content-Type: image/jpeg");
    client.println("Content-Length: " + String(fb->len));
    client.println();
    client.write(fb->buf, fb->len);
    client.println();

    esp_camera_fb_return(fb);
    delay(30);
  }
  Serial.println("Stream ended");
}

void sendCapture(WiFiClient& client) {
  camera_fb_t* fb = esp_camera_fb_get();
  if (fb) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: image/jpeg");
    client.println("Content-Length: " + String(fb->len));
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    client.write(fb->buf, fb->len);
    esp_camera_fb_return(fb);
  } else {
    client.println("HTTP/1.1 500 Internal Server Error");
    client.println("Content-Type: text/plain");
    client.println();
    client.println("Failed to capture");
  }
}

void sendHomePage(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();

  String ip = WiFi.softAPIP().toString();
  client.println("<!DOCTYPE html><html><head><title>R2D2 FaceCam</title>");
  client.println("<style>body{font-family:Arial;background:#1a1a2e;color:#eee;margin:40px;}");
  client.println("h1{color:#00ff88}.box{background:#16213e;padding:20px;border-radius:10px;margin:20px 0;}");
  client.println("a{color:#00ff88;text-decoration:none}.btn{background:#00ff88;color:#000;");
  client.println("padding:10px 20px;border:none;border-radius:5px;cursor:pointer;font-size:16px;}");
  client.println("</style></head><body>");
  client.println("<h1>R2D2 Face Recognition Camera</h1>");
  client.println("<div class='box'>");
  client.println("<p><strong>WiFi Network:</strong> " + String(ap_ssid) + "</p>");
  client.println("<p><strong>Password:</strong> " + String(ap_password) + "</p>");
  client.println("<p><strong>IP Address:</strong> " + ip + "</p>");
  client.println("</div>");
  client.println("<div class='box'>");
  client.println("<p><a href='/stream'><button class='btn'>View Live Stream</button></a></p>");
  client.println("<p><a href='/capture'><button class='btn'>Take Photo</button></a></p>");
  client.println("</div>");
  client.println("</body></html>");
}
