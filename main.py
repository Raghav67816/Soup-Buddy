import cv2
import threading
from time import sleep, time
from deepface import DeepFace as dp
import paho.mqtt.client as mqtt
from paho.mqtt.enums import CallbackAPIVersion
from playsound3 import playsound
import random

tags = {
    "happy": ['chingi - happyx3.aac', 'happy-a.mp3'],
    "sad": ["heona - say cheese.aac", "acon - be happy x3.aac"],
    "neutral": ['sophia - you want some monster.aac', 'tongyu - have some fish.aac'],
    "stressed": ['sam - world ending.aac', 'heona - say cheese.aac', 'tongyu - have some fish.aac', 'tongyu - have some fish.aac']
}

def on_connect(client, userdata, flags, reason_code, properties):
    pass

def on_message(client, userdata, message):
    pass

def speak_dialog(emotion: str):
    audio_files = tags[emotion]
    random_num = random.randint(0, len(audio_files) - 1)
    select = f"./sounds/{audio_files[random_num]}"
    playsound(select)

client = mqtt.Client(CallbackAPIVersion.VERSION2)
client.on_connect = on_connect
client.on_message = on_message
client.connect("0.0.0.0", 1883, 60)
client.loop_start()

analysis_running = False 

def analyze_face_background(frame_to_analyze):
    global analysis_running
    try:
        cv2.imwrite("user.jpg", frame_to_analyze)
        data = dp.analyze(img_path="./user.jpg", actions=['emotion'], detector_backend='retinaface', enforce_detection=False)
        
        emotion = data[0]['dominant_emotion']
            
        print(f"Detected: {emotion}")
        print("Sending to soup...")
        client.publish("emotion", emotion)
        
        speak_dialog(emotion)
            
    except Exception as e:
        print(f"Analysis skipped or failed: {e}")
        
    finally:
        analysis_running = False

capture = cv2.VideoCapture(0)
last_analysis_time = 0
analysis_interval = 5.0 

while True:
    rect, frame = capture.read()
    if not rect:
        continue

    cv2.imshow("Live Feed", frame)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

    current_time = time()
    if (current_time - last_analysis_time >= analysis_interval) and not analysis_running:
        analysis_running = True
        last_analysis_time = current_time
        
        worker = threading.Thread(target=analyze_face_background, args=(frame.copy(),), daemon=True)
        worker.start()

capture.release()
cv2.destroyAllWindows()
client.loop_stop()
