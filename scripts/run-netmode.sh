#!/bin/bash
sudo qemu-system-x86_64 \
	-hda ./core/ipxe/src/bin/ipxe.usb \
	-hdb /dev/sda \
	-net nic,macaddr=52:00:00:00:00:01 \
	-net tap,ifname=tap0,script=no \
	-usbdevice host:80ee:0030 \
	-m 1G \
	-M pc
