#!/bin/bash

# Usage: ./run.sh [uart|vnc|gtk]
# uart: UART output (default, headless)
# vnc: Graphics via VNC
# gtk: Graphics via GTK if available

MODE=${1:-uart}

case $MODE in
    uart)
        echo "Running with UART output..."
        qemu-system-arm -M virt -kernel kernel.img -nographic
        ;;
    vnc)
        echo "Running with VNC graphics..."
        qemu-system-arm -M virt -kernel kernel.img -vnc :0
        ;;
    gtk)
        echo "Running with GTK graphics..."
        qemu-system-arm -M virt -kernel kernel.img -display gtk
        ;;
    *)
        echo "Invalid mode. Use uart, vnc, or gtk."
        exit 1
        ;;
esac