from picamera import PiCamera
from picamera import Color
from time import sleep
import time
import datetime

camera = PiCamera()

# camera.rotation = 180
# https://picamera.readthedocs.io/en/release-1.13/fov.html#sensor-modes
camera.resolution = (3280, 2464)
#camera.resolution = (1920, 1080) # 3280x2464 which is 4:3
camera.annotate_text_size = 64
camera.annotate_background = Color('black')
camera.annotate_foreground = Color('white')
#for i in range(10):
try:
    while True:
        sleep(2)
        filenamestamp = datetime.datetime.fromtimestamp(time.time()).strftime('%Y_%m_%d_%H_%M_%S')
        annotationstamp = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')
        camera.annotate_text = annotationstamp
        # Capture options:
        # - quality has range from 1 to 100, default is 85. 1 is low quality and 100 is high quality
        camera.capture('/home/pi/github/ip-cam/upl/%s_py.jpeg' % filenamestamp, resize=(1600, 1200), quality=20)
        #camera.capture('/home/pi/github/ip-cam/upl/%s_py.jpeg' % filenamestamp, quality=10)
finally:
    #Defaults
    camera.rotation = 0
    camera.brightness = 50
    camera.contrast = 50
    camera.image_effect = 'none'
    camera.awb_mode = 'auto'
    camera.exposure_mode = 'auto'
    camera.resolution = (1920, 1080)
    camera.framerate = 30
    camera.annotate_text_size = 32
    camera.annotate_text = "GOOD BYE"
    camera.close()
