#!/bin/bash
qemu-img create -f raw /home/`whoami`/brdisk-img.raw 2G
sudo mkfs.ext2 /home/`whoami`/brdisk-img.raw
sudo losetup /dev/loop0 /home/`whoami`/brdisk-img.raw

