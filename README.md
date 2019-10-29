# Internet Protocol Camera (ip-cam)

## About
This project is a security suite using [gstreamer](https://gstreamer.freedesktop.org/documentation/tutorials/index.html) and running on a [RaspBerry Pi3](https://www.raspberrypi.org/).
It can perform the following activities:
* On a defined interval take a snapshot picture from a live stream IP camera on the local network.
* On a defined interval take a snapshot picture from the local attached Raspberry Pi3 v2 camera.
* With a defined period of time capture video from a live stream IP camera on the local network.
* Scale down the captured pictures.
* Motion detection.
* Every minute upload all the captured pictures or videos to an FTP-server in a date/time structured directory (easy to find back the data).
* The uploaded data has a retention time of 14 days. Every 30 minutes a cleanup acion is done on the FTP-server.
* The suite is split up in two services:
  * picamera.service which is programmed in Python. This only captures the pictures from the Pi Camera v2.
  * ip-cam.service which is programmed in c language. Controls the rest of the whole suite.


## Setup RaspBerry Pi3
Setup [RaspBerry Pi3](https://projects.raspberrypi.org/en/projects/raspberry-pi-setting-up/3) including a [pi camera v2](https://www.raspberrypi.org/products/camera-module-v2/)

On a Windows machine prepare an SD-card (16 GB or larger)
* Format SD-card by using [sdcard formatter](https://www.sdcard.org/downloads/formatter/eula_windows/index.html)
* Download latest [NOOBS](https://www.raspberrypi.org/downloads/noobs/) zipfile
* Extract all NOOBS files and copy all files to SD-card with Windows Explorer.

On the RaspBerry Pi3
* Boot Raspberry Pi3 with SD-card inserted
* Enable 'ssh connection' and 'camera' in Pi configuration
* Open a new Terminal
* Check out the git repository
```
mkdir github
cd github
git clone https://github.com/harmcoenen/ip-cam.git
```
* Install [gstreamer](https://gstreamer.freedesktop.org/documentation/installing/on-linux.html)
```
sudo apt-get install libgstreamer1.0-0 gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-pulseaudio
sudo apt-get install libgstreamer-plugins-base1.0-dev
sudo apt-get install libcurl4-nss-dev
```
* Build the software suite
```
cd ~/github/ip-cam
./build.sh
```
* Create run.sh (from readme.txt and below values are examples)
```
#!/bin/bash
export GST_DEBUG="ipcam:2"
export CAMERA='rtsp://192.168.178.28:88/videoMain'
export CAM_USER="username"
export CAM_PASS="password"
./ipCamCapture -l "$CAMERA" -u "$CAM_USER" -p "$CAM_PASS" -a 'photo' -t 5 -s
```
* Add services to systemd \(to make sure that after every boot your security suite is automatically started\)
```
sudo cp /home/pi/github/ip-cam/ip-cam.service /lib/systemd/system/.
sudo cp /home/pi/github/ip-cam/picamera/picamera.service /lib/systemd/system/.
sudo systemctl enable ip-cam.service
sudo systemctl enable picamera.service
```

## Start the security suite
* Since all is set up now just reboot the RaspBerry Pi3 and the suite should start automatically after boot.
* The following commands can be used to control the two services.
```
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
```



