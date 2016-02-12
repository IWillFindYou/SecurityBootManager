#!/bin/bash

# grub2 install
cd grub2
if test -e "`pwd`/configure"; then
  make
  make install
else
  ./linguas.sh
  ./autogen.sh
  ./configure --disable-efiemu --prefix=/home/`whoami`/g2/usr
  make
  make install
fi
cd ..

# create image file
if test -e "/home/`whoami`/brdisk-img.raw"; then
  echo "exist brdisk-img.raw file"
else
  qemu-img create -f raw /home/`whoami`/brdisk-img.raw 1G
  sudo mkfs.ext2 /home/`whoami`/brdisk-img.raw
  sudo losetup /dev/loop0 /home/`whoami`/brdisk-img.raw
fi

# make directory
if test -e "/home/`whoami`/boot"; then
  echo "exist boot directory"
else
  mkdir /home/`whoami`/boot
fi

# mount brdisk-img.raw && grub2 install
is_mounted=`mount | grep /dev/loop0`
if test -z "$is_mounted"; then
  sudo mount /dev/loop0 /home/`whoami`/boot
fi
sudo ~/g2/usr/sbin/grub-install --force --no-floppy --boot-directory=/home/`whoami`/boot /dev/loop0

