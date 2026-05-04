/*
 * R2D2 WiFi Control - With SYNTHESIZED Sounds
 *
 * This version generates R2D2-like sounds using Web Audio API
 * NO audio files needed - sounds are created by code!
 *
 * FEATURES:
 * - D-Pad + Joystick control
 * - Synthesized R2D2 sounds (beeps, chirps, whistles)
 * - Auto-play sounds on events
 * - Works offline, very small code size
 */

#include <WiFi.h>
#include <WebServer.h>

// Motor Pins
const int ENA = 14, IN1 = 27, IN2 = 26;
const int ENB = 32, IN3 = 25, IN4 = 33;

// PWM
const int PWM_FREQ = 5000, PWM_RESOLUTION = 8;
const int PWM_CHANNEL_A = 0, PWM_CHANNEL_B = 1;

// WiFi
const char* AP_SSID = "R2D2_Control";
const char* AP_PASSWORD = "astromech";

WebServer server(80);
int currentSpeed = 150;
String currentDirection = "stop";

void handleRoot() {
  String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0, user-scalable=no'>";
  html += "<title>R2D2 Control</title>";
  html += "<style>*{margin:0;padding:0;box-sizing:border-box;-webkit-user-select:none;user-select:none}";
  html += "body{font-family:Arial,sans-serif;background:#1a1a2e;min-height:100vh;display:flex;flex-direction:column;align-items:center;padding:10px;color:#fff;padding-bottom:80px}";
  html += "h1{color:#4FC3F7;text-align:center;margin-bottom:10px;font-size:1.3rem}";
  html += ".mode-switch{display:flex;gap:8px;margin-bottom:10px;background:rgba(79,195,247,0.1);padding:4px;border-radius:20px}";
  html += ".mode-btn{flex:1;padding:8px 16px;border:none;border-radius:16px;background:transparent;color:#4FC3F7;cursor:pointer;font-size:0.8rem;transition:all 0.3s}";
  html += ".mode-btn.active{background:#4FC3F7;color:#1a1a2e}";
  html += ".status{background:rgba(79,195,247,0.1);border:1px solid #4FC3F7;border-radius:8px;padding:8px 16px;margin-bottom:10px;text-align:center}";
  html += ".status-value{color:#4FC3F7;font-size:1.1rem;font-weight:bold}";
  html += ".control-panel{width:100%;max-width:320px}";
  html += ".dpad-container{display:flex;flex-direction:column;align-items:center;gap:4px}";
  html += ".dpad-row{display:flex;gap:4px}";
  html += ".dpad-btn{width:60px;height:60px;background:#2a2a4a;border:2px solid #4FC3F7;border-radius:10px;cursor:pointer;display:flex;align-items:center;justify-content:center;transition:all 0.1s}";
  html += ".dpad-btn svg{width:30px;height:30px;fill:#4FC3F7}";
  html += ".dpad-btn:active,.dpad-btn.active{background:#4FC3F7;transform:scale(0.95)}";
  html += ".dpad-btn:active svg,.dpad-btn.active svg{fill:#1a1a2e}";
  html += ".dpad-btn.stop{background:#4a1a1a;border-color:#f44336}";
  html += ".dpad-btn.stop svg{fill:#f44336}";
  html += ".joystick-container{display:none;position:relative;width:180px;height:180px;margin:15px auto}";
  html += ".joystick-base{width:180px;height:180px;background:rgba(79,195,247,0.15);border:3px solid #4FC3F7;border-radius:50%;position:relative;touch-action:none}";
  html += ".joystick-base::before{content:'';position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);width:30px;height:30px;background:rgba(79,195,247,0.3);border-radius:50%}";
  html += ".joystick-stick{width:65px;height:65px;background:linear-gradient(145deg,#5BC0DE,#4FC3F7);border-radius:50%;position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);cursor:pointer;box-shadow:0 4px 20px rgba(79,195,247,0.6);transition:transform 0.15s ease-out}";
  html += ".joystick-stick.active{background:linear-gradient(145deg,#4FC3F7,#3AA8CE)}";
  html += ".joystick-direction{position:absolute;top:-25px;left:50%;transform:translateX(-50%);color:#4FC3F7;font-size:0.8rem;opacity:0;transition:opacity 0.2s}";
  html += ".joystick-direction.show{opacity:1}";
  // Soundboard
  html += ".soundboard{position:fixed;bottom:0;left:0;right:0;background:#1a1a2e;border-top:2px solid #4FC3F7;padding:8px;z-index:100}";
  html += ".soundboard-title{color:#4FC3F7;font-size:0.8rem;text-align:center;margin-bottom:6px}";
  html += ".sound-btns{display:flex;gap:6px;justify-content:center;flex-wrap:wrap}";
  html += ".sound-btn{background:#2a2a4a;border:1px solid #4FC3F7;color:#4FC3F7;padding:8px 12px;border-radius:20px;cursor:pointer;font-size:0.75rem;min-width:70px}";
  html += ".sound-btn:active{background:#4FC3F7;color:#1a1a2e;transform:scale(0.95)}";
  html += ".sound-icon{font-size:1rem;display:block}";
  html += ".speed-control{text-align:center;margin-top:10px}";
  html += ".speed-slider{-webkit-appearance:none;width:100%;max-width:280px;height:6px;border-radius:3px;background:rgba(79,195,247,0.3)}";
  html += ".speed-slider::-webkit-slider-thumb{-webkit-appearance:none;width:20px;height:20px;border-radius:50%;background:#4FC3F7;cursor:pointer}";
  html += ".speed-value{color:#4FC3F7;font-size:1rem;margin-top:6px}";
  html += ".speed-btns{display:flex;justify-content:center;gap:6px;margin-top:8px;flex-wrap:wrap}";
  html += ".speed-btn{background:rgba(79,195,247,0.1);border:1px solid #4FC3F7;color:#4FC3F7;padding:6px 12px;border-radius:12px;cursor:pointer;font-size:0.75rem}";
  html += ".speed-btn.active{background:#4FC3F7;color:#1a1a2e}";
  html += "</style></head><body>";

  html += "<h1>🤖 R2D2 Control</h1>";
  html += "<div class='mode-switch'>";
  html += "<button class='mode-btn active' id='dpadModeBtn' onclick='switchMode(\"dpad\")'>D-PAD</button>";
  html += "<button class='mode-btn' id='joystickModeBtn' onclick='switchMode(\"joystick\")'>JOYSTICK</button>";
  html += "</div>";
  html += "<div class='status'><div class='status-value' id='status'>STOPPED</div></div>";
  html += "<div class='control-panel'>";

  // D-Pad
  html += "<div class='dpad-container' id='dpadContainer'>";
  html += "<div class='dpad-row'><button class='dpad-btn' id='btn-forward' ontouchstart=\"startPress('forward')\" onmousedown=\"startPress('forward')\" ontouchend=\"endPress()\" onmouseup=\"endPress()\" onmouseleave=\"endPress()\"><svg viewBox='0 0 24 24'><path d='M12 4l-8 8h16l-8-8z'/></svg></button></div>";
  html += "<div class='dpad-row'>";
  html += "<button class='dpad-btn' id='btn-left' ontouchstart=\"startPress('left')\" onmousedown=\"startPress('left')\" ontouchend=\"endPress()\" onmouseup=\"endPress()\" onmouseleave=\"endPress()\"><svg viewBox='0 0 24 24'><path d='M4 12l8-8v16l-8-8z'/></svg></button>";
  html += "<button class='dpad-btn stop' id='btn-stop' onclick=\"send('stop');updateStatus('stop')\"><svg viewBox='0 0 24 24'><rect x='6' y='6' width='12' height='12' rx='2'/></svg></button>";
  html += "<button class='dpad-btn' id='btn-right' ontouchstart=\"startPress('right')\" onmousedown=\"startPress('right')\" ontouchend=\"endPress()\" onmouseup=\"endPress()\" onmouseleave=\"endPress()\"><svg viewBox='0 0 24 24'><path d='M20 12l-8 8V4l8 8z'/></svg></button>";
  html += "</div>";
  html += "<div class='dpad-row'><button class='dpad-btn' id='btn-backward' ontouchstart=\"startPress('backward')\" onmousedown=\"startPress('backward')\" ontouchend=\"endPress()\" onmouseup=\"endPress()\" onmouseleave=\"endPress()\"><svg viewBox='0 0 24 24'><path d='M12 20l8-8H4l8 8z'/></svg></button></div>";
  html += "</div>";

  // Joystick
  html += "<div class='joystick-container' id='joystickContainer'>";
  html += "<div class='joystick-direction' id='joyDirection'></div>";
  html += "<div class='joystick-base' id='joystickBase'><div class='joystick-stick' id='joystickStick'></div></div>";
  html += "</div></div>";

  // Speed
  html += "<div class='speed-control'>";
  html += "<input type='range' class='speed-slider' id='speedSlider' min='0' max='255' value='150' oninput='updateSpeed(this.value)'>";
  html += "<div class='speed-value'>Speed: <span id='speedValue'>150</span></div>";
  html += "<div class='speed-btns'>";
  html += "<button class='speed-btn' onclick='setSpeed(100)'>Slow</button>";
  html += "<button class='speed-btn active' onclick='setSpeed(150)'>Normal</button>";
  html += "<button class='speed-btn' onclick='setSpeed(200)'>Fast</button>";
  html += "<button class='speed-btn' onclick='setSpeed(255)'>Max</button>";
  html += "</div></div>";

  // Soundboard
  html += "<div class='soundboard'>";
  html += "<div class='soundboard-title'>🔊 SOUNDBARD</div>";
  html += "<div class='sound-btns'>";
  html += "<button class='sound-btn' onclick='playSound(\"boot\")'><span class='sound-icon'>🚀</span>Boot</button>";
  html += "<button class='sound-btn' onclick='playSound(\"excited\")'><span class='sound-icon'>😄</span>Excited</button>";
  html += "<button class='sound-btn' onclick='playSound(\"standby\")'><span class='sound-icon'>😊</span>Standby</button>";
  html += "<button class='sound-btn' onclick='playSound(\"happy\")'><span class='sound-icon'>✨</span>Happy</button>";
  html += "<button class='sound-btn' onclick='playSound(\"sad\")'><span class='sound-icon'>😢</span>Sad</button>";
  html += "<button class='sound-btn' onclick='playSound(\"alert\")'><span class='sound-icon'>⚠️</span>Alert</button>";
  html += "</div></div>";

  // JavaScript with R2D2 Sound Synthesizer
  html += "<script>let speed=150,pressTimer=null,currentDir='',joyActive=false,lastCmd='',lastCmdTime=0,wasMoving=false,audioCtx=null;";
  html += "const CMD_DELAY=80,base=document.getElementById('joystickBase'),stick=document.getElementById('joystickStick'),dirLabel=document.getElementById('joyDirection');";
  html += "let joyX=0,joyY=0,baseRect=null;";

  // R2D2 Sound Synthesizer - generates authentic sounds without files!
  html += "function initAudio(){if(!audioCtx)audioCtx=new(window.AudioContext||window.webkitAudioContext)();if(audioCtx.state==='suspended')audioCtx.resume()}";

  // Sound generators
  html += "function playSound(type){initAudio();";
  html += "const osc=audioCtx.createOscillator(),gain=audioCtx.createGain();";
  html += "osc.connect(gain);gain.connect(audioCtx.destination);";
  html += "const now=audioCtx.currentTime;";

  // Different sound types
  html += "switch(type){";

  // Boot - rising chirp sequence
  html += "case'boot':for(let i=0;i<5;i++){const o=audioCtx.createOscillator(),g=audioCtx.createGain();o.connect(g);g.connect(audioCtx.destination);o.frequency.setValueAtTime(800+i*400,now+i*0.15);o.frequency.exponentialRampToValueAtTime(1200+i*400,now+i*0.15+0.1);g.gain.setValueAtTime(0.3,now+i*0.15);g.gain.exponentialRampToValueAtTime(0.01,now+i*0.15+0.15);o.type='square';o.start(now+i*0.15);o.stop(now+i*0.15+0.15)}break;";

  // Excited - rapid warbling
  html += "case'excited':for(let i=0;i<8;i++){const o=audioCtx.createOscillator(),g=audioCtx.createGain();o.connect(g);g.connect(audioCtx.destination);o.frequency.setValueAtTime(1000+Math.random()*500,now+i*0.08);o.frequency.linearRampToValueAtTime(1500+Math.random()*500,now+i*0.08+0.04);g.gain.setValueAtTime(0.2,now+i*0.08);g.gain.exponentialRampToValueAtTime(0.01,now+i*0.08+0.06);o.type='sawtooth';o.start(now+i*0.08);o.stop(now+i*0.08+0.06)}break;";

  // Standby - gentle humming
  html += "case'standby':osc.frequency.setValueAtTime(200,now);osc.frequency.exponentialRampToValueAtTime(150,now+0.5);gain.gain.setValueAtTime(0.15,now);gain.gain.exponentialRampToValueAtTime(0.01,now+0.5);osc.type='sine';osc.start(now);osc.stop(now+0.5);break;";

  // Happy - ascending melody
  html += "case'happy':for(let i=0;i<4;i++){const o=audioCtx.createOscillator(),g=audioCtx.createGain();o.connect(g);g.connect(audioCtx.destination);o.frequency.setValueAtTime(600+i*200,now+i*0.1);gain.gain.setValueAtTime(0.25,now+i*0.1);g.gain.exponentialRampToValueAtTime(0.01,now+i*0.1+0.1);o.type='square';o.start(now+i*0.1);o.stop(now+i*0.1+0.1)}break;";

  // Sad - descending tones
  html += "case'sad':for(let i=0;i<3;i++){const o=audioCtx.createOscillator(),g=audioCtx.createGain();o.connect(g);g.connect(audioCtx.destination);o.frequency.setValueAtTime(800-i*200,now+i*0.2);o.frequency.exponentialRampToValueAtTime(400-i*100,now+i*0.2+0.15);g.gain.setValueAtTime(0.2,now+i*0.2);g.gain.exponentialRampToValueAtTime(0.01,now+i*0.2+0.2);o.type='sawtooth';o.start(now+i*0.2);o.stop(now+i*0.2+0.2)}break;";

  // Alert - sharp beeps
  html += "case'alert':for(let i=0;i<3;i++){const o=audioCtx.createOscillator(),g=audioCtx.createGain();o.connect(g);g.connect(audioCtx.destination);o.frequency.setValueAtTime(1200,now+i*0.15);o.frequency.exponentialRampToValueAtTime(800,now+i*0.15+0.08);g.gain.setValueAtTime(0.3,now+i*0.15);g.gain.exponentialRampToValueAtTime(0.01,now+i*0.15+0.1);o.type='square';o.start(now+i*0.15);o.stop(now+i*0.15+0.1)}break;}";

  html += "}";

  // Control functions
  html += "function send(d){const now=Date.now(),isMoving=d!=='stop';if(d===lastCmd&&now-lastCmdTime<CMD_DELAY)return;lastCmd=d;lastCmdTime=now;fetch('/control?dir='+d+'&speed='+speed);updateStatus(d);if(isMoving&&!wasMoving)playSound('excited');if(d==='stop'&&wasMoving)playSound('standby');wasMoving=isMoving}";

  html += "function updateStatus(d){const s=document.getElementById('status');s.textContent=d.toUpperCase();s.style.color=d=='stop'?'#f44336':'#4FC3F7'}";
  html += "function startPress(dir){if(pressTimer)return;currentDir=dir;document.getElementById('btn-'+dir).classList.add('active');send(dir)}";
  html += "function endPress(){if(pressTimer){clearTimeout(pressTimer);pressTimer=null}document.querySelectorAll('.dpad-btn').forEach(b=>b.classList.remove('active'));if(currentDir&&currentDir!='stop'){send('stop');currentDir=''}}";
  html += "function updateSpeed(v){speed=v;document.getElementById('speedValue').textContent=v}";
  html += "function setSpeed(v){speed=v;document.getElementById('speedSlider').value=v;document.getElementById('speedValue').textContent=v;document.querySelectorAll('.speed-btn').forEach(b=>b.classList.remove('active'));event.target.classList.add('active')}";

  html += "function switchMode(m){document.getElementById('dpadContainer').style.display=m=='dpad'?'flex':'none';document.getElementById('joystickContainer').style.display=m=='joystick'?'block':'none';document.getElementById('dpadModeBtn').classList.toggle('active',m=='dpad');document.getElementById('joystickModeBtn').classList.toggle('active',m=='joystick');if(m=='dpad'){send('stop');resetJoystick()}}";
  html += "function resetJoystick(){joyActive=false;joyX=0;joyY=0;stick.classList.remove('active');stick.style.transform='translate(-50%,-50%)';dirLabel.classList.remove('show');send('stop');updateStatus('stop')}";

  html += "function updateJoystickPosition(clientX,clientY){if(!baseRect)baseRect=base.getBoundingClientRect();const radius=baseRect.width/2,centerX=baseRect.left+radius,centerY=baseRect.top+radius,maxDist=radius-32;let dx=clientX-centerX,dy=clientY-centerY,dist=Math.sqrt(dx*dx+dy*dy);if(dist>maxDist){dx=dx/dist*maxDist;dy=dy/dist*maxDist}stick.style.transform='translate(calc(-50%+'+dx.toFixed(1)+'px),calc(-50%+'+dy.toFixed(1)+'px))';joyX=dx/maxDist;joyY=dy/maxDist;updateJoystick()}";

  html += "function updateJoystick(){const deadzone=0.12,threshold=0.25;if(Math.abs(joyX)<deadzone&&Math.abs(joyY)<deadzone){send('stop');dirLabel.classList.remove('show');return}let cmd='';if(Math.abs(joyY)>Math.abs(joyX)){if(joyY<-threshold)cmd='forward';else if(joyY>threshold)cmd='backward'}else{if(joyX<-threshold)cmd='left';else if(joyX>threshold)cmd='right'}if(cmd){dirLabel.textContent=cmd.toUpperCase();dirLabel.classList.add('show');send(cmd)}else{dirLabel.classList.remove('show')}}";

  html += "function handleJoyStart(e){e.preventDefault();e.stopPropagation();joyActive=true;stick.classList.add('active');baseRect=base.getBoundingClientRect();handleJoyMove(e)}";
  html += "function handleJoyMove(e){if(!joyActive)return;e.preventDefault();e.stopPropagation();let clientX,clientY;if(e.touches&&e.touches.length>0){clientX=e.touches[0].clientX;clientY=e.touches[0].clientY}else{clientX=e.clientX;clientY=e.clientY}updateJoystickPosition(clientX,clientY)}";
  html += "function handleJoyEnd(e){e.preventDefault();e.stopPropagation();resetJoystick()}";

  html += "base.addEventListener('touchstart',handleJoyStart,{passive:false,capture:true});base.addEventListener('touchmove',handleJoyMove,{passive:false,capture:true});base.addEventListener('touchend',handleJoyEnd,{passive:false,capture:true});base.addEventListener('touchcancel',handleJoyEnd,{passive:false,capture:true});base.addEventListener('mousedown',handleJoyStart);document.addEventListener('mousemove',handleJoyMove);document.addEventListener('mouseup',handleJoyEnd);";
  html += "document.addEventListener('keydown',e=>{if(e.repeat)return;if(e.key=='ArrowUp'||e.key=='w')send('forward');else if(e.key=='ArrowDown'||e.key=='s')send('backward');else if(e.key=='ArrowLeft'||e.key=='a')send('left');else if(e.key=='ArrowRight'||e.key=='d')send('right');else if(e.key==' ')send('stop')});";
  html += "document.addEventListener('keyup',e=>{if(['ArrowUp','ArrowDown','w','s','ArrowLeft','ArrowRight','a','d'].includes(e.key))send('stop')});";

  // Play boot sound on load and init audio on first interaction
  html += "window.addEventListener('load',()=>{setTimeout(()=>playSound('boot'),500)});";
  html += "document.addEventListener('touchstart',initAudio,{once:true});";
  html += "document.addEventListener('click',initAudio,{once:true});";

  html += "</script></body></html>";
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
  Serial.println("R2D2 WiFi Control - With Synthesized Sounds");
  Serial.println("=================================\n");

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(ENA, PWM_CHANNEL_A);
  ledcAttachPin(ENB, PWM_CHANNEL_B);
  stopMotors();
  Serial.println("Motor pins initialized");

  WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP: "); Serial.println(IP);

  server.on("/", handleRoot);
  server.on("/control", handleControl);

  server.begin();
  Serial.println("HTTP server started");
  Serial.println("\n=================================");
  Serial.println("READY TO CONNECT!");
  Serial.println("=================================");
  Serial.print("WiFi SSID: "); Serial.println(AP_SSID);
  Serial.print("Password: "); Serial.println(AP_PASSWORD);
  Serial.print("Open: http://"); Serial.println(IP);
  Serial.println("=================================\n");
}

void loop() {
  server.handleClient();
  delay(2);
}

void executeCommand(String direction, int speed) {
  currentSpeed = speed;
  currentDirection = direction;
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);

  if (direction == "stop") {
    ledcWrite(PWM_CHANNEL_A, 0); ledcWrite(PWM_CHANNEL_B, 0);
    return;
  }

  if (direction == "forward") {
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
    ledcWrite(PWM_CHANNEL_A, speed); ledcWrite(PWM_CHANNEL_B, speed);
  }
  else if (direction == "backward") {
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    ledcWrite(PWM_CHANNEL_A, speed); ledcWrite(PWM_CHANNEL_B, speed);
  }
  else if (direction == "left") {
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
    ledcWrite(PWM_CHANNEL_A, speed); ledcWrite(PWM_CHANNEL_B, speed);
  }
  else if (direction == "right") {
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
    ledcWrite(PWM_CHANNEL_A, speed); ledcWrite(PWM_CHANNEL_B, speed);
  }
}

void stopMotors() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  ledcWrite(PWM_CHANNEL_A, 0); ledcWrite(PWM_CHANNEL_B, 0);
}
