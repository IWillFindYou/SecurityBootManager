#!/bin/bash

# grub2 install
cd grub2
./linguas.sh
./autogen.sh
./configure --disable-efiemu --prefix=/home/`whoami`/g2/usr
make
make install

cd ..

# setup network settings

# grub2 install
sudo ~/g2/usr/bin/grub-mknetdir --net-directory=/srv/tftpboot --subdir=boot/grub --modules=http

