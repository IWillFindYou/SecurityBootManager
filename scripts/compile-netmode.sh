#!/bin/bash

# grub2 install
cd ./core/grub2
if test -e "`pwd`/configure"; then
  make
#  make install
else
  ./linguas.sh
  ./autogen.sh
  ./configure --disable-efiemu --prefix=/home/`whoami`/g2/usr
  make
  make install
fi
cd ../..

# ipxe compile
cd ./core/ipxe/src
make bin/ipxe.usb
cd ../../..

# create bridge setting
nic_interface_name=""
br0_interface_name=""
tap0_interface_name=""
interface_list=`ifconfig | grep "Ethernet" | awk '{split($$0,arr," "); print arr[1];}'`
for x in $interface_list
do
  check_nic_card=`ifconfig $x | grep -e RX\ bytes:[1-9]`
  if test -n "$check_nic_card"; then
    nic_interface_name=$x
  fi
  if test $x == "br0"; then
    br0_interface_name=$x
  fi
  if test $x == "tap0"; then
    tap0_interface_name=$x
  fi
done

# create br0 interface
if test -z "$nic_interface_name"; then
  echo "not found ethernet network nic card - error"
elif test -z "$br0_interface_name"; then
  nic_addr_info=`ifconfig $nic_interface_name | grep -E '[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}'`
  nic_addr_addr=`echo $nic_addr_info | awk '{split($$0,arr," "); print arr[2];}' | awk '{split($$0,arr,":"); print arr[2];}'`
  nic_addr_mask=`echo $nic_addr_info | awk '{split($$0,arr," "); print arr[4];}' | awk '{split($$0,arr,":"); print arr[2];}'`

  sudo brctl addbr br0
  sudo ifconfig br0 $nic_addr_addr netmask $nic_addr_mask up
  sudo ifconfig $nic_interface_name 0.0.0.0
  sudo brctl addif br0 $nic_interface_name
else
  echo "already bridge network setting - skip"
fi

# create tap0 interface
if test -z "$tap0_interface_name"; then
  sudo openvpn --mktun --dev tap0
  sudo ifconfig tap0 0.0.0.0 up
  sudo brctl addif br0 tap0
fi

# mount brdisk-img.raw && grub2 install
is_mounted=`mount | grep /dev/loop0`
if test -z "$is_mounted"; then
  sudo mount /dev/loop0 /home/`whoami`/boot
fi
sudo ~/g2/usr/bin/grub-mknetdir --net-directory=/srv/tftpboot --subdir=boot/grub --modules=http
