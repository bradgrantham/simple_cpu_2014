// Vector table
.org 0
        jmp reset
        jmp exIllegal
        jmp exUnaligned
        jmp exBadAccess

reset:  moviu r0, 0x0000
        moviu r1, 0x0000
        moviu r3, 0x0100
        shift.la r3, 1
        addi sp, 0x80           // stack pointer starts at 0x80

loop:   store.short r0, r1
        load.short  r2, r0
        cmp r2, r1
        jne badMem
        addi r0, 2
        addi r1, 2
        cmpiu r0, 0x800000
        jl loop

test:   // some simple instruction tests
        moviu r3, 0x1234
        addi r3, 0x5678         // r3 = 0x12345678
        push r3
        pop r4                  // r4 = 0x12345678
        store.word r5, r3       // *(short*)r5 = 0x5678
        load.word r6, r5        // r6 = *(r5) = 0x5678
        mult r0, r3
        hlt

badMem:
        moviu r0, 0xbaad
        addi r0, 0xbeef
        hlt

exIllegal: // Illegal instruction 0x300
        moviu r0, 0x177e
        addi r0, 0x6a70
        hlt

exUnaligned: // Unaligned access 0x310
        moviu r0, 0xacce
        addi r0, 0x55ed
        hlt

exBadAccess: // Bad access 0x320
        moviu r0, 0xbada
        addi r0, 0xcce5
        hlt

        moviu r0, 0xffffffff // should be out of range

        .define  large 0xf0000000
        jmp large // should be out of range

        .define  lsbs 0x00000001
        jmp lsbs // should truncate

        .define cafebabe 0xCAFEBABE
        moviu r1, cafebabe.hi
        addiu r1, cafebabe.lo

        .define deadbeef 0xDEADBEEF
        assign r0, deadbeef

l255:   .byte 0xff, 0x00, 0x00
s1:     .short 0x1234
w1:     .word  0x12345678
w2:     .word  l255
w3:     .word  endoffile // test reference of label at end of file
w1:     .word  w1       // test detection of redefinition

        .define a b
        .define b a
w4:     .word  a        // test detection of circular expressions

hello:  .string "hello world"
endoffile:
