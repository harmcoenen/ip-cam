#!/bin/bash
cp /mnt/hgfs/SharedFolder/ip-cam/* /home/harm/github/ip-cam/.

gcc ipCamGstCapt.c -o ipCamGstCapt `pkg-config --cflags --libs gstreamer-video-1.0 gtk+-3.0 gstreamer-1.0`
