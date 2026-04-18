========================================
R2D2 FACE RECOGNITION SYSTEM
========================================

OVERVIEW
--------
This system connects to your ESP32 CAM and performs real-time face recognition
on your laptop. It recognizes Joash, Ryan, and Agastya, and alerts "INTRUDER"
for unknown people.

ARCHITECTURE
------------
1. ESP32 CAM provides raw video feed via WiFi
2. Laptop runs Python Flask server for face recognition
3. Web browser displays live feed with bounding boxes

QUICK START
-----------
1. Double-click: start_face_recognition.bat
2. Choose "Simple Version" (recommended)
3. Wait for dependencies to install (first run only)
4. Open browser to: http://localhost:5000


REQUIREMENTS
------------
- Python 3.8 or higher (install from python.org)
- ESP32 CAM powered on and running R2D2 firmware
- Laptop and ESP32 connected to same WiFi network


SETUP STEPS
-----------

Step 1: Prepare ESP32 CAM
--------------------------
- Upload the esp32_cam_test.ino to your ESP32 CAM
- Power on the ESP32 CAM
- Connect laptop to ESP32's "R2D2-CAM" WiFi (password: 12345678)
- Or configure ESP32 to join your home WiFi


Step 2: Check Face Reference Images
------------------------------------
Make sure these folders exist with face images:
  Matter/People_Reference/Face pics/Joash Dsouza/
  Matter/People_Reference/Face pics/Ryan/
  Matter/People_Reference/Face pics/Agastya/


Step 3: Run the Server
-----------------------
Double-click: start_face_recognition.bat

Choose:
  [1] Simple Version - Easier setup, good accuracy
  [2] Advanced Version - Higher accuracy, harder to install


TROUBLESHOOTING
---------------

Problem: "Python is not installed"
Solution: Install Python 3.8+ from https://www.python.org/
          Check "Add Python to PATH" during installation

Problem: "Connection to ESP32 CAM lost"
Solution: Make sure ESP32 is powered on
          Verify you're connected to the same WiFi
          Try accessing http://192.168.4.1 directly in browser

Problem: "No faces detected" or low accuracy
Solution: Improve lighting conditions
            Ensure face reference images are clear
            For advanced version, try adjusting TOLERANCE value

Problem: "dlib installation failed" (Advanced Version)
Solution: Use the Simple Version instead
            Or install Visual Studio Build Tools for Windows


CONFIGURATION
-------------

To change ESP32 stream URL:
- Edit face_recognition_simple.py or face_recognition_server.py
- Change ESP32_STREAM_URL at the top

If ESP32 is on your home WiFi:
  ESP32_STREAM_URL = "http://192.168.1.XXX/stream"
Replace XXX with your ESP32's IP address


FACE RECOGNITION VERSIONS
--------------------------

Simple Version (Recommended):
- Uses OpenCV's Haar Cascade + LBPH Face Recognizer
- Easy to install on Windows
- Good accuracy for well-lit conditions
- Files: face_recognition_simple.py, requirements_simple.txt

Advanced Version:
- Uses face_recognition library with dlib
- Higher accuracy, works in varied conditions
- Requires dlib (can be difficult to install on Windows)
- Files: face_recognition_server.py, requirements.txt


COLOR CODING
------------
Green box  = Joash Dsouza
Blue box   = Ryan
Cyan box   = Agastya
Red box    = INTRUDER (unknown person)


SYSTEM STATUS
-------------
The web interface shows:
- Status: INITIALIZING / CONNECTED / DISCONNECTED
- Faces Detected: Number of faces in frame
- Intruder: YES/NO - Shows when unknown person detected
- Activity Log: Real-time detection events


FILES CREATED
-------------
- start_face_recognition.bat    - Launcher script
- face_recognition_simple.py    - Simple version (OpenCV)
- face_recognition_server.py    - Advanced version (dlib)
- requirements_simple.txt       - Dependencies for simple
- requirements.txt              - Dependencies for advanced
- FACE_RECOGNITION_README.txt   - This file


SUPPORT
-------
For issues with:
- ESP32 CAM: Check esp32_cam_test.ino
- Face Recognition: Check Python error messages
- WiFi: Verify ESP32 and laptop on same network
