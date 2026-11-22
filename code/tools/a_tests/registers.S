/* Distinct full-width sentinels per thread detect RV64 truncation and cross-thread leakage. */
.text
.globl a_test_registers
a_test_registers:
    addi sp, sp, -112
    sd ra, 0(sp)
    sd a0, 104(sp)
    sd s0, 8(sp)
    sd s1, 16(sp)
    sd s2, 24(sp)
    sd s3, 32(sp)
    sd s4, 40(sp)
    sd s5, 48(sp)
    sd s6, 56(sp)
    sd s7, 64(sp)
    sd s8, 72(sp)
    sd s9, 80(sp)
    sd s10, 88(sp)
    sd s11, 96(sp)
    li t0, 0x1020304050607000
    add t0, t0, a0
    li s0, 101
    add s0, s0, t0
    li s1, 102
    add s1, s1, t0
    li s2, 103
    add s2, s2, t0
    li s3, 104
    add s3, s3, t0
    li s4, 105
    add s4, s4, t0
    li s5, 106
    add s5, s5, t0
    li s6, 107
    add s6, s6, t0
    li s7, 108
    add s7, s7, t0
    li s8, 109
    add s8, s8, t0
    li s9, 110
    add s9, s9, t0
    li s10, 111
    add s10, s10, t0
    li s11, 112
    add s11, s11, t0
    call schedule
    ld t1, 104(sp)
    li t0, 0x1020304050607000
    add t1, t1, t0
    li t0, 101
    add t0, t0, t1
    bne s0, t0, 1f
    li t0, 102
    add t0, t0, t1
    bne s1, t0, 1f
    li t0, 103
    add t0, t0, t1
    bne s2, t0, 1f
    li t0, 104
    add t0, t0, t1
    bne s3, t0, 1f
    li t0, 105
    add t0, t0, t1
    bne s4, t0, 1f
    li t0, 106
    add t0, t0, t1
    bne s5, t0, 1f
    li t0, 107
    add t0, t0, t1
    bne s6, t0, 1f
    li t0, 108
    add t0, t0, t1
    bne s7, t0, 1f
    li t0, 109
    add t0, t0, t1
    bne s8, t0, 1f
    li t0, 110
    add t0, t0, t1
    bne s9, t0, 1f
    li t0, 111
    add t0, t0, t1
    bne s10, t0, 1f
    li t0, 112
    add t0, t0, t1
    bne s11, t0, 1f
    li a0, 0
    j 2f
1:
    li a0, 1
2:
    ld ra, 0(sp)
    ld s0, 8(sp)
    ld s1, 16(sp)
    ld s2, 24(sp)
    ld s3, 32(sp)
    ld s4, 40(sp)
    ld s5, 48(sp)
    ld s6, 56(sp)
    ld s7, 64(sp)
    ld s8, 72(sp)
    ld s9, 80(sp)
    ld s10, 88(sp)
    ld s11, 96(sp)
    addi sp, sp, 112
    ret
