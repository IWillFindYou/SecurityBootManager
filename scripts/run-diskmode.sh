#!/bin/bash
if test -e "/home/`whoami`/brdisk-img.raw"; then
  device_name="Realtek"
  realtek_wifi_usb=`lsusb | grep $device_name | awk '{split($$0,arr," "); printf arr[6];}'`
  device_name="VirtualBox"
  webcam_usb="`lsusb | grep $device_name | awk '{split($$0,arr," "); printf arr[6];}'`"
  webcam_vendor_id=`echo $webcam_usb | awk '{split($$0,arr,":"); printf arr[1];}'`
  webcam_product_id=`echo $webcam_usb | awk '{split($$0,arr,":"); printf arr[2];}'`

  sudo qemu-system-i386 \
	  -m 512 \
	  -hda /home/`whoami`/brdisk-img.raw \
	  -device usb-ehci,id=ehci \
	  -usbdevice host:80ee:0030 \
	  -serial stdio
#	  -usbdevice host:$realtek_wifi_usb \
#	  -device usb-host,vendorid=0x$webcam_vendor_id,productid=0x$webcam_product_id,bus=ehci.0,port=1 \
else
  echo "Not found brdisk-img.raw"
  echo "Execute compile-diskmode.sh"
  ./compile_diskmode.sh
  echo "please try again to './run-diskmode.sh'"
fi

