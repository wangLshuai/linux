#!/usr/bin/env bash
parameter="-m 1024M -smp 2  -kernel build/arch/x86/boot/bzImage  -hda ./sda.raw -usb -device usb-kbd"

for i in $@
do
    case $i in
    -debug)
        parameter=$parameter" -S -s";;
    -nogui)
        parameter=$parameter" -nographic";;
    -build)
        make O=./build -j`nproc`
        exit 0;;
    -clean)
        make O=./build clean
        exit 0;;
    esac        
done

qemu-system-x86_64 $parameter -append "console=tty0 console=ttyS0 loglevel=15 root=/dev/sda2 nokaslr" -fsdev local,id=fs0,security_model=passthrough,path=./share -device virtio-9p-pci,fsdev=fs0,mount_tag=myshare
