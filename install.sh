#!/bin/bash
sudo ~/g2/usr/sbin/grub-install --force --no-floppy --boot-directory=/home/`whoami`/boot /dev/loop0
