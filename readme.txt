##############
EXAMPLE run.sh
##############
#!/bin/bash

#sudo modprobe bcm2835-v4l2
#/opt/vc/bin/vcgencmd measure_temp
#gst-launch-1.0 v4l2src ! decodebin ! autovideosink
#gst-launch-1.0 v4l2src ! ximagesink

#export GST_DEBUG=4
#export GST_DEBUG="splitmuxsink:9"
#export GST_DEBUG="x264enc:4"
#export GST_DEBUG_FILE="debug.log"
#export GST_DEBUG_DUMP_DOT_DIR=/home/pi/RaspBerryPi/GSTreamerTutorial
export GST_DEBUG="ipcam:2"

reset

#./ipCamCapture -l 'rtsp://192.168.178.28:88/videoMain' -u '<username>' -p '<password>' -a 'video' -t 5
#./ipCamCapture -l 'rtsp://192.168.178.28:88/videoSub' -u '<username>' -p '<password>' -a 'video' -t 5
#./ipCamCapture -l 'rtsp://192.168.178.28:88/videoSub' -u '<username>' -p '<password>' -a 'photo' -t 2
#./ipCamCapture -l 'rtsp://192.168.178.28:88/videoMain' -u '<username>' -p '<password>' -a 'photo' -t 2 -s
./ipCamCapture -l 'rtsp://192.168.178.28:88/videoMain' -u '<username>' -p '<password>' -a 'photo' -t 2




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

journalctl -u ip-cam.service
journalctl -u picamera.service

sudo systemctl stop ip-cam.service
sudo systemctl stop picamera.service

sudo systemctl disable ip-cam.service
sudo systemctl disable picamera.service



