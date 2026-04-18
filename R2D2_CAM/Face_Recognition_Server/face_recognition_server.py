"""
ESP32-CAM Face Recognition Server
Captures video stream from ESP32-CAM, performs face detection and recognition,
and serves the processed video to a web interface.

Based on: Gesture-Detection-using-ESP32 by skdey-00
"""

import cv2
import numpy as np
import face_recognition
import os
import glob
import threading
import time
from flask import Flask, render_template, Response, jsonify
import urllib.request

# ================================
# CONFIGURATION
# ================================

# ESP32-CAM stream URL (AP mode - ESP32 hosts its own WiFi)
# Connect laptop to WiFi: "R2D2_FaceRec" (password: recognize)
ESP32_STREAM_URL = "http://192.168.4.1/stream"

# Alternative: Use USB serial camera or webcam
# Set USE_WEBCAM = True to use your computer's webcam instead
USE_WEBCAM = False
WEBCAM_ID = 0

# Path to reference images
REFERENCE_PATH = "../Matter/People_Reference/Face pics"

# Face recognition settings
FACE_DETECTION_MODEL = "hog"  # "hog" (faster) or "cnn" (more accurate, needs GPU)
FACE_MATCH_THRESHOLD = 0.5    # Lower = more strict (0.0-1.0)
FRAME_RESIZE = 0.5            # Resize frame for processing (0.5 = half size)

# MJPEG stream settings
MJPEG_BOUNDARY = "frame"
JPEG_QUALITY = 85

# ================================
# FLASK APP SETUP
# ================================

app = Flask(__name__)
app.secret_key = "face_recognition_secret_key"

# Global variables
latest_frame = None
latest_frame_lock = threading.Lock()
face_encodings_dict = {}
face_names = []
is_streaming = False

# ================================
# FACE ENCODING LOADER
# ================================

def load_face_encodings():
    """
    Load and encode faces from reference images.
    Scans the People_Reference folder for person subfolders.
    """
    global face_encodings_dict, face_names

    print("=" * 50)
    print("Loading Face Encodings...")
    print("=" * 50)

    # Find all person folders
    person_folders = []
    if os.path.exists(REFERENCE_PATH):
        for item in os.listdir(REFERENCE_PATH):
            item_path = os.path.join(REFERENCE_PATH, item)
            if os.path.isdir(item_path):
                person_folders.append((item, item_path))

    if not person_folders:
        print(f"WARNING: No person folders found in {REFERENCE_PATH}")
        print("Please organize reference images as: People_Reference/Face pics/PersonName/*.jpg")
        return

    print(f"Found {len(person_folders)} people to encode:")

    # Process each person's images
    for person_name, folder_path in person_folders:
        print(f"\nProcessing {person_name}...")

        # Find all image files
        image_extensions = ['*.jpg', '*.jpeg', '*.png', '*.JPG', '*.JPEG', '*.PNG']
        image_files = []
        for ext in image_extensions:
            image_files.extend(glob.glob(os.path.join(folder_path, ext)))

        if not image_files:
            print(f"  WARNING: No images found for {person_name}")
            continue

        print(f"  Found {len(image_files)} images")

        # Encode faces from images
        encodings = []
        processed = 0
        errors = 0

        for img_path in image_files:
            try:
                # Load image
                img = face_recognition.load_image_file(img_path)

                # Find face locations and encodings
                face_locations = face_recognition.face_locations(img, model=FACE_DETECTION_MODEL)

                if face_locations:
                    # Get encoding for the first face found
                    encoding = face_recognition.face_encodings(img, face_locations)[0]
                    encodings.append(encoding)
                    processed += 1
                else:
                    errors += 1

                # Progress indicator
                if processed % 20 == 0:
                    print(f"  Processed {processed}/{len(image_files)} images...")

            except Exception as e:
                errors += 1
                if errors <= 5:  # Only print first few errors
                    print(f"  Error processing {os.path.basename(img_path)}: {e}")

        # Store encodings (use average to reduce memory)
        if encodings:
            # Compute average encoding for this person
            avg_encoding = np.mean(encodings, axis=0)
            face_encodings_dict[person_name] = avg_encoding
            face_names.append(person_name)
            print(f"  ✓ Encoded {person_name} from {processed} images ({errors} errors)")
        else:
            print(f"  ✗ No faces detected in {person_name}'s images")

    print("\n" + "=" * 50)
    print(f"Face encoding complete! Recognized {len(face_names)} people:")
    for name in face_names:
        print(f"  - {name}")
    print("=" * 50)

# ================================
# VIDEO CAPTURE
# ================================

class ESP32Camera:
    """Captures video from ESP32-CAM MJPEG stream"""

    def __init__(self, stream_url):
        self.stream_url = stream_url
        self.stream = None
        self.bytes_collected = b''

    def connect(self):
        """Connect to the MJPEG stream"""
        try:
            self.stream = urllib.request.urlopen(self.stream_url, timeout=5)
            return True
        except Exception as e:
            print(f"Failed to connect to ESP32-CAM: {e}")
            return False

    def read_frame(self):
        """
        Read a single JPEG frame from MJPEG stream.
        Returns the frame as numpy array or None.
        """
        if self.stream is None:
            if not self.connect():
                return None

        try:
            # Read data until we find a complete JPEG frame
            while True:
                # Find JPEG start marker
                while self.bytes_collected.find(b'\xff\xd8') == -1:
                    chunk = self.stream.read(4096)
                    if not chunk:
                        return None
                    self.bytes_collected += chunk

                # Find JPEG end marker
                start_idx = self.bytes_collected.find(b'\xff\xd8')
                end_idx = self.bytes_collected.find(b'\xff\xd9', start_idx)

                if end_idx != -1:
                    # Extract complete JPEG
                    jpeg_data = self.bytes_collected[start_idx:end_idx+2]
                    self.bytes_collected = self.bytes_collected[end_idx+2:]

                    # Decode JPEG
                    img_array = np.frombuffer(jpeg_data, dtype=np.uint8)
                    frame = cv2.imdecode(img_array, cv2.IMREAD_COLOR)

                    return frame
                else:
                    # Need more data
                    chunk = self.stream.read(4096)
                    if not chunk:
                        return None
                    self.bytes_collected += chunk

        except Exception as e:
            print(f"Error reading frame: {e}")
            self.stream = None
            self.bytes_collected = b''
            return None

class WebcamCapture:
    """Fallback to webcam if ESP32-CAM is unavailable"""

    def __init__(self, camera_id=0):
        self.camera_id = camera_id
        self.cap = None

    def connect(self):
        try:
            self.cap = cv2.VideoCapture(self.camera_id)
            return self.cap.isOpened()
        except Exception as e:
            print(f"Failed to open webcam: {e}")
            return False

    def read_frame(self):
        if self.cap is None:
            self.connect()

        ret, frame = self.cap.read()
        return frame if ret else None

# Initialize camera
camera = None

def init_camera():
    """Initialize the appropriate camera source"""
    global camera

    if USE_WEBCAM:
        print("Using webcam...")
        camera = WebcamCapture(WEBCAM_ID)
    else:
        print(f"Connecting to ESP32-CAM at {ESP32_STREAM_URL}...")
        camera = ESP32Camera(ESP32_STREAM_URL)
        print("Waiting for connection...")

# ================================
# FACE DETECTION & RECOGNITION
# ================================

def detect_and_recognize_faces(frame):
    """
    Detect faces in frame and recognize them against known encodings.
    Returns the annotated frame.
    """
    if frame is None:
        return None

    # Resize for faster processing
    small_frame = cv2.resize(frame, (0, 0), fx=FRAME_RESIZE, fy=FRAME_RESIZE)

    # Convert BGR to RGB for face_recognition
    rgb_frame = cv2.cvtColor(small_frame, cv2.COLOR_BGR2RGB)

    # Detect face locations
    face_locations = face_recognition.face_locations(rgb_frame, model=FACE_DETECTION_MODEL)

    # Get face encodings
    face_encodings = face_recognition.face_encodings(rgb_frame, face_locations)

    # Process each detected face
    for (top, right, bottom, left), face_encoding in zip(face_locations, face_encodings):
        # Scale back up to original frame size
        scale = 1 / FRAME_RESIZE
        top = int(top * scale)
        right = int(right * scale)
        bottom = int(bottom * scale)
        left = int(left * scale)

        # Try to match face
        name = "Intruder"
        color = (0, 0, 255)  # Red for intruder

        if face_encodings_dict:
            # Compare against known faces
            matches = face_recognition.compare_faces(
                list(face_encodings_dict.values()),
                face_encoding,
                tolerance=FACE_MATCH_THRESHOLD
            )

            # Find best match
            face_distances = face_recognition.face_distance(
                list(face_encodings_dict.values()),
                face_encoding
            )

            if len(face_distances) > 0:
                best_match_index = np.argmin(face_distances)

                if matches[best_match_index]:
                    name = list(face_encodings_dict.keys())[best_match_index]
                    color = (0, 255, 0)  # Green for recognized

        # Draw bounding box
        cv2.rectangle(frame, (left, top), (right, bottom), color, 3)

        # Draw label background
        label = f"Hello {name}" if name != "Intruder" else name
        label_size, _ = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.7, 2)
        cv2.rectangle(frame, (left, bottom + 10), (left + label_size[0] + 10, bottom - 30), color, -1)

        # Draw label text
        cv2.putText(frame, label, (left + 5, bottom - 8),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

    # Add status overlay
    status_text = f"Known faces: {len(face_names)} | Detected: {len(face_locations)}"
    cv2.putText(frame, status_text, (10, 30),
               cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)

    return frame

# ================================
# STREAM PROCESSING THREAD
# ================================

def stream_processor():
    """Background thread that continuously captures and processes frames"""
    global latest_frame, is_streaming

    print("Stream processor started")

    while is_streaming:
        frame = camera.read_frame()

        if frame is not None:
            # Process frame for face detection/recognition
            processed_frame = detect_and_recognize_faces(frame)

            if processed_frame is not None:
                with latest_frame_lock:
                    latest_frame = processed_frame
        else:
            print("Lost connection, retrying...")
            time.sleep(1)

# ================================
# FLASK ROUTES
# ================================

@app.route('/')
def index():
    """Main page with video stream"""
    return render_template('index.html',
                          known_faces=face_names,
                          esp32_url=ESP32_STREAM_URL,
                          using_webcam=USE_WEBCAM)

@app.route('/processed_feed')
def processed_feed():
    """
    MJPEG stream endpoint with face detection/recognition overlay.
    Returns a multipart response with JPEG frames.
    """
    def generate():
        while True:
            with latest_frame_lock:
                if latest_frame is not None:
                    # Encode frame as JPEG
                    ret, buffer = cv2.imencode('.jpg', latest_frame)
                    if ret:
                        frame_bytes = buffer.tobytes()

                        # Yield MJPEG frame
                        yield (b'--' + MJPEG_BOUNDARY.encode() + b'\r\n'
                               b'Content-Type: image/jpeg\r\n\r\n' +
                               frame_bytes + b'\r\n\r\n')

            time.sleep(0.033)  # ~30 FPS

    return Response(generate(),
                    mimetype='multipart/x-mixed-replace; boundary=' + MJPEG_BOUNDARY)

@app.route('/raw_feed')
def raw_feed():
    """
    Raw MJPEG stream from ESP32-CAM (no processing).
    For debugging/viewing the unprocessed stream.
    """
    def generate():
        while True:
            frame = camera.read_frame()
            if frame is not None:
                ret, buffer = cv2.imencode('.jpg', frame)
                if ret:
                    frame_bytes = buffer.tobytes()
                    yield (b'--' + MJPEG_BOUNDARY.encode() + b'\r\n'
                           b'Content-Type: image/jpeg\r\n\r\n' +
                           frame_bytes + b'\r\n\r\n')
            time.sleep(0.033)

    return Response(generate(),
                    mimetype='multipart/x-mixed-replace; boundary=' + MJPEG_BOUNDARY)

@app.route('/status')
def status():
    """Return server status information"""
    status_info = {
        'running': is_streaming,
        'known_faces': face_names,
        'face_count': len(face_names),
        'esp32_connected': camera is not None,
        'using_webcam': USE_WEBCAM,
        'esp32_url': ESP32_STREAM_URL
    }
    return jsonify(status_info)

# ================================
# MAIN
# ================================

def main():
    global is_streaming

    print("=" * 60)
    print("ESP32-CAM Face Recognition Server")
    print("=" * 60)

    # Load face encodings
    load_face_encodings()

    if not face_encodings_dict:
        print("\nWARNING: No face encodings loaded!")
        print("Please check your reference images folder structure.")
        print("Expected: People_Reference/Face pics/PersonName/*.jpg")
        return

    # Initialize camera
    init_camera()

    # Start stream processor thread
    is_streaming = True
    processor_thread = threading.Thread(target=stream_processor, daemon=True)
    processor_thread.start()

    print("\n" + "=" * 60)
    print("Server starting...")
    print("=" * 60)
    print("\nOpen your browser to: http://localhost:5000")
    print("\nPress Ctrl+C to stop the server")
    print("=" * 60 + "\n")

    # Run Flask app
    try:
        app.run(host='0.0.0.0', port=5000, debug=False, threaded=True)
    except KeyboardInterrupt:
        print("\n\nShutting down...")
        is_streaming = False
        if camera and hasattr(camera, 'cap') and camera.cap:
            camera.cap.release()

if __name__ == '__main__':
    main()
