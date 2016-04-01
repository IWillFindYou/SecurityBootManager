#!/bin/bash
sudo qemu-system-x86_64 -m 512 -hda ./core/ipxe/src/bin/ipxe.usb -net nic,macaddr=52:00:00:00:00:01 -net tap,ifname=tap0,script=no -M pc
