#!/bin/bash

# Run this script to install the device tree overlay.  In principle,
# the device tree overlay should be already loaded, but oftentimes the
# BBB comes up with the overlay unloaded, or the cape is somehow messed up.  
# You can see if the cape's presence has been sensed using this command:
#
#    cat /sys/devices/bone_capemgr.*/slots
#
# and use this command to see if the device tree overlay is loaded:
#
#    cat /sys/kernel/debug/pinctrl/44e10800.pinmux/pinmux-pins 


progress_and_delay () {
    echo -n "."
    sleep 1
}

SLOTS=/sys/devices/bone_capemgr.*/slots

echo -n "Installing uio_pruss module ."

/sbin/modprobe uio_pruss
progress_and_delay

echo ADC_001 > /sys/devices/bone_capemgr.9/slots 

rmmod uio_pruss
progress_and_delay

/sbin/modprobe uio_pruss
progress_and_delay

echo ADC_001 > /sys/devices/bone_capemgr.9/slots 

rmmod uio_pruss
progress_and_delay

/sbin/modprobe uio_pruss
progress_and_delay


( [ -L /sys/class/uio/uio0 ] || [ -d /sys/class/uio/uio0 ] ) && echo ' Success!' && exit 0

# shouldn't get here if things worked
echo 'Failed.'
exit 1;

