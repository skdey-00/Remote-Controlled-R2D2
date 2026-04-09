/*
 * R2D2 WiFi Control System
 * Controls 2 BO 300RPM motors via L298N driver
 * Web interface hosted on ESP32 access point
 */

#include <WiFi.h>
#include <WebServer.h>

// Motor Pin Definitions - L298N Driver
// Motor A (Left)
const int ENA = 14;  // Enable/speed motor A (PWM)
const int IN1 = 27;  // Direction 1
const int IN2 = 26;  // Direction 2

// Motor B (Right)
const int ENB = 32;  // Enable/speed motor B (PWM)
const int IN3 = 25;  // Direction 1
const int IN4 = 33;  // Direction 2

// PWM settings for speed control
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;  // 8-bit = 0-255
const int PWM_CHANNEL_A = 0;
const int PWM_CHANNEL_B = 1;

// WiFi Settings
const char* AP_SSID = "R2D2_Control";
const char* AP_PASSWORD = "astromech";

WebServer server(80);

// Motor state variables
int currentSpeed = 150;  // Default speed (0-255)
String currentDirection = "stop";

// Simplified HTML page
void handleRoot() {
  String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>R2D2 Control</title>";
  html += "<style>*{margin:0;padding:0;box-sizing:border-box}";
  html += "body{font-family:Arial,sans-serif;background:#1a1a2e;min-height:100vh;";
  html += "display:flex;flex-direction:column;align-items:center;padding:20px;color:#fff}";
  html += "h1{color:#4FC3F7;text-align:center;margin-bottom:20px}";
  html += ".status{background:rgba(79,195,247,0.1);border:1px solid #4FC3F7;";
  html += "border-radius:10px;padding:15px 30px;margin-bottom:20px;text-align:center}";
  html += ".status-value{color:#4FC3F7;font-size:1.5rem;font-weight:bold}";
  html += ".dpad{display:grid;grid-template-columns:repeat(3,70px);gap:5px;margin-bottom:20px}";
  html += ".dpad-btn{background:#2a2a4a;border:2px solid #4FC3F7;border-radius:10px;";
  html += "color:#4FC3F7;font-size:1.8rem;cursor:pointer;padding:20px;text-align:center}";
  html += ".dpad-btn:active,.dpad-btn.active{background:#4FC3F7;color:#1a1a2e}";
  html += ".dpad-btn.stop{background:#4a1a1a;border-color:#f44336;color:#f44336}";
  html += ".speed-control{text-align:center;margin-top:20px}";
  html += ".speed-slider{width:100%;max-width:300px;height:10px;border-radius:5px}";
  html += ".speed-value{color:#4FC3F7;font-size:1.3rem;margin-top:10px}";
  html += ".speed-btn{background:rgba(79,195,247,0.1);border:1px solid #4FC3F7;";
  html += "color:#4FC3F7;padding:8px 15px;border-radius:15px;cursor:pointer;margin:5px}";
  html += ".speed-btn.active{background:#4FC3F7;color:#1a1a2e}";
  html += "@media(max-width:500px){.dpad{grid-template-columns:repeat(3,60px)}";
  html += ".dpad-btn{font-size:1.5rem;padding:15px}}</style></head><body>";
  html += "<h1>🤖 R2D2 Control</h1>";
  html += "<div class='status'><div class='status-value' id='status'>STOPPED</div></div>";
  html += "<div class='dpad'>";
  html += "<button class='dpad-btn' style='grid-column:2' ontouchstart=\"send('forward')\"";
  html += " onmousedown=\"send('forward')\" onmouseup=\"send('stop')\"";
  html += " onmouseleave=\"send('stop')\" ontouchend=\"send('stop')\">▲</button>";
  html += "<button class='dpad-btn' style='grid-column:1' ontouchstart=\"send('left')\"";
  html += " onmousedown=\"send('left')\" onmouseup=\"send('stop')\"";
  html += " onmouseleave=\"send('stop')\" ontouchend=\"send('stop')\">◀</button>";
  html += "<button class='dpad-btn stop' style='grid-column:2' onclick=\"send('stop')\"";
  html += " ontouchstart=\"send('stop')\">STOP</button>";
  html += "<button class='dpad-btn' style='grid-column:3' ontouchstart=\"send('right')\"";
  html += " onmousedown=\"send('right')\" onmouseup=\"send('stop')\"";
  html += " onmouseleave=\"send('stop')\" ontouchend=\"send('stop')\">▶</button>";
  html += "<button class='dpad-btn' style='grid-column:2' ontouchstart=\"send('backward')\"";
  html += " onmousedown=\"send('backward')\" onmouseup=\"send('stop')\"";
  html += " onmouseleave=\"send('stop')\" ontouchend=\"send('stop')\">▼</button>";
  html += "</div>";
  html += "<div class='speed-control'>";
  html += "<input type='range' class='speed-slider' id='speedSlider' min='0' max='255' value='150'";
  html += " oninput='updateSpeed(this.value)'>";
  html += "<div class='speed-value'>Speed: <span id='speedValue'>150</span></div>";
  html += "<button class='speed-btn' onclick='setSpeed(100)'>Slow</button>";
  html += "<button class='speed-btn active' onclick='setSpeed(150)'>Normal</button>";
  html += "<button class='speed-btn' onclick='setSpeed(200)'>Fast</button>";
  html += "<button class='speed-btn' onclick='setSpeed(255)'>Max</button>";
  html += "</div>";
  html += "<script>let speed=150;function send(d){fetch('/control?dir='+d+'&speed='+speed);";
  html += "document.getElementById('status').textContent=d.toUpperCase();";
  html += "if(d=='stop')document.getElementById('status').style.color='#f44336';";
  html += "else document.getElementById('status').style.color='#4FC3F7'}";
  html += "function updateSpeed(v){speed=v;document.getElementById('speedValue').textContent=v}";
  html += "function setSpeed(v){speed=v;document.getElementById('speedSlider').value=v;";
  html += "document.getElementById('speedValue').textContent=v;";
  html += "document.querySelectorAll('.speed-btn').forEach(b=>b.classList.remove('active'));";
  html += "event.target.classList.add('active')}";
  html += "document.addEventListener('keydown',e=>{if(e.repeat)return;";
  html += "if(e.key=='ArrowUp'||e.key=='w')send('forward');";
  html += "else if(e.key=='ArrowDown'||e.key=='s')send('backward');";
  html += "else if(e.key=='ArrowLeft'||e.key=='a')send('left');";
  html += "else if(e.key=='ArrowRight'||e.key=='d')send('right');";
  html += "else if(e.key==' ')send('stop')});";
  html += "document.addEventListener('keyup',e=>{if(['ArrowUp','ArrowDown','w','s',";
  html += "'ArrowLeft','ArrowRight','a','d'].includes(e.key))send('stop')})</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleControl() {
  String direction = server.arg("dir");
  int speed = server.arg("speed").toInt();

  if (speed < 0) speed = 0;
  if (speed > 255) speed = 255;

  Serial.println("Command: " + direction + " | Speed: " + String(speed));

  executeCommand(direction, speed);
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=================================");
  Serial.println("R2D2 WiFi Control System");
  Serial.println("=================================\n");

  // Initialize motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Initialize PWM for motor speed control
  // Using ESP32 LEDC
  ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(ENA, PWM_CHANNEL_A);
  ledcAttachPin(ENB, PWM_CHANNEL_B);

  // Initialize motors to stopped state
  stopMotors();
  Serial.println("Motor pins initialized");

  // Setup WiFi Access Point
  Serial.print("Starting Access Point...");
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress IP = WiFi.softAPIP();
  Serial.println(" Done!");
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/control", handleControl);

  // Start server
  server.begin();
  Serial.println("HTTP server started");
  Serial.println("\n=================================");
  Serial.println("READY TO CONNECT!");
  Serial.println("=================================");
  Serial.print("WiFi SSID: ");
  Serial.println(AP_SSID);
  Serial.print("Password: ");
  Serial.println(AP_PASSWORD);
  Serial.print("Open browser: http://");
  Serial.println(IP);
  Serial.println("=================================\n");
}

void loop() {
  server.handleClient();
  delay(2);
}

// Motor control functions
void executeCommand(String direction, int speed) {
  currentSpeed = speed;
  currentDirection = direction;

  // Stop all motors first
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  if (direction == "stop") {
    ledcWrite(PWM_CHANNEL_A, 0);
    ledcWrite(PWM_CHANNEL_B, 0);
    return;
  }

  if (direction == "forward") {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    ledcWrite(PWM_CHANNEL_A, speed);
    ledcWrite(PWM_CHANNEL_B, speed);
  }
  else if (direction == "backward") {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    ledcWrite(PWM_CHANNEL_A, speed);
    ledcWrite(PWM_CHANNEL_B, speed);
  }
  else if (direction == "left") {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    ledcWrite(PWM_CHANNEL_A, speed);
    ledcWrite(PWM_CHANNEL_B, speed);
  }
  else if (direction == "right") {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    ledcWrite(PWM_CHANNEL_A, speed);
    ledcWrite(PWM_CHANNEL_B, speed);
  }
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  ledcWrite(PWM_CHANNEL_A, 0);
  ledcWrite(PWM_CHANNEL_B, 0);
}
