.section ".text"

.global _start

_start:
    eor     x0, x0, x0
    eor     x1, x1, x1
    mov     x0, 0
1:
    add     x0, x0, 1
    svc     0
    cmp     x0, 5
    blt     1b
1:
    b       1b