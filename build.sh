#!/bin/bash
cp '/mnt/hgfs/SharedFolder/github/ip-cam/.travis.yml' /home/harm/github/ip-cam/.
cp /mnt/hgfs/SharedFolder/github/ip-cam/* /home/harm/github/ip-cam/.

gcc ipCamGstCapt.c ipCamPrinting.c ipCamFTP.c -o ipCamGstCapt `pkg-config --cflags --libs gstreamer-video-1.0 gstreamer-1.0 libcurl`
