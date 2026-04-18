"""
R2D2 Face Recognition Server - Fast OpenCV Version
Uses OpenCV's built-in face detection and LBPH recognizer.
Fast processing + model caching for instant startup.
"""

import os
import sys
import time
import threading
import pickle
import cv2
import numpy as np
import io
from flask import Flask, Response, render_template_string, jsonify
import requests

# ==========================================
# CONFIGURATION
# ==========================================
ESP32_STREAM_URL = "http://192.168.4.1/stream"

FACE_REFERENCE_PATH = "../Matter/People_Reference/Face pics"
CONFIDENCE_THRESHOLD = 70
MODEL_FILE = "face_model_fast.pkl"

# Known people folders and their colors
KNOWN_PEOPLE = {
    "Joash Dsouza": "#00ff00",
    "Ryan": "#0088ff",
    "Agastya": "#00ffff"
}

# ==========================================
# FLASK APP SETUP
# ==========================================
app = Flask(__name__)

# Global variables
detection_results = {"faces": [], "intruder_detected": False, "timestamp": 0}
lock = threading.Lock()
face_recognizer = None
face_cascade = None
label_map_reverse = {}
latest_frame = None  # Store latest frame from ESP32
frame_lock = threading.Lock()

# ==========================================
# HTML TEMPLATE
# ==========================================
HTML_TEMPLATE = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>R2D2 Face Recognition System</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Courier New', monospace;
            background: linear-gradient(135deg, #0a1628 0%, #1a2845 100%);
            color: #00d4ff;
            min-height: 100vh;
        }
        body::before {
            content: '';
            position: fixed;
            top: 0; left: 0; right: 0; bottom: 0;
            background: repeating-linear-gradient(0deg, rgba(0,0,0,0.1), rgba(0,0,0,0.1) 1px, transparent 1px, transparent 2px);
            pointer-events: none;
            z-index: 1000;
        }
        .header {
            text-align: center;
            padding: 20px;
            background: rgba(0, 212, 255, 0.1);
            border-bottom: 2px solid #00d4ff;
            box-shadow: 0 0 20px rgba(0, 212, 255, 0.3);
        }
        .header h1 {
            font-size: 24px;
            text-shadow: 0 0 10px #00d4ff;
            letter-spacing: 2px;
        }
        .main-container {
            display: flex;
            justify-content: center;
            align-items: flex-start;
            padding: 20px;
            gap: 20px;
        }
        .video-container {
            position: relative;
            border: 3px solid #00d4ff;
            border-radius: 10px;
            overflow: hidden;
            background: #000;
            box-shadow: 0 0 30px rgba(0, 212, 255, 0.5);
        }
        #videoFeed {
            display: block;
            max-width: 100%;
            height: auto;
        }
        .info-panel {
            width: 280px;
            background: rgba(0, 212, 255, 0.1);
            border: 2px solid #00d4ff;
            border-radius: 10px;
            padding: 15px;
        }
        .info-panel h2 {
            font-size: 16px;
            margin-bottom: 10px;
            text-align: center;
            border-bottom: 1px solid #00d4ff;
            padding-bottom: 8px;
        }
        .detected-face {
            background: rgba(0, 212, 255, 0.2);
            border: 1px solid #00d4ff;
            border-radius: 5px;
            padding: 8px;
            margin-bottom: 8px;
        }
        .detected-face.intruder {
            background: rgba(255, 68, 68, 0.3);
            border-color: #ff4444;
            animation: pulse 0.5s infinite;
        }
        @keyframes pulse {
            0%, 100% { box-shadow: 0 0 5px rgba(255, 68, 68, 0.5); }
            50% { box-shadow: 0 0 20px rgba(255, 68, 68, 0.8); }
        }
        .face-name {
            font-size: 14px;
            font-weight: bold;
        }
        .face-confidence {
            font-size: 11px;
            opacity: 0.7;
        }
        .status-bar {
            display: flex;
            justify-content: center;
            gap: 30px;
            padding: 10px;
            background: rgba(0, 212, 255, 0.05);
        }
        .status-item {
            text-align: center;
        }
        .status-value {
            font-size: 18px;
            font-weight: bold;
            color: #00ffff;
        }
        .controls {
            text-align: center;
            padding: 15px;
        }
        .btn {
            background: linear-gradient(180deg, #00d4ff, #0088aa);
            color: #000;
            border: 2px solid #00ffff;
            padding: 10px 20px;
            border-radius: 8px;
            cursor: pointer;
            font-family: 'Courier New', monospace;
            font-weight: bold;
            margin: 5px;
        }
        .btn:hover {
            box-shadow: 0 0 20px rgba(0, 212, 255, 0.8);
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>&#129302; R2D2 FACE RECOGNITION (FAST VERSION)</h1>
    </div>

    <div class="status-bar">
        <div class="status-item">
            <div>Status</div>
            <div class="status-value" id="statusValue">CONNECTING...</div>
        </div>
        <div class="status-item">
            <div>Faces</div>
            <div class="status-value" id="faceCount">0</div>
        </div>
        <div class="status-item">
            <div>Intruder</div>
            <div class="status-value" id="intruderStatus">NO</div>
        </div>
    </div>

    <div class="main-container">
        <div class="video-container">
            <img id="videoFeed" src="/video_feed" />
            <canvas id="overlay" style="position:absolute;top:0;left:0;pointer-events:none;"></canvas>
        </div>

        <div class="info-panel">
            <h2>&#128101; DETECTED FACES</h2>
            <div id="detectedFaces">
                <div style="text-align:center; opacity:0.5; padding:15px;">No faces detected</div>
            </div>
            <div class="controls">
                <button class="btn" onclick="testConnection()">&#128266; Test ESP32</button>
                <button class="btn" onclick="location.reload()">&#128257; Refresh</button>
            </div>
        </div>
    </div>

    <script>
        const videoFeed = document.getElementById('videoFeed');
        const overlay = document.getElementById('overlay');
        const overlayCtx = overlay.getContext('2d');

        videoFeed.onload = function() {
            overlay.width = videoFeed.naturalWidth;
            overlay.height = videoFeed.naturalHeight;
            document.getElementById('statusValue').textContent = 'ACTIVE';
            document.getElementById('statusValue').style.color = '#00ff00';
            fetchDetections();
        };

        videoFeed.onerror = function() {
            document.getElementById('statusValue').textContent = 'NO SIGNAL';
            document.getElementById('statusValue').style.color = '#ff4444';
        };

        function testConnection() {
            fetch('/test_esp32')
                .then(r => r.json())
                .then(data => {
                    if (data.connected) {
                        alert('ESP32 Connected! ✓\\n' + data.esp32_url);
                    } else {
                        alert('ESP32 NOT connected! ✗\\n\\nCheck:\\n1. ESP32 is powered on\\n2. Connected to R2D2_FaceRec WiFi\\n3. ESP32 code is uploaded');
                    }
                });
        }

        function fetchDetections() {
            fetch('/detections')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('faceCount').textContent = data.faces.length;
                    document.getElementById('intruderStatus').textContent = data.intruder_detected ? 'YES' : 'NO';

                    if (data.faces.length > 0) {
                        document.getElementById('detectedFaces').innerHTML = data.faces.map(face => `
                            <div class="detected-face ${face.intruder ? 'intruder' : ''}">
                                <div class="face-name">${face.name}</div>
                                <div class="face-confidence">Confidence: ${face.confidence}%</div>
                            </div>
                        `).join('');
                    } else {
                        document.getElementById('detectedFaces').innerHTML = '<div style="text-align:center; opacity:0.5; padding:15px;">No faces detected</div>';
                    }

                    overlayCtx.clearRect(0, 0, overlay.width, overlay.height);
                    data.faces.forEach(face => {
                        const [x, y, w, h] = face.box;
                        const color = face.intruder ? '#ff4444' : face.color;
                        overlayCtx.strokeStyle = color;
                        overlayCtx.lineWidth = 3;
                        overlayCtx.strokeRect(x, y, w, h);
                        overlayCtx.fillStyle = color;
                        overlayCtx.fillRect(x, y - 22, w, 22);
                        overlayCtx.fillStyle = '#000';
                        overlayCtx.font = 'bold 14px Courier New';
                        overlayCtx.fillText(face.name, x + 5, y - 5);
                    });
                });
            setTimeout(fetchDetections, 100);
        }
    </script>
</body>
</html>
"""

# ==========================================
# FACE RECOGNITION TRAINING
# ==========================================
def train_face_recognizer():
    """Train or load the LBPH face recognizer with reference images."""
    global face_recognizer, face_cascade, label_map_reverse

    print("\n" + "="*50)
    print("LOADING FACE RECOGNITION MODEL")
    print("="*50)

    # Initialize
    face_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + 'haarcascade_frontalface_default.xml')

    if face_cascade.empty():
        print("ERROR: Failed to load face cascade!")
        return False

    # Try to load saved model
    model_path = os.path.join(os.path.dirname(__file__), MODEL_FILE)

    if os.path.exists(model_path):
        print("Found saved model - loading...")
        try:
            with open(model_path, 'rb') as f:
                data = pickle.load(f)
                face_recognizer = cv2.face.LBPHFaceRecognizer_create()
                face_recognizer.read(data['model'])
                label_map_reverse = data['label_map']
            print(f"Model loaded in <1 second!")
            print(f"People loaded: {list(label_map_reverse.values())}")
            return True
        except Exception as e:
            print(f"Failed to load model: {e}")
            print("Will retrain...")

    # Train new model
    print("No saved model found - training...")
    print("This will take 1-2 minutes on first run...")
    face_recognizer = cv2.face.LBPHFaceRecognizer_create()

    reference_path = os.path.join(os.path.dirname(__file__), FACE_REFERENCE_PATH)

    if not os.path.exists(reference_path):
        print(f"WARNING: Reference path not found: {reference_path}")
        face_recognizer.train([np.zeros((100, 100), dtype=np.uint8)], np.array([0]))
        label_map_reverse = {0: {'name': 'Unknown', 'color': '#ff4444'}}
        return False

    faces = []
    labels = []
    label_id = 0
    temp_label_map = {}

    for person_name, color in KNOWN_PEOPLE.items():
        person_path = os.path.join(reference_path, person_name)

        if not os.path.exists(person_path):
            print(f"WARNING: Folder not found for {person_name}")
            continue

        print(f"\nProcessing {person_name}...")
        temp_label_map[person_name] = label_id
        label_map_reverse[label_id] = {'name': person_name, 'color': color}

        image_count = 0
        valid_extensions = ['.jpg', '.jpeg', '.png', '.JPG', '.JPEG', '.PNG']

        for filename in os.listdir(person_path):
            if not any(filename.endswith(ext) for ext in valid_extensions):
                continue

            image_path = os.path.join(person_path, filename)

            try:
                img = cv2.imread(image_path)
                if img is None:
                    continue

                gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
                detected = face_cascade.detectMultiScale(gray, 1.1, 5)

                for (x, y, w, h) in detected:
                    face_roi = gray[y:y+h, x:x+w]
                    face_roi = cv2.resize(face_roi, (100, 100))
                    faces.append(face_roi)
                    labels.append(label_id)
                    image_count += 1

            except Exception as e:
                continue

        if image_count > 0:
            print(f"  Loaded {image_count} face samples")
            label_id += 1
        else:
            print(f"  WARNING: No faces found for {person_name}")

    if faces:
        print(f"\nTraining model with {len(faces)} samples...")
        start_time = time.time()
        face_recognizer.train(faces, np.array(labels))
        elapsed = time.time() - start_time
        print(f"Model trained in {elapsed:.1f} seconds!")

        # Save model for next time
        try:
            model_file = os.path.join(os.path.dirname(__file__), MODEL_FILE.replace('.pkl', '.xml'))
            face_recognizer.write(model_file)
            # Save label map separately
            with open(model_path, 'wb') as f:
                pickle.dump({'model': model_file, 'label_map': label_map_reverse}, f)
            print("Model saved! Next startup will be instant.")
        except Exception as e:
            print(f"Could not save model: {e}")

        return True
    else:
        print("\nWARNING: No training data found.")
        face_recognizer.train([np.zeros((100, 100), dtype=np.uint8)], np.array([0]))
        label_map_reverse = {0: {'name': 'Unknown', 'color': '#ff4444'}}
        return False

# ==========================================
# FRAME PROCESSING THREAD
# ==========================================
def process_frames():
    """Continuously fetch and process frames from ESP32."""
    global detection_results

    print("Starting frame processing thread...")
    print(f"Connecting to: {ESP32_STREAM_URL}")
    print("DEBUG: Will print face detection status every 10 seconds...")

    consecutive_errors = 0
    max_errors = 10
    frame_count = 0
    last_debug_time = time.time()

    while True:
        try:
            print(f"Connecting to ESP32 at {ESP32_STREAM_URL}...")
            response = requests.get(ESP32_STREAM_URL, stream=True, timeout=10)

            if response.status_code != 200:
                print(f"ERROR: ESP32 returned status {response.status_code}")
                consecutive_errors += 1
                if consecutive_errors >= max_errors:
                    print(f"Connection error {response.status_code}, waiting 5 seconds...")
                    time.sleep(5)
                    consecutive_errors = 0
                time.sleep(1)
                continue

            if consecutive_errors > 0:
                print("✓ Reconnected to ESP32!")
            else:
                print("✓ Connected to ESP32!")
            consecutive_errors = 0

            # Read MJPEG stream - ESP32 sends continuous JPEG frames
            buffer = b''
            chunk_size = 4096

            for chunk in response.iter_content(chunk_size=chunk_size):
                buffer += chunk

                # Find JPEG start and end markers
                while True:
                    start_idx = buffer.find(b'\xff\xd8')
                    if start_idx == -1:
                        break
                    end_idx = buffer.find(b'\xff\xd9', start_idx)
                    if end_idx == -1:
                        break

                    # Extract complete JPEG frame
                    jpeg_data = buffer[start_idx:end_idx+2]
                    buffer = buffer[end_idx+2:]

                    # Decode frame
                    nparr = np.frombuffer(jpeg_data, dtype=np.uint8)
                    frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

                    if frame is None:
                        print("DEBUG: Failed to decode frame")
                        continue

                    # Save latest frame for video feed (avoid double connection to ESP32)
                    with frame_lock:
                        global latest_frame
                        latest_frame = frame.copy()

                    frame_count += 1
                    current_time = time.time()

                    # Debug output every 10 seconds
                    if current_time - last_debug_time >= 10:
                        print(f"DEBUG: Processed {frame_count} frames | Frame shape: {frame.shape}")
                        last_debug_time = current_time

                    # Process frame - detect faces
                    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
                    faces = face_cascade.detectMultiScale(gray, scaleFactor=1.1, minNeighbors=3, minSize=(30, 30))

                    # Debug: print face detection status
                    if current_time - last_debug_time >= 10:
                        if len(faces) > 0:
                            print(f"DEBUG: Detected {len(faces)} face(s) at this moment")
                        else:
                            print("DEBUG: No faces detected in this frame (trying to detect...)")
                        last_debug_time = current_time

                    detected_faces = []
                    intruder_detected = False

                    for (x, y, w, h) in faces:
                        face_roi = gray[y:y+h, x:x+w]
                        face_roi = cv2.resize(face_roi, (100, 100))

                        # Predict
                        label_id, confidence = face_recognizer.predict(face_roi)

                        if label_id in label_map_reverse:
                            person_data = label_map_reverse[label_id]
                            confidence_percent = max(0, 100 - confidence)

                            if confidence_percent >= CONFIDENCE_THRESHOLD:
                                name = person_data['name']
                                color = person_data['color']
                                intruder = False
                            else:
                                name = "Unknown"
                                color = "#ff4444"
                                intruder = True
                                intruder_detected = True
                        else:
                            name = "Unknown"
                            color = "#ff4444"
                            intruder = True
                            intruder_detected = True
                            confidence_percent = 0

                        detected_faces.append({
                            'name': name,
                            'box': [int(x), int(y), int(w), int(h)],
                            'color': color,
                            'confidence': round(confidence_percent, 1),
                            'intruder': intruder
                        })

                    with lock:
                        detection_results = {
                            'faces': detected_faces,
                            'intruder_detected': intruder_detected,
                            'timestamp': time.time()
                        }

        except requests.exceptions.RequestException as e:
            consecutive_errors += 1
            if consecutive_errors >= max_errors:
                print("Connection lost, waiting...")
                time.sleep(5)
                consecutive_errors = 0
            time.sleep(1)
        except Exception as e:
            print(f"Processing error: {e}")
            time.sleep(1)

# ==========================================
# FLASK ROUTES
# ==========================================
@app.route('/')
def index():
    return render_template_string(HTML_TEMPLATE)

@app.route('/video_feed')
def video_feed():
    """Video streaming route - serves frames from ESP32 (single connection)."""
    def generate():
        while True:
            with frame_lock:
                if latest_frame is not None:
                    # Encode the frame as JPEG
                    _, buffer = cv2.imencode('.jpg', latest_frame)
                    yield (b'--frame\r\n'
                           b'Content-Type: image/jpeg\r\n\r\n' + buffer.tobytes() + b'\r\n\r\n')
                else:
                    # Send waiting frame if no frame available yet
                    waiting_img = np.zeros((240, 320, 3), dtype=np.uint8)
                    cv2.putText(waiting_img, "Connecting to ESP32...", (30, 120),
                               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 255), 1)
                    _, buffer = cv2.imencode('.jpg', waiting_img)
                    yield (b'--frame\r\n'
                           b'Content-Type: image/jpeg\r\n\r\n' + buffer.tobytes() + b'\r\n\r\n')
            time.sleep(0.033)  # ~30 FPS

    return Response(generate(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/detections')
def detections():
    """Return current face detection results."""
    with lock:
        return jsonify(detection_results)

@app.route('/test_esp32')
def test_esp32():
    """Test connection to ESP32."""
    try:
        response = requests.get(ESP32_STREAM_URL, timeout=3)
        return jsonify({
            'connected': response.status_code == 200,
            'esp32_url': ESP32_STREAM_URL,
            'status_code': response.status_code
        })
    except Exception as e:
        return jsonify({
            'connected': False,
            'esp32_url': ESP32_STREAM_URL,
            'error': str(e)
        })

# ==========================================
# MAIN
# ==========================================
if __name__ == '__main__':
    print("\n" + "="*50)
    print("R2D2 FACE RECOGNITION SERVER - FAST VERSION")
    print("="*50 + "\n")

    # Load or train model
    train_face_recognizer()

    # Start processing thread
    process_thread = threading.Thread(target=process_frames, daemon=True)
    process_thread.start()

    # Start server
    print("\n" + "="*50)
    print("SERVER STARTING")
    print("="*50)
    print(f"Open: http://localhost:5000")
    print(f"ESP32 URL: {ESP32_STREAM_URL}")
    print("\nPress Ctrl+C to stop")
    print("="*50 + "\n")

    try:
        app.run(host='0.0.0.0', port=5000, debug=False, threaded=True)
    except KeyboardInterrupt:
        print("\nServer stopped.")
        sys.exit(0)
