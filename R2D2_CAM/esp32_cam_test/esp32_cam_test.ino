/*
 * ESP32 CAM - R2D2 Holographic Display Edition
 * Features:
 * - Holographic projection effect
 * - Motion detection
 * - Photo capture gallery with SPIFFS
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>

// ==========================================
// AP MODE SETTINGS
// ==========================================
const char* ap_ssid = "R2D2-CAM";      // Network name you'll see
const char* ap_password = "12345678";   // Password to connect (must be 8+ chars)

// ==========================================
// MOTION DETECTION SETTINGS
// ==========================================
float motionThreshold = 15.0;  // Percentage difference to trigger motion
uint8_t* previousFrame = NULL;
size_t previousFrameSize = 0;
bool motionDetected = false;
float motionLevel = 0.0;

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
int photoCount = 0;

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("\n\n========================================");
  Serial.println("R2D2 CAM - Holographic Display Edition");
  Serial.println("========================================");

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS mount failed, formatting...");
    delay(1000);
    if(!SPIFFS.begin(true)){
      Serial.println("✗ SPIFFS initialization failed!");
    } else {
      Serial.println("✓ SPIFFS formatted successfully!");
    }
  } else {
    Serial.println("✓ SPIFFS mounted successfully!");
  }

  // Count existing photos
  photoCount = countPhotos();
  Serial.printf("✓ Found %d existing photos\n", photoCount);

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

  // Setup web server routes
  server.on("/stream", HTTP_GET, handleStream);
  server.on("/capture", HTTP_GET, handleCapture);
  server.on("/gallery", HTTP_GET, handleGallery);
  server.on("/download", HTTP_GET, handleDownload);
  server.on("/delete", HTTP_GET, handleDelete);
  server.on("/list", HTTP_GET, handleList);
  server.on("/motion", HTTP_GET, handleMotionData);
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("✓ Web server started!");
}

int countPhotos() {
  File root = SPIFFS.open("/");
  if(!root){
    return 0;
  }

  int count = 0;
  File file = root.openNextFile();
  while(file){
    if(String(file.name()).endsWith(".jpg")){
      count++;
    }
    file = root.openNextFile();
  }
  return count;
}

void loop() {
  server.handleClient();
}

// ==========================================
// MOTION DETECTION FUNCTION
// ==========================================
float detectMotion(uint8_t* currentFrame, size_t currentSize) {
  if (previousFrame == NULL || previousFrameSize != currentSize) {
    // Store first frame as reference
    if (previousFrame != NULL) free(previousFrame);
    previousFrame = (uint8_t*)malloc(currentSize);
    if (previousFrame) {
      memcpy(previousFrame, currentFrame, currentSize);
      previousFrameSize = currentSize;
    }
    return 0.0;
  }

  // Compare frames (sample every 10th byte for performance)
  int sampleRate = 10;
  int differentBytes = 0;
  int totalSamples = currentSize / sampleRate;

  for (int i = 0; i < currentSize; i += sampleRate) {
    int diff = abs((int)currentFrame[i] - (int)previousFrame[i]);
    if (diff > 30) {  // Threshold for pixel difference
      differentBytes++;
    }
  }

  // Update previous frame
  memcpy(previousFrame, currentFrame, currentSize);

  // Calculate motion percentage
  float motionPercent = (float)differentBytes / totalSamples * 100.0;

  if (motionPercent > motionThreshold) {
    motionDetected = true;
    Serial.printf("⚠ MOTION DETECTED: %.1f%%\n", motionPercent);
  } else {
    motionDetected = false;
  }

  motionLevel = motionPercent;
  return motionPercent;
}

void handleStream() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }

  // Run motion detection
  detectMotion(fb->buf, fb->len);

  // Send single frame as MJPEG (browser will auto-refresh)
  WiFiClient client = server.client();
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: image/jpeg");
  client.println("Cache-Control: no-cache, no-store, must-revalidate");
  client.println("Pragma: no-cache");
  client.println("Expires: 0");
  client.print("Content-Length: ");
  client.println(fb->len);
  client.println();

  // Send image data
  client.write(fb->buf, fb->len);

  esp_camera_fb_return(fb);
}

void handleCapture() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }

  // Generate filename
  photoCount++;
  char filename[32];
  sprintf(filename, "/photo_%03d.jpg", photoCount);

  // Save to SPIFFS
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (file) {
    file.write(fb->buf, fb->len);
    file.close();
    Serial.printf("✓ Photo saved: %s (%d bytes)\n", filename, fb->len);

    String response = "{\"success\":true,\"filename\":\"";
    response += filename;
    response += "\",\"size\":";
    response += fb->len;
    response += ",\"count\":";
    response += photoCount;
    response += "}";

    server.send(200, "application/json", response);
  } else {
    Serial.printf("✗ Failed to save photo: %s\n", filename);
    server.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save photo\"}");
  }

  esp_camera_fb_return(fb);
}

void handleMotionData() {
  String json = "{\"motionDetected\":";
  json += motionDetected ? "true" : "false";
  json += ",\"motionLevel\":";
  json += motionLevel;
  json += ",\"threshold\":";
  json += motionThreshold;
  json += "}";

  server.send(200, "application/json", json);
}

void handleList() {
  String json = "{\"photos\":[";

  File root = SPIFFS.open("/");
  if (root) {
    bool first = true;
    File file = root.openNextFile();

    while (file) {
      if (String(file.name()).endsWith(".jpg")) {
        if (!first) json += ",";
        json += "{\"name\":\"";
        json += file.name();
        json += "\",\"size\":";
        json += file.size();
        json += "}";
        first = false;
      }
      file = root.openNextFile();
    }
  }

  json += "],\"count\":";
  json += photoCount;
  json += "}";

  server.send(200, "application/json", json);
}

void handleDownload() {
  if (!server.hasArg("file")) {
    server.send(400, "text/plain", "Missing file parameter");
    return;
  }

  String filename = server.arg("file");
  if (!SPIFFS.exists(filename)) {
    server.send(404, "text/plain", "File not found");
    return;
  }

  File file = SPIFFS.open(filename, FILE_READ);
  if (file) {
    server.streamFile(file, "image/jpeg");
    file.close();
  } else {
    server.send(500, "text/plain", "Failed to open file");
  }
}

void handleDelete() {
  if (!server.hasArg("file")) {
    server.send(400, "text/plain", "Missing file parameter");
    return;
  }

  String filename = server.arg("file");
  if (SPIFFS.remove(filename)) {
    photoCount = countPhotos();
    Serial.printf("✓ Deleted: %s\n", filename.c_str());
    server.send(200, "application/json", "{\"success\":true,\"count\":" + String(photoCount) + "}");
  } else {
    server.send(500, "application/json", "{\"success\":false}");
  }
}

void handleGallery() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>R2D2 CAM - Photo Gallery</title>";
  html += "<style>";
  html += "body{font-family:'Courier New',monospace;text-align:center;";
  html += "background:#0a1628;color:#00d4ff;padding:20px;margin:0;}";
  html += "h1{text-shadow:0 0 10px #00d4ff,0 0 20px #00d4ff;}";
  html += ".gallery{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));";
  html += "gap:20px;max-width:1200px;margin:20px auto;padding:20px;}";
  html += ".photo-card{background:rgba(0,212,255,0.1);border:2px solid #00d4ff;";
  html += "border-radius:10px;padding:15px;box-shadow:0 0 15px rgba(0,212,255,0.3);}";
  html += ".photo-card img{width:100%;border-radius:5px;margin-bottom:10px;}";
  html += ".btn{background:#00d4ff;color:#000;border:none;padding:8px 15px;";
  html += "border-radius:5px;cursor:pointer;margin:5px;font-weight:bold;}";
  html += ".btn:hover{background:#00ffff;box-shadow:0 0 15px #00d4ff;}";
  html += ".btn-danger{background:#ff4444;color:#fff;}";
  html += ".btn-danger:hover{background:#ff6666;}";
  html += ".back-btn{position:fixed;top:20px;left:20px;z-index:1000;}";
  html += ".empty{color:#666;font-size:18px;padding:50px;}";
  html += "</style></head><body>";
  html += "<a href='/' class='btn back-btn'>← Back to Stream</a>";
  html += "<h1>📸 Photo Gallery</h1>";
  html += "<div id='gallery' class='gallery'>Loading...</div>";
  html += "<script>";
  html += "function loadGallery(){";
  html += "  fetch('/list').then(r=>r.json()).then(data=>{";
  html += "    const gallery=document.getElementById('gallery');";
  html += "    if(data.photos.length===0){";
  html += "      gallery.innerHTML='<div class=\\'empty\\'>No photos captured yet.</div>';";
  html += "      return;";
  html += "    }";
  html += "    gallery.innerHTML=data.photos.map(p=>`";
  html += "      <div class=\\'photo-card\\'>";
  html += "        <img src=\\'/download?file=${p.name}\\' loading=\\'lazy\\'>";
  html += "        <div>${p.name}</div>";
  html += "        <div>${(p.size/1024).toFixed(1)} KB</div>";
  html += "        <button class=\\'btn\\' onclick=\\'downloadPhoto(\"${p.name}\")\\'>";
  html += "          ⬇ Download</button>";
  html += "        <button class=\\'btn btn-danger\\' onclick=\\'deletePhoto(\"${p.name}\")\\'>";
  html += "          ✕ Delete</button>";
  html += "      </div>";
  html += "    `).join('');";
  html += "  });";
  html += "}";
  html += "function downloadPhoto(name){";
  html += "  const a=document.createElement('a');";
  html += "  a.href=`/download?file=${name}`;";
  html += "  a.download=name;";
  html += "  a.click();";
  html += "}";
  html += "function deletePhoto(name){";
  html += "  if(confirm('Delete '+name+'?')){";
  html += "    fetch('/delete?file='+name).then(r=>r.json()).then(data=>{";
  html += "      if(data.success) loadGallery();";
  html += "    });";
  html += "  }";
  html += "}";
  html += "loadGallery();";
  html += "setInterval(loadGallery,5000);";
  html += "</script></body></html>";

  server.send(200, "text/html", html);
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>R2D2 CAM - Holographic Display</title>";

  // R2D2 Holographic CSS
  html += "<style>";
  html += "*{margin:0;padding:0;box-sizing:border-box;}";
  html += "body{";
  html += "font-family:'Courier New',monospace;";
  html += "background:#0a1628;";
  html += "color:#00d4ff;";
  html += "min-height:100vh;";
  html += "overflow:hidden;";
  html += "}";

  html += "h1{";
  html += "text-align:center;";
  html += "padding:20px;";
  html += "font-size:28px;";
  html += "text-shadow:0 0 10px #00d4ff,0 0 20px #00d4ff,0 0 30px #00d4ff;";
  html += "animation:hologramFlicker 3s infinite;";
  html += "letter-spacing:3px;";
  html += "}";

  // Camera container
  html += "#container{";
  html += "position:relative;";
  html += "max-width:640px;";
  html += "margin:20px auto;";
  html += "padding:10px;";
  html += "}";

  // Holographic image style
  html += "#cam{";
  html += "width:100%;";
  html += "border:3px solid #00d4ff;";
  html += "border-radius:10px;";
  html += "background:#000;";
  html += "display:block;";
  html += "box-shadow:";
  html += "0 0 20px rgba(0,212,255,0.5),";
  html += "inset 0 0 20px rgba(0,212,255,0.1);";
  html += "}";

  // HUD overlay
  html += "#hud{";
  html += "position:absolute;";
  html += "top:10px;left:10px;right:10px;bottom:10px;";
  html += "pointer-events:none;";
  html += "border:2px solid rgba(0,212,255,0.3);";
  html += "border-radius:10px;";
  html += "}";

  // Crosshair
  html += "#hud::before{";
  html += "content:'';";
  html += "position:absolute;";
  html += "top:50%;left:50%;";
  html += "width:40px;height:40px;";
  html += "transform:translate(-50%,-50%);";
  html += "border:2px solid rgba(0,212,255,0.6);";
  html += "border-radius:50%;";
  html += "box-shadow:0 0 10px rgba(0,212,255,0.5);";
  html += "}";

  html += "#hud::after{";
  html += "content:'+';";
  html += "position:absolute;";
  html += "top:50%;left:50%;";
  html += "transform:translate(-50%,-50%);";
  html += "color:rgba(0,212,255,0.8);";
  html += "font-size:20px;";
  html += "font-weight:bold;";
  html += "}";

  // Status bar
  html += "#statusBar{";
  html += "display:flex;";
  html += "justify-content:space-between;";
  html += "padding:15px 20px;";
  html += "background:rgba(0,212,255,0.1);";
  html += "border:2px solid #00d4ff;";
  html += "border-radius:10px;";
  html += "margin:20px auto;";
  html += "max-width:640px;";
  html += "box-shadow:0 0 15px rgba(0,212,255,0.3);";
  html += "}";

  html += ".status-item{";
  html += "text-align:center;";
  html += "font-size:14px;";
  html += "text-shadow:0 0 5px #00d4ff;";
  html += "}";

  html += ".status-value{";
  html += "font-size:20px;";
  html += "font-weight:bold;";
  html += "color:#00ffff;";
  html += "}";

  // Motion indicator
  html += "#motionIndicator{";
  html += "padding:10px 20px;";
  html += "margin:20px auto;";
  html += "max-width:640px;";
  html += "border:2px solid #00d4ff;";
  html += "border-radius:10px;";
  html += "background:rgba(0,212,255,0.1);";
  html += "text-align:center;";
  html += "transition:all 0.3s;";
  html += "}";

  html += "#motionIndicator.active{";
  html += "border-color:#ff4444;";
  html += "background:rgba(255,68,68,0.2);";
  html += "box-shadow:0 0 20px rgba(255,68,68,0.5);";
  html += "animation:pulse 0.5s infinite;";
  html += "}";

  html += "@keyframes pulse{";
  html += "0%,100%{transform:scale(1);}";
  html += "50%{transform:scale(1.02);}";
  html += "}";

  // Control buttons
  html += "#controls{";
  html += "display:flex;";
  html += "justify-content:center;";
  html += "gap:15px;";
  html += "padding:20px;";
  html += "}";

  html += ".btn{";
  html += "background:linear-gradient(180deg,#00d4ff,#0088aa);";
  html += "color:#000;";
  html += "border:2px solid #00ffff;";
  html += "padding:12px 25px;";
  html += "border-radius:8px;";
  html += "cursor:pointer;";
  html += "font-family:'Courier New',monospace;";
  html += "font-size:16px;";
  html += "font-weight:bold;";
  html += "text-shadow:0 0 5px rgba(0,255,255,0.5);";
  html += "box-shadow:0 0 15px rgba(0,212,255,0.4);";
  html += "transition:all 0.3s;";
  html += "}";

  html += ".btn:hover{";
  html += "background:linear-gradient(180deg,#00ffff,#00aacc);";
  html += "box-shadow:0 0 25px rgba(0,212,255,0.8);";
  html += "transform:scale(1.05);";
  html += "}";

  html += ".btn:active{";
  html += "transform:scale(0.98);";
  html += "}";

  // Timestamp
  html += "#timestamp{";
  html += "text-align:center;";
  html += "font-size:14px;";
  html += "padding:10px;";
  html += "opacity:0.8;";
  html += "}";

  html += "</style>";

  // JavaScript
  html += "<script>";
  html += "var loading = false;";
  html += "var motionData = {detected:false,level:0,threshold:15};";
  html += "var connectionLost = false;";

  html += "function refreshImage(){";
  html += "  if(loading) return;";
  html += "  loading = true;";
  html += "  var img = document.getElementById('cam');";
  html += "  var statusDiv = document.getElementById('connectionStatus');";
  html += "  var newImg = new Image();";
  html += "  newImg.onload = function(){";
  html += "    img.src = this.src;";
  html += "    loading = false;";
  html += "    if(connectionLost){";
  html += "      connectionLost = false;";
  html += "      statusDiv.textContent = 'CONNECTED';";
  html += "      statusDiv.style.color = '#00ffff';";
  html += "    }";
  html += "  };";
  html += "  newImg.onerror = function(){";
  html += "    loading = false;";
  html += "    if(!connectionLost){";
  html += "      connectionLost = true;";
  html += "      statusDiv.textContent = 'CONNECTION LOST - Retrying...';";
  html += "      statusDiv.style.color = '#ff4444';";
  html += "    }";
  html += "  };";
  html += "  newImg.src = '/stream?' + new Date().getTime();";
  html += "}";

  html += "function updateMotion(){";
  html += "  fetch('/motion').then(r=>r.json()).then(data=>{";
  html += "    motionData = data;";
  html += "    var indicator = document.getElementById('motionIndicator');";
  html += "    var motionLevel = document.getElementById('motionLevel');";
  html += "    var motionStatus = document.getElementById('motionStatus');";

  html += "    motionLevel.textContent = data.motionLevel.toFixed(1) + '%';";

  html += "    if(data.motionDetected){";
  html += "      indicator.classList.add('active');";
  html += "      motionStatus.textContent = '⚠ MOTION DETECTED';";
  html += "    } else {";
  html += "      indicator.classList.remove('active');";
  html += "      motionStatus.textContent = 'Scanning...';";
  html += "    }";
  html += "  }).catch(err=>{console.error('Motion update failed:',err);});";
  html += "}";

  html += "function capturePhoto(){";
  html += "  fetch('/capture').then(r=>r.json()).then(data=>{";
  html += "    if(data.success){";
  html += "      alert('Photo saved!\\n' + data.filename + '\\n' + (data.size/1024).toFixed(1) + ' KB');";
  html += "      document.getElementById('photoCount').textContent = data.count;";
  html += "    } else {";
  html += "      alert('Failed to capture photo: ' + data.error);";
  html += "    }";
  html += "  });";
  html += "}";

  html += "function updateTimestamp(){";
  html += "  var now = new Date();";
  html += "  var imperial = 'Imperial Date: ' + ";
  html += "    now.getFullYear() + '.' + ";
  html += "    String(now.getMonth()+1).padStart(2,'0') + '.' + ";
  html += "    String(now.getDate()).padStart(2,'0') + ' - ' + ";
  html += "    String(now.getHours()).padStart(2,'0') + ':' + ";
  html += "    String(now.getMinutes()).padStart(2,'0') + ':' + ";
  html += "    String(now.getSeconds()).padStart(2,'0');";
  html += "  document.getElementById('timestamp').textContent = imperial;";
  html += "}";

  html += "setInterval(refreshImage, 30);";
  html += "setInterval(updateMotion, 100);";
  html += "setInterval(updateTimestamp, 1000);";
  html += "refreshImage();";
  html += "updateTimestamp();";
  html += "</script>";

  // HTML body
  html += "</head><body>";
  html += "<h1>🤖 R2D2 CAM - HOLOGRAPHIC DISPLAY</h1>";

  html += "<div id='statusBar'>";
  html += "<div class='status-item'><div>FPS</div><div class='status-value'>~30</div></div>";
  html += "<div class='status-item'><div>Photos</div><div class='status-value' id='photoCount'>" + String(photoCount) + "</div></div>";
  html += "<div class='status-item'><div>Status</div><div class='status-value' id='connectionStatus'>CONNECTED</div></div>";
  html += "</div>";

  html += "<div id='container'>";
  html += "<img id='cam' src='/stream' alt='R2D2 Camera Stream' />";
  html += "<div id='hud'></div>";
  html += "</div>";

  html += "<div id='motionIndicator'>";
  html += "<div style='font-size:18px;font-weight:bold;margin-bottom:5px;' id='motionStatus'>Scanning...</div>";
  html += "<div>Motion Level: <span id='motionLevel'>0.0</span>% (Threshold: " + String(motionThreshold) + "%)</div>";
  html += "</div>";

  html += "<div id='controls'>";
  html += "<button class='btn' onclick='capturePhoto()'>📸 Capture Photo</button>";
  html += "<a href='/gallery' class='btn'>🖼 Photo Gallery</a>";
  html += "</div>";

  html += "<div id='timestamp'></div>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}
