"""
Simple test script to check ESP32-CAM connection
Run this if start_fast.bat doesn't show video
"""

import requests
import cv2
import numpy as np

ESP32_URL = "http://192.168.4.1/stream"

print("=" * 50)
print("ESP32-CAM Connection Test")
print("=" * 50)
print()

print(f"Testing connection to: {ESP32_URL}")
print()

try:
    print("1. Connecting to ESP32...")
    response = requests.get(ESP32_URL, stream=True, timeout=5)
    print(f"   Status code: {response.status_code}")

    if response.status_code != 200:
        print("   ERROR: ESP32 returned non-200 status!")
        exit(1)

    print("   ✓ Connected!")
    print()

    print("2. Reading first frame...")
    buffer = b''
    for chunk in response.iter_content(chunk_size=4096):
        buffer += chunk
        # Look for JPEG end marker
        if b'\xff\xd9' in buffer:
            break

    # Find JPEG markers
    start_idx = buffer.find(b'\xff\xd8')
    end_idx = buffer.find(b'\xff\xd9')

    if start_idx != -1 and end_idx != -1:
        jpeg_data = buffer[start_idx:end_idx+2]
        print(f"   ✓ Got JPEG frame: {len(jpeg_data)} bytes")

        # Try to decode
        nparr = np.frombuffer(jpeg_data, dtype=np.uint8)
        frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

        if frame is not None:
            print(f"   ✓ Frame decoded successfully!")
            print(f"   Resolution: {frame.shape[1]}x{frame.shape[0]}")
            print()
            print("=" * 50)
            print("SUCCESS! ESP32 is working correctly.")
            print("The issue is with the face recognition script.")
            print()
            print("Check the face_recognition_fast.py console for errors.")
        else:
            print("   ERROR: Could not decode frame")
    else:
        print("   ERROR: No JPEG frame found in response")
        print(f"   Start marker: {'found' if start_idx != -1 else 'NOT FOUND'}")
        print(f"   End marker: {'found' if end_idx != -1 else 'NOT FOUND'}")

except requests.exceptions.ConnectionError as e:
    print(f"   ERROR: Cannot connect to ESP32!")
    print(f"   Details: {e}")
    print()
    print("Make sure:")
    print("  1. You are connected to WiFi: R2D2_FaceRec")
    print("  2. ESP32 is powered on")
    print("  3. ESP32 is NOT being viewed in a browser")

except Exception as e:
    print(f"   ERROR: {e}")
    import traceback
    traceback.print_exc()

print()
print("=" * 50)
