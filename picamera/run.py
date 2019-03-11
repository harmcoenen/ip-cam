from picamera import PiCamera
from picamera import Color
from time import sleep
import time
import datetime

try:
    while True:
        camera = PiCamera()
        # camera.rotation = 180
        # https://picamera.readthedocs.io/en/release-1.13/fov.html#sensor-modes
        camera.resolution = (3280, 2464)
        #camera.resolution = (1920, 1080) # 3280x2464 which is 4:3
        camera.annotate_text_size = 64
        camera.annotate_background = Color('black')
        camera.annotate_foreground = Color('white')
        for i in range(7200):
            sleep(2)
            filenamestamp = datetime.datetime.fromtimestamp(time.time()).strftime('%Y_%m_%d_%H_%M_%S')
            annotationstamp = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')
            camera.annotate_text = annotationstamp + ' ' + str(i)
            # Capture options:
            # - quality has range from 1 to 100, default is 85. 1 is low quality and 100 is high quality
            camera.capture('/home/pi/github/ip-cam/upl/%s_py.jpeg' % filenamestamp, resize=(1600, 1200), quality=20)
            #camera.capture('/home/pi/github/ip-cam/upl/%s_py.jpeg' % filenamestamp, quality=10)
        else:
            camera.close()
except:
    print("An exception occurred.")
finally:
    print("End of program.")
