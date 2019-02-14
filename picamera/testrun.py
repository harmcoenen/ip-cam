from picamera import PiCamera
from picamera import Color
from time import sleep
import datetime
import time
import cv2
import numpy as np

print("OpenCV Version: {}".format(cv2.__version__))

camera = PiCamera()

# camera.rotation = 180
# https://picamera.readthedocs.io/en/release-1.13/fov.html#sensor-modes
# https://picamera.readthedocs.io/en/release-1.10/recipes1.html 
# camera.resolution = (1920, 1080) # 3280x2464 which is 4:3
camera.resolution = (3280, 2464)
camera.annotate_background = Color('black')
camera.annotate_foreground = Color('white')
try:
    for i in range(10):
    #while True:
        filenamestamp = datetime.datetime.fromtimestamp(time.time()).strftime('%Y_%m_%d_%H_%M_%S')
        annotationstamp = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')
        camera.annotate_text = annotationstamp
        # Capture options:
        # - quality has range from 1 to 100, default is 85. 1 is low quality and 100 is high quality
        #camera.capture('/home/pi/github/ip-cam/upl/%s_py.jpeg' % filenamestamp, resize=(1280, 720), quality=20)
        t1 = time.time()
        camera.capture('image1.jpeg', quality=100)
        t2 = time.time()
        sleep(5)
        t3 = time.time()
        camera.capture('image2.jpeg', quality=100)
        t4 = time.time()
        img1 = cv2.imread('image1.jpeg')
        t5 = time.time()
        img2 = cv2.imread('image2.jpeg')
        t6 = time.time()
        diff = cv2.subtract(img1, img2)
        t7 = time.time()
        b, g, r = cv2.split(diff)
        t8 = time.time()
        print("t1->t2 is ", t2-t1, " t3->t4 is ", t4-t3) 
        print("t4->t5 is ", t5-t4, " t5->t6 is ", t6-t5)
        print("t6->t7 is ", t7-t6, " t7->t8 is ", t8-t7)
        print("diffs in b ", cv2.countNonZero(b), ", diffs in g ", cv2.countNonZero(g), ", diffs in r ", cv2.countNonZero(r))


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
