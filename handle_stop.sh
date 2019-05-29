#!/bin/bash

if [ -f ./reboot.force ]; then
    rm ./reboot.force
    sync
    echo "Rebooting now..."
    reboot -f
else
    echo "No need for reboot."
fi
