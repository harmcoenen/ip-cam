##############
EXAMPLE run.sh
##############
#!/bin/bash

#sudo modprobe bcm2835-v4l2
#/opt/vc/bin/vcgencmd measure_temp
#gst-launch-1.0 v4l2src ! decodebin ! autovideosink
#gst-launch-1.0 v4l2src ! ximagesink


## COMPLETE SETUP OF RASPBERRY PI (CUSTOM):
## Setup RaspBerry Pi3
## 
## https://www.raspberrypi.org/downloads/noobs/
## https://projects.raspberrypi.org/en/projects/raspberry-pi-setting-up/3
## https://www.sdcard.org/downloads/formatter/eula_windows/index.html
## 
## 1) Format SD-card
## 2) Extract all NOOBS files and copy all files to SD-card with Windows explorer.
## 3) Boot Raspberry Pi with SD-card
## 4) Enable 'ssh connection' and 'camera' in Pi configuration
## 5) Check out git repository
##      cd
##      mkdir github
##      cd github
##      git clone https://github.com/harmcoenen/ip-cam.git
## 6) Install gstreamer
##     https://gstreamer.freedesktop.org/documentation/installing/on-linux.html
##     sudo apt-get install libgstreamer1.0-0 gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-pulseaudio
##     sudo apt-get install libgstreamer-plugins-base1.0-dev
##     sudo apt-get install libcurl4-nss-dev
## 7) Create run.sh (from readme.txt)
## 8) Add services to systemd (disable execute bit)
##      sudo cp /home/pi/github/ip-cam/ip-cam.service /lib/systemd/system/.
##      sudo cp /home/pi/github/ip-cam/picamera/picamera.service /lib/systemd/system/.
##      sudo systemctl enable ip-cam.service
##      sudo systemctl enable picamera.service
## 9) Adapt journalctl max log size
##      sudo vi /etc/systemd/journald.conf
##      SystemMaxUse=100M
##      RuntimeMaxUse=100M





#| # | Name    | Description                                                    |
#|---|---------|----------------------------------------------------------------|
#| 0 | none    | No debug information is output.                                |
#| 1 | ERROR   | Logs all fatal errors. These are errors that do not allow the  |
#|   |         | core or elements to perform the requested action. The          |
#|   |         | application can still recover if programmed to handle the      |
#|   |         | conditions that triggered the error.                           |
#| 2 | WARNING | Logs all warnings. Typically these are non-fatal, but          |
#|   |         | user-visible problems are expected to happen.                  |
#| 3 | FIXME   | Logs all "fixme" messages. Those typically that a codepath that|
#|   |         | is known to be incomplete has been triggered. It may work in   |
#|   |         | most cases, but may cause problems in specific instances.      |
#| 4 | INFO    | Logs all informational messages. These are typically used for  |
#|   |         | events in the system that only happen once, or are important   |
#|   |         | and rare enough to be logged at this level.                    |
#| 5 | DEBUG   | Logs all debug messages. These are general debug messages for  |
#|   |         | events that happen only a limited number of times during an    |
#|   |         | objects lifetime; these include setup, teardown, change of     |
#|   |         | parameters, etc.                                               |
#| 6 | LOG     | Logs all log messages. These are messages for events that      |
#|   |         | happen repeatedly during an objects lifetime; these include    |
#|   |         | streaming and steady-state conditions. This is used for log    |
#|   |         | messages that happen on every buffer in an element for example.|
#| 7 | TRACE   | Logs all trace messages. Those are message that happen very    |
#|   |         | very often. This is for example is each time the reference     |
#|   |         | count of a GstMiniObject, such as a GstBuffer or GstEvent, is  |
#|   |         | modified.                                                      |
#| 8 | MEMDUMP | Logs all memory dump messages. This is the heaviest logging and|
#|   |         | may include dumping the content of blocks of memory.           |
#+------------------------------------------------------------------------------+

#export GST_DEBUG=4
#export GST_DEBUG="splitmuxsink:9"
#export GST_DEBUG="x264enc:4"
#export GST_DEBUG_FILE="debug.log"
#export GST_DEBUG_DUMP_DOT_DIR=/home/pi/RaspBerryPi/GSTreamerTutorial
export GST_DEBUG="ipcam:2"

#export CAMERA='rtsp://192.168.178.28:88/videoSub'
export CAMERA='rtsp://192.168.178.28:88/videoMain'
export CAM_USER=""
export CAM_PASS=""

#touch debug.snapshot
#rm debug.snapshot

#reset

#./ipCamCapture -l "$CAMERA" -u "$CAM_USER" -p "$CAM_PASS" -a 'video' -t 5
#./ipCamCapture -l "$CAMERA" -u "$CAM_USER" -p "$CAM_PASS" -a 'photo' -t 5
 ./ipCamCapture -l "$CAMERA" -u "$CAM_USER" -p "$CAM_PASS" -a 'photo' -t 5 -s




##############################
RASPBIAN SYSTEMD SERVICE FILES
##############################
sudo systemctl enable ip-cam.service
sudo systemctl enable picamera.service

sudo systemctl start ip-cam.service
sudo systemctl start picamera.service

sudo systemctl status ip-cam.service
sudo systemctl status picamera.service

sudo journalctl -u ip-cam.service
sudo journalctl -u picamera.service

sudo systemctl stop ip-cam.service
sudo systemctl stop picamera.service

sudo systemctl disable ip-cam.service
sudo systemctl disable picamera.service



##############################
Some convenient journalctl commands.
##############################
journalctl --disk-usage
journalctl --list-boots
journalctl --verify
journalctl --boot
journalctl --help
journalctl --user
journalctl --system
