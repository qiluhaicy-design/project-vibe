#!/bin/bash

# For graphics (run locally with GUI QEMU):
qemu-system-arm -M raspi2b -kernel kernel.img -usb -device usb-mouse

# For UART output (works in headless):
# qemu-system-arm -M raspi2b -kernel kernel.img -nographic