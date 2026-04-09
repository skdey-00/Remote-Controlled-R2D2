# R2D2 WiFi Control System

Control your R2D2 robot wirelessly using a web browser on your phone, tablet, or computer!

## Hardware Required

- ESP32 Board
- L298N Motor Driver
- 2x BO 300RPM Motors (or any DC motors)
- Battery pack (recommended: 2x 18650 or 7.4V LiPo)
- Jumper wires

## Wiring Guide

### ESP32 to L298N Motor Driver

```
ESP32 Pin          →  L298N Pin
─────────────────────────────────
GPIO 14 (ENA)      →  ENA (Enable A)
GPIO 27 (IN1)      →  IN1
GPIO 26 (IN2)      →  IN2
GPIO 32 (ENB)      →  ENB (Enable B)
GPIO 25 (IN3)      →  IN3
GPIO 33 (IN4)      →  IN4
GND                →  GND
```

### L298N to Motors

```
Motor A (Left)     →  OUT1 & OUT2
Motor B (Right)    →  OUT3 & OUT4
```

### Power

```
Battery Positive   →  L298N +12V
Battery Negative   →  L298N GND
ESP32 VIN/5V       →  L298N +5V (if using 12V+)
                  OR
ESP32 5V           →  L298N +5V output
```

## Pin Configuration

| Function | ESP32 Pin | L298N Pin |
|----------|-----------|-----------|
| ENA | 14 | Enable A |
| IN1 | 27 | IN1 |
| IN2 | 26 | IN2 |
| ENB | 32 | Enable B |
| IN3 | 25 | IN3 |
| IN4 | 33 | IN4 |

## Setup Instructions

### 1. Install ESP32 Board in Arduino IDE

1. Open Arduino IDE
2. Go to File → Preferences
3. Add this URL to "Additional Board Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Go to Tools → Board → Boards Manager
5. Search for "ESP32" and install

### 2. Upload the Code

1. Connect ESP32 to your computer via USB
2. Open `R2D2_WiFi_Control.ino`
3. Select your board: Tools → Board → ESP32 Arduino → Your ESP32 board
4. Select correct COM port
5. Click Upload

### 3. Connect and Control

1. Open Serial Monitor (115200 baud)
2. Note the IP address shown (usually 192.168.4.1)
3. On your phone/tablet, connect to WiFi: **"R2D2_Control"**
4. Password: **"astromech"**
5. Open browser and go to: `http://192.168.4.1`
6. Start driving!

## Web Interface Controls

### Direction Pad
- **▲** - Move forward
- **▼** - Move backward
- **◀** - Turn left (spin in place)
- **▶** - Turn right (spin in place)
- **STOP** - Emergency stop (big red button)
- **Diagonal buttons** - Forward/backward with turning

### Speed Control
- Use the slider (0-255)
- Quick buttons: Slow, Normal, Fast, Max

### Keyboard Controls (Desktop)
- **W / ↑** - Forward
- **S / ↓** - Backward
- **A / ←** - Turn left
- **D / →** - Turn right
- **Space** - Stop

## Troubleshooting

### Motors not spinning?
- Check wiring connections
- Verify battery is charged
- Check that L298N has power (LED should be on)
- Try increasing speed using the slider

### ESP32 not connecting?
- Make sure ESP32 is powered properly
- Try resetting the ESP32
- Check Serial Monitor for error messages

### Robot going wrong direction?
- Swap the motor wires on OUT1/OUT2 or OUT3/OUT4
- Or modify the `moveForward()` function to reverse IN1/IN2

### WiFi not showing up?
- Wait 30 seconds after powering on
- Reset ESP32
- Check Serial Monitor for confirmation

## Customization

### Change WiFi Name/Password
Edit these lines in the sketch:
```cpp
const char* AP_SSID = "R2D2_Control";
const char* AP_PASSWORD = "astromech";
```

### Adjust Motor Directions
If your robot goes backward when you press forward:
1. Swap the wires on OUT1 and OUT2, OR
2. Edit `moveForward()` function to swap IN1/IN2

## Safety Notes

- Always start with low speed and test
- Keep fingers away from moving parts
- Use appropriate battery for your motors
- Robot may drift - calibrate by adjusting PWM in turn functions

## Features

- 🌐 Local WiFi access point (no internet needed)
- 📱 Mobile-friendly responsive design
- ⌨️ Keyboard support for desktop
- 🎮 D-pad style control interface
- ⚡ Variable speed control (0-255 PWM)
- 🔄 Diagonal movement support
- 🛑 Emergency stop button
- ✨ Cool R2D2 themed design
