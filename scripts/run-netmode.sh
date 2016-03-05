#!/bin/bash
qemu-system-i386 -m 512 -net nic,macaddr=52:00:00:00:00:01 -net tap,ifname=tap0,script=no -boot n

