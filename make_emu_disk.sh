#!/usr/bin/env bash
sudo losetup -d /dev/loop0
dd if=/dev/zero of=sdcard.img bs=1M count=129
parted -s sdcard.img mklabel msdos
parted -s sdcard.img mkpart primary 1 135
sudo losetup -Pf --show sdcard.img
sudo chown ${USER}:${USER} /dev/loop0p1
dd if=../../z80/2063-Z80-cpm/filesystem/drive.img of=/dev/loop0p1 bs=512
cp -f ../../z80/2063-Z80-cpm/boot/firmware.bin ./
truncate --size=16K firmware.bin

