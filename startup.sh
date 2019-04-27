#!/bin/bash
qemu-system-x86_64 -curses -drive id=boot,format=raw,file=$USER.img,if=none -drive id=data,format=raw,file=$USER-data.img,if=none -device ahci,id=ahci -device ide-drive,drive=boot,bus=ahci.0 -device ide-drive,drive=data,bus=ahci.1 -gdb tcp::9999

#Explanation of parameters:
#  -curses         use a text console (omit this to use default SDL/VNC console)
#  -drive ...      connect a CD-ROM or hard drive with corresponding image
#  -device ...     configure an AHCI controller for the boot and data disks
#  -gdb tcp::9999  listen for "remote" debugging connections on port NNNN
#  -S              wait for GDB to connect at startup
#  -no-reboot      prevent reboot when OS crashes
