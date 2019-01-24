from picamera import PiCamera
from time import sleep

camera = PiCamera()

camera.rotation = 180
camera.start_preview(alpha=245)
for i in range(5):
    sleep(5)
    camera.capture('/home/pi/Desktop/image%s.jpg' % i)
sleep(2)

camera.start_recording('/home/pi/video.h264')
sleep(10)
camera.stop_recording()

camera.annotate_text_size = 80

for i in range(100):
    camera.annotate_text = "Brightness: %s" % i
    camera.brightness = i
    sleep(0.1)

camera.brightness = 50

for i in range(100):
    camera.annotate_text = "Contrast: %s" % i
    camera.contrast = i
    sleep(0.1)

camera.contrast = 10

#camera.annotate_background = Color('blue')
#camera.annotate_foreground = Color('yellow')
camera.annotate_text = " Hello world "
sleep(5)

for effect in camera.IMAGE_EFFECTS:
    camera.image_effect = effect
    camera.annotate_text = "Effect: %s" % effect
    sleep(5)

camera.image_effect = 'none'

for awbmodes in camera.AWB_MODES:
    camera.awb_mode = awbmodes
    camera.annotate_text = "AWB_MODE: %s" % awbmodes
    sleep(5)

camera.awb_mode = 'auto'

for exposuremodes in camera.EXPOSURE_MODES:
    camera.exposure_mode = exposuremodes
    camera.annotate_text = "EXPOSURE_MODE: %s" % exposuremodes
    sleep(5)

camera.exposure_mode = 'auto'

camera.resolution = (2592, 1944)
camera.framerate = 15
sleep(5)
camera.capture('/home/pi/Desktop/max.jpg')

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
sleep(5)

camera.stop_preview()
