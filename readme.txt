##############
EXAMPLE run.sh
##############
#!/bin/bash

#sudo modprobe bcm2835-v4l2
#/opt/vc/bin/vcgencmd measure_temp
#gst-launch-1.0 v4l2src ! decodebin ! autovideosink
#gst-launch-1.0 v4l2src ! ximagesink

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

#touch debug.snapshot
#rm debug.snapshot

reset

#./ipCamCapture -l 'rtsp://192.168.178.28:88/videoMain' -u '<username>' -p '<password>' -a 'video' -t 5
#./ipCamCapture -l 'rtsp://192.168.178.28:88/videoSub' -u '<username>' -p '<password>' -a 'video' -t 5
#./ipCamCapture -l 'rtsp://192.168.178.28:88/videoSub' -u '<username>' -p '<password>' -a 'photo' -t 2
#./ipCamCapture -l 'rtsp://192.168.178.28:88/videoMain' -u '<username>' -p '<password>' -a 'photo' -t 2 -s
./ipCamCapture -l 'rtsp://192.168.178.28:88/videoMain' -u '<username>' -p '<password>' -a 'photo' -t 5




##############################
RASPBIAN SYSTEMD SERVICE FILES
##############################
sudo cp /home/pi/github/ip-cam/ip-cam.service /etc/systemd/system/.
sudo cp /home/pi/github/ip-cam/picamera/picamera.service /etc/systemd/system/.

sudo chmod +x /etc/systemd/system/ip-cam.service 
sudo chmod +x /etc/systemd/system/picamera.service 

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



