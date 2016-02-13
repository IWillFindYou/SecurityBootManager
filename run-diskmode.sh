#!/bin/bash
if test -e "/home/`whoami`/brdisk-img.raw"; then
  qemu-system-i386 -m 512 -hda /home/`whoami`/brdisk-img.raw
else
  echo "Not found brdisk-img.raw"
  echo "Execute compile-diskmode.sh"
  ./compile_diskmode.sh
fi

