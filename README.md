# Microkernel OS for QEMU Raspberry Pi

This is a microkernel-based operating system targeting QEMU emulation of Raspberry Pi 2/3 and future porting to ARM phones.

## Phase 1: Graphical Interface and Window Manager

Current implementation includes:
- Minimal kernel with UART output
- Framebuffer initialization via mailbox
- Basic graphics primitives (pixel, rect, line)
- Simple window drawing

## Building

Install dependencies:
- gcc-arm-none-eabi
- binutils-arm-none-eabi
- qemu-system-arm
- make

Run `make` to build `kernel.img`.

## Running

Run `./scripts/run.sh` for graphics display (requires local machine with GUI).

For UART output only (works in headless environments like Codespace):
```
qemu-system-arm -M raspi2b -kernel kernel.img -nographic
```

If display fails, ensure QEMU has GUI support or use VNC:
```
qemu-system-arm -M raspi2b -kernel kernel.img -vnc :0
```
Then connect with a VNC viewer.

## Architecture

- `kernel/`: Core kernel code
- `drivers/`: Device drivers (framebuffer, input - TODO)
- `wm/`: Window manager (TODO)
- `apps/`: User applications (TODO)
- `scripts/`: Run scripts
- `docs/`: Documentation

## TODO for Phase 1
- Input handling (mouse, touchscreen)
- IPC for user processes
- Compositor with damage tracking
- Demo applications