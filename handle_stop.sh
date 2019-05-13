#!/bin/bash

if [ -f ./reboot.force ]; then
  echo "Rebooting now..."
  reboot -f
else
  echo "No need for reboot."
fi
