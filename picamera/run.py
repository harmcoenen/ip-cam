from picamera import PiCamera
from picamera import Color
from time import sleep
import sys
import time
import getopt
import datetime

def main():
    rotation = 0
    sleeptime = 3
    run_program = True

    try:
        opts, args = getopt.getopt(sys.argv[1:], "r:s:", ["rotation=","sleeptime="])
    except getopt.GetoptError as err:
        print (str(err))
        sys.exit(2)

    for opt, arg in opts:
        if opt in ("-r", "--rotation"):
            rotation = arg
        elif opt in ("-s", "--sleeptime"):
            sleeptime = arg

    print("Rotation  : " + str(rotation))
    print("Sleeptime : " + str(sleeptime))

    while run_program:
        try:
            camera = PiCamera()
            camera.rotation = int(rotation)
            # https://picamera.readthedocs.io/en/release-1.13/fov.html#sensor-modes
            #camera.resolution = (3280, 2464)
            camera.resolution = (1600, 1200)
            #camera.resolution = (1920, 1080) # 3280x2464 which is 4:3
            camera.annotate_text_size = 64
            camera.annotate_background = Color('black')
            camera.annotate_foreground = Color('white')
            for i in range(7200):
                sleep(int(sleeptime))
                filenamestamp = datetime.datetime.fromtimestamp(time.time()).strftime('%Y_%m_%d_%H_%M_%S')
                annotationstamp = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')
                camera.annotate_text = annotationstamp + ' ' + str(i)
                # Capture options:
                # - quality has range from 1 to 100, default is 85. 1 is low quality and 100 is high quality
                camera.capture('/home/pi/github/ip-cam/upl/%s_py.jpeg' % filenamestamp, resize=(1600, 1200), quality=20)
                #camera.capture('/home/pi/github/ip-cam/upl/%s_py.jpeg' % filenamestamp, quality=20)
                #camera.capture('/home/pi/github/ip-cam/upl/%s_py.jpeg' % filenamestamp, quality=10)
            else:
                camera.close()
        except KeyboardInterrupt:
            print("Exception: ", sys.exc_info())
            run_program = False
        except:
            print("Exception: ", sys.exc_info())
        finally:
            camera.close()
            if run_program:
                print("Restart program.")
    print("End program.")

if __name__ == "__main__":
    main()

