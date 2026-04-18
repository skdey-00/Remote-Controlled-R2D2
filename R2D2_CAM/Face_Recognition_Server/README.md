# R2D2 Face Recognition System

Real-time face detection and recognition using ESP32-CAM in Access Point mode - no external WiFi needed!

## Features

- **Live Video Stream** from ESP32-CAM (hosts its own WiFi network)
- **Face Detection** using state-of-the-art face_recognition library
- **Face Recognition** against your reference images
- **Web Interface** to view live processed feed with bounding boxes
- **Green Box** = "Hello [Name]" for recognized faces
- **Red Box** = "Intruder" for unknown faces

## Quick Start

### 1. Install Python Dependencies

**Windows:**
```bash
pip install dlib-bin
pip install opencv-python face_recognition Pillow Flask
```

**If dlib-bin doesn't work**, install Visual Studio C++ Build Tools first, then:
```bash
pip install cmake
pip install dlib
```

### 2. Upload Arduino Sketch to ESP32-CAM

1. Open `esp32cam_stream/esp32cam_stream.ino` in Arduino IDE
2. Select: Tools → Board → ESP32 Arduino → AI Thinker ESP32-CAM
3. Connect GPIO0 to GND (upload mode)
4. Upload the sketch
5. Disconnect GPIO0 from GND and press reset

### 3. Connect Laptop to ESP32-CAM WiFi

The ESP32-CAM creates its own WiFi network:

- **Network Name:** `R2D2_FaceRec`
- **Password:** `recognize`

### 4. Run Face Recognition Server

**Windows:** Double-click `start_server.bat`

**Or manually:**
```bash
cd R2D2_CAM\Face_Recognition_Server
python face_recognition_server.py
```

### 5. Open Browser

Go to: **http://localhost:5000**

## System Architecture

```
ESP32-CAM (Access Point) → Laptop connects to "R2D2_FaceRec"
                              ↓
                       Python Flask Server
                              ↓
                       Face Detection/Recognition
                       (OpenCV + face_recognition)
                              ↓
                       Web Browser at localhost:5000
```

## Hardware Setup

### ESP32-CAM Connections (for uploading)

```
ESP32-CAM          USB-Serial/FTDI
--------          --------------
VCC      ------>   5V
GND      ------>   GND
RX       ------>   TX
TX       ------>   RX
GPIO0    ------>   GND (only during upload)
```

### After Upload

1. Disconnect GPIO0 from GND
2. Power ESP32-CAM with 5V supply
3. Connect laptop to WiFi "R2D2_FaceRec"

## Reference Images Setup

Your images are already organized correctly:
```
R2D2_CAM/Matter/People_Reference/Face pics/
├── Agastya/        (210 images)
├── Joash Dsouza/   (120 images)
└── Ryan/           (136 images)
```

## Configuration

Edit `face_recognition_server.py` if needed:

```python
# ESP32-CAM stream URL (AP mode default)
ESP32_STREAM_URL = "http://192.168.4.1/stream"

# Face match threshold (lower = stricter)
FACE_MATCH_THRESHOLD = 0.5

# Detection model ("hog" = faster, "cnn" = more accurate)
FACE_DETECTION_MODEL = "hog"

# Frame resize for performance (0.5 = half size)
FRAME_RESIZE = 0.5
```

## Troubleshooting

### "Failed to connect to ESP32-CAM"
- Make sure laptop is connected to "R2D2_FaceRec" WiFi
- Check ESP32-CAM is powered on
- Try http://192.168.4.1/stream in browser directly

### "No module named 'dlib'"
```bash
pip install dlib-bin
```

### "No face encodings loaded"
- Check path to reference images
- Ensure folder structure is correct

### Slow Performance
- Lower `FRAME_RESIZE` to 0.3 in Python script
- Use "hog" model instead of "cnn"

## WiFi Settings

To change ESP32-CAM WiFi name/password, edit these lines in `esp32cam_stream.ino`:

```cpp
const char* ap_ssid = "R2D2_FaceRec";     // WiFi name
const char* ap_password = "recognize";     // WiFi password
```

## API Endpoints

| Endpoint | Description |
|----------|-------------|
| `http://localhost:5000/` | Main web interface |
| `http://localhost:5000/processed_feed` | MJPEG stream with detection |
| `http://localhost:5000/raw_feed` | Unprocessed camera stream |
| `http://localhost:5000/status` | JSON status info |
| `http://192.168.4.1/stream` | Raw ESP32-CAM stream |

## Project Structure

```
Face_Recognition_Server/
├── face_recognition_server.py   # Main Python server
├── requirements.txt             # Python dependencies
├── start_server.bat             # Windows startup script
├── start_server.sh              # Linux/macOS startup script
├── README.md                    # This file
├── templates/
│   └── index.html               # Web interface
└── esp32cam_stream/
    ├── esp32cam_stream.ino      # ESP32-CAM sketch (AP mode)
    └── camera_pins.h            # Pin definitions
```

## License

MIT License - Free to use and modify
