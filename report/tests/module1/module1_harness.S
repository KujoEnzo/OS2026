#include <riscv.h>
.text
.option push
.option norelax
.globl module1_run
module1_run:
    csrw sie, zero
    la sp, module1_stack_top
    li a0, 0
    call module1_prepare
    csrci sstatus, 2
    li x1, 0x1122334400000101
    li x5, 0x1122334400000105
    li x6, 0x1122334400000106
    li x7, 0x1122334400000107
    li x8, 0x1122334400000108
    li x9, 0x1122334400000109
    li x10, 0x112233440000010a
    li x11, 0x112233440000010b
    li x12, 0x112233440000010c
    li x13, 0x112233440000010d
    li x14, 0x112233440000010e
    li x15, 0x112233440000010f
    li x16, 0x1122334400000110
    li x17, 0x1122334400000111
    li x18, 0x1122334400000112
    li x19, 0x1122334400000113
    li x20, 0x1122334400000114
    li x21, 0x1122334400000115
    li x22, 0x1122334400000116
    li x23, 0x1122334400000117
    li x24, 0x1122334400000118
    li x25, 0x1122334400000119
    li x26, 0x112233440000011a
    li x27, 0x112233440000011b
    li x28, 0x112233440000011c
    li x29, 0x112233440000011d
    li x30, 0x112233440000011e
    li x31, 0x112233440000011f
.globl module1_fault0
module1_fault0:
    .4byte 0x00100073
.globl module1_after0
module1_after0:
    csrw sscratch, t6
    la t6, module1_observed
    STORE x0, 0*REGBYTES(t6)
    STORE x1, 1*REGBYTES(t6)
    STORE x2, 2*REGBYTES(t6)
    STORE x3, 3*REGBYTES(t6)
    STORE x4, 4*REGBYTES(t6)
    STORE x5, 5*REGBYTES(t6)
    STORE x6, 6*REGBYTES(t6)
    STORE x7, 7*REGBYTES(t6)
    STORE x8, 8*REGBYTES(t6)
    STORE x9, 9*REGBYTES(t6)
    STORE x10, 10*REGBYTES(t6)
    STORE x11, 11*REGBYTES(t6)
    STORE x12, 12*REGBYTES(t6)
    STORE x13, 13*REGBYTES(t6)
    STORE x14, 14*REGBYTES(t6)
    STORE x15, 15*REGBYTES(t6)
    STORE x16, 16*REGBYTES(t6)
    STORE x17, 17*REGBYTES(t6)
    STORE x18, 18*REGBYTES(t6)
    STORE x19, 19*REGBYTES(t6)
    STORE x20, 20*REGBYTES(t6)
    STORE x21, 21*REGBYTES(t6)
    STORE x22, 22*REGBYTES(t6)
    STORE x23, 23*REGBYTES(t6)
    STORE x24, 24*REGBYTES(t6)
    STORE x25, 25*REGBYTES(t6)
    STORE x26, 26*REGBYTES(t6)
    STORE x27, 27*REGBYTES(t6)
    STORE x28, 28*REGBYTES(t6)
    STORE x29, 29*REGBYTES(t6)
    STORE x30, 30*REGBYTES(t6)
    csrr t0, sscratch
    STORE t0, 31*REGBYTES(t6)
    csrw sscratch, zero
    csrr t0, sstatus
    STORE t0, 32*REGBYTES(t6)
    csrr t0, sepc
    STORE t0, 33*REGBYTES(t6)
    csrr t0, sscratch
    STORE t0, 34*REGBYTES(t6)
    csrr t0, scause
    STORE t0, 35*REGBYTES(t6)
    call module1_post
    la sp, module1_stack_top
    li a0, 1
    call module1_prepare
    csrci sstatus, 2
    csrsi sstatus, 2
    li x1, 0x1122334400000101
    li x5, 0x1122334400000105
    li x6, 0x1122334400000106
    li x7, 0x1122334400000107
    li x8, 0x1122334400000108
    li x9, 0x1122334400000109
    li x10, 0x112233440000010a
    li x11, 0x112233440000010b
    li x12, 0x112233440000010c
    li x13, 0x112233440000010d
    li x14, 0x112233440000010e
    li x15, 0x112233440000010f
    li x16, 0x1122334400000110
    li x17, 0x1122334400000111
    li x18, 0x1122334400000112
    li x19, 0x1122334400000113
    li x20, 0x1122334400000114
    li x21, 0x1122334400000115
    li x22, 0x1122334400000116
    li x23, 0x1122334400000117
    li x24, 0x1122334400000118
    li x25, 0x1122334400000119
    li x26, 0x112233440000011a
    li x27, 0x112233440000011b
    li x28, 0x112233440000011c
    li x29, 0x112233440000011d
    li x30, 0x112233440000011e
    li x31, 0x112233440000011f
.globl module1_fault1
module1_fault1:
    .2byte 0x9002
.globl module1_after1
module1_after1:
    csrw sscratch, t6
    la t6, module1_observed
    STORE x0, 0*REGBYTES(t6)
    STORE x1, 1*REGBYTES(t6)
    STORE x2, 2*REGBYTES(t6)
    STORE x3, 3*REGBYTES(t6)
    STORE x4, 4*REGBYTES(t6)
    STORE x5, 5*REGBYTES(t6)
    STORE x6, 6*REGBYTES(t6)
    STORE x7, 7*REGBYTES(t6)
    STORE x8, 8*REGBYTES(t6)
    STORE x9, 9*REGBYTES(t6)
    STORE x10, 10*REGBYTES(t6)
    STORE x11, 11*REGBYTES(t6)
    STORE x12, 12*REGBYTES(t6)
    STORE x13, 13*REGBYTES(t6)
    STORE x14, 14*REGBYTES(t6)
    STORE x15, 15*REGBYTES(t6)
    STORE x16, 16*REGBYTES(t6)
    STORE x17, 17*REGBYTES(t6)
    STORE x18, 18*REGBYTES(t6)
    STORE x19, 19*REGBYTES(t6)
    STORE x20, 20*REGBYTES(t6)
    STORE x21, 21*REGBYTES(t6)
    STORE x22, 22*REGBYTES(t6)
    STORE x23, 23*REGBYTES(t6)
    STORE x24, 24*REGBYTES(t6)
    STORE x25, 25*REGBYTES(t6)
    STORE x26, 26*REGBYTES(t6)
    STORE x27, 27*REGBYTES(t6)
    STORE x28, 28*REGBYTES(t6)
    STORE x29, 29*REGBYTES(t6)
    STORE x30, 30*REGBYTES(t6)
    csrr t0, sscratch
    STORE t0, 31*REGBYTES(t6)
    csrw sscratch, zero
    csrr t0, sstatus
    STORE t0, 32*REGBYTES(t6)
    csrr t0, sepc
    STORE t0, 33*REGBYTES(t6)
    csrr t0, sscratch
    STORE t0, 34*REGBYTES(t6)
    csrr t0, scause
    STORE t0, 35*REGBYTES(t6)
    call module1_post
    la sp, module1_stack_top
    li a0, 2
    call module1_prepare
    csrci sstatus, 2
    li x1, 0x1122334400000101
    li x5, 0x1122334400000105
    li x6, 0x1122334400000106
    li x7, 0x1122334400000107
    li x8, 0x1122334400000108
    li x9, 0x1122334400000109
    li x10, 0x112233440000010a
    li x11, 0x112233440000010b
    li x12, 0x112233440000010c
    li x13, 0x112233440000010d
    li x14, 0x112233440000010e
    li x15, 0x112233440000010f
    li x16, 0x1122334400000110
    li x17, 0x1122334400000111
    li x18, 0x1122334400000112
    li x19, 0x1122334400000113
    li x20, 0x1122334400000114
    li x21, 0x1122334400000115
    li x22, 0x1122334400000116
    li x23, 0x1122334400000117
    li x24, 0x1122334400000118
    li x25, 0x1122334400000119
    li x26, 0x112233440000011a
    li x27, 0x112233440000011b
    li x28, 0x112233440000011c
    li x29, 0x112233440000011d
    li x30, 0x112233440000011e
    li x31, 0x112233440000011f
.globl module1_fault2
module1_fault2:
    .4byte 0x00100073
.globl module1_after2
module1_after2:
    csrw sscratch, t6
    la t6, module1_observed
    STORE x0, 0*REGBYTES(t6)
    STORE x1, 1*REGBYTES(t6)
    STORE x2, 2*REGBYTES(t6)
    STORE x3, 3*REGBYTES(t6)
    STORE x4, 4*REGBYTES(t6)
    STORE x5, 5*REGBYTES(t6)
    STORE x6, 6*REGBYTES(t6)
    STORE x7, 7*REGBYTES(t6)
    STORE x8, 8*REGBYTES(t6)
    STORE x9, 9*REGBYTES(t6)
    STORE x10, 10*REGBYTES(t6)
    STORE x11, 11*REGBYTES(t6)
    STORE x12, 12*REGBYTES(t6)
    STORE x13, 13*REGBYTES(t6)
    STORE x14, 14*REGBYTES(t6)
    STORE x15, 15*REGBYTES(t6)
    STORE x16, 16*REGBYTES(t6)
    STORE x17, 17*REGBYTES(t6)
    STORE x18, 18*REGBYTES(t6)
    STORE x19, 19*REGBYTES(t6)
    STORE x20, 20*REGBYTES(t6)
    STORE x21, 21*REGBYTES(t6)
    STORE x22, 22*REGBYTES(t6)
    STORE x23, 23*REGBYTES(t6)
    STORE x24, 24*REGBYTES(t6)
    STORE x25, 25*REGBYTES(t6)
    STORE x26, 26*REGBYTES(t6)
    STORE x27, 27*REGBYTES(t6)
    STORE x28, 28*REGBYTES(t6)
    STORE x29, 29*REGBYTES(t6)
    STORE x30, 30*REGBYTES(t6)
    csrr t0, sscratch
    STORE t0, 31*REGBYTES(t6)
    csrw sscratch, zero
    csrr t0, sstatus
    STORE t0, 32*REGBYTES(t6)
    csrr t0, sepc
    STORE t0, 33*REGBYTES(t6)
    csrr t0, sscratch
    STORE t0, 34*REGBYTES(t6)
    csrr t0, scause
    STORE t0, 35*REGBYTES(t6)
    call module1_post
    call module1_finish
.globl module1_done
module1_done:
    j module1_done
.option pop
.section .bss
.balign 16
.globl module1_stack_bottom
module1_stack_bottom:
    .space 8192
.globl module1_stack_top
module1_stack_top:
    .space 16
.balign 16
    .space 8192
.globl module1_alt_top
module1_alt_top:
    .space 16
.balign 16
.globl module1_observed
module1_observed:
    .space 36*REGBYTES
