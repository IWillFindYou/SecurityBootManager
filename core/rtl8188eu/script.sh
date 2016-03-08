make clean -C /lib/modules/`uname -r`/build M=`pwd`
make -j2 C=1 CF=-D__CHECK_ENDIAN__ -C /lib/modules/`uname -r`/build M=`pwd`

sudo rmmod rtl8188eu
sudo rmmod rtl_usb
sudo rmmod rtlwifi

sleep 1

sudo insmod ./rtlwifi.ko
sudo insmod ./rtl_usb.ko
sudo insmod ./rtl8188eu/rtl8188eu.ko
