from picamera import PiCamera
from time import sleep
import time
import datetime

camera = PiCamera()
camera.rotation=180
camera.start_preview(alpha=140)
st = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')
    
camera.start_preview()
sleep(3)
#camera.annotate_text_size = 32
camera.annotate_text = st
sleep(5)
camera.stop_preview()
