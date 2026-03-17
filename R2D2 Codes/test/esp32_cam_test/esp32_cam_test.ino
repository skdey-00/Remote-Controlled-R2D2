/*
 * ESP32 CAM - Access Point Mode (No Router Needed!)
 * The ESP32 creates its own WiFi hotspot
 * Connect your computer/phone to "ESP32-CAM" network
 * Then open http://192.168.4.1 in your browser
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

// ==========================================
// AP MODE SETTINGS
// ==========================================
const char* ap_ssid = "ESP32-CAM";      // Network name you'll see
const char* ap_password = "12345678";   // Password to connect (must be 8+ chars)

// ==========================================
// CAMERA PIN CONFIGURATION
// ==========================================
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

WebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("\n\nESP32 CAM - Access Point Mode");
  Serial.println("================================");

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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 15;
  config.fb_count = 2;

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    Serial.println("\nCheck:");
    Serial.println("- Camera module connected properly");
    Serial.println("- Sufficient power (5V 2A recommended)");
    return;
  }

  Serial.println("✓ Camera initialized!");

  // Start Access Point
  Serial.println("\nStarting Access Point...");
  WiFi.softAP(ap_ssid, ap_password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("✓ Access Point started!\n");
  Serial.print("Network Name: ");
  Serial.println(ap_ssid);
  Serial.print("Password: ");
  Serial.println(ap_password);
  Serial.print("\nOpen this URL in your browser:\n");
  Serial.print("http://");
  Serial.println(IP);
  Serial.println("\nWaiting for connection...\n");

  // Setup web server
  server.on("/stream", HTTP_GET, handleStream);
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();
}

void loop() {
  server.handleClient();
}

void handleStream() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }

  WiFiClient client = server.client();

  // Send HTTP headers
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: image/jpeg");
  client.println("Refresh: 1");
  client.println("Connection: close");
  client.print("Content-Length: ");
  client.println(fb->len);
  client.println();

  // Send image data
  client.write(fb->buf, fb->len);

  esp_camera_fb_return(fb);
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>ESP32 CAM Live</title>";
  html += "<style>body{font-family:sans-serif;text-align:center;padding:20px;background:#f0f0f0;margin:0;}";
  html += "h1{color:#333;}#container{position:relative;max-width:640px;margin:20px auto;}";
  html += "#cam{max-width:100%;border:3px solid #333;border-radius:10px;background:#000;display:block;}";
  html += "</style>";
  html += "<script>";
  html += "var loading = false;";
  html += "function refreshImage(){";
  html += "  if(loading) return;";
  html += "  loading = true;";
  html += "  var img = document.getElementById('cam');";
  html += "  var newImg = new Image();";
  html += "  newImg.onload = function(){";
  html += "    img.src = this.src;";
  html += "    loading = false;";
  html += "  };";
  html += "  newImg.src = '/stream?' + new Date().getTime();";
  html += "}";
  html += "setInterval(refreshImage, 30);";
  html += "</script>";
  html += "</head><body>";
  html += "<h1>ESP32 CAM Live Stream</h1>";
  html += "<div id='container'>";
  html += "<img id='cam' src='/stream' alt='Camera Stream' />";
  html += "</div>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}
