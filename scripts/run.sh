#!/bin/bash

# For graphics (run locally with GUI QEMU):
qemu-system-arm -M virt -kernel kernel.img -vga std

# For UART output (works in headless):
# qemu-system-arm -M virt -kernel kernel.img -nographic