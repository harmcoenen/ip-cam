#!/bin/bash

if [ -d /mnt/hgfs/SharedFolder/github/ip-cam ] ; then
    cp '/mnt/hgfs/SharedFolder/github/ip-cam/.travis.yml' /home/harm/github/ip-cam/.
    cp -r /mnt/hgfs/SharedFolder/github/ip-cam/* /home/harm/github/ip-cam/.
else
    echo "Can not copy source tree as the SharedFolder is not available on this system."
fi

gcc ipCamCapture.c ipCamPrinting.c ipCamFTP.c -o ipCamCapture `pkg-config --cflags --libs gstreamer-video-1.0 gstreamer-1.0 libcurl` -D_XOPEN_SOURCE=700 -D_GNU_SOURCE
