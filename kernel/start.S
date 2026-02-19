.global _start

_start:
    ldr sp, =0x40000000 + 0x100000
    bl main

hang:
    b hang