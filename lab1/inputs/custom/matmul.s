# compiled with https://godbolt.org, https://stackoverflow.com/a/63386888
# Step 1 compile C to ASM: mips gcc 15.2.0 with flags -xc -O3 -march=mips32 -Wall -fverbose-asm -fno-delayed-branch
# Step 2 modify assembly: 1: add .text with j main, 2. replace j $31 with syscall
# Step 3 Assemble to Hex machine code using MARS simulator

.text
        j main

main:
        li      $3,196608             # 0x30000        #,
        ori     $3,$3,0x8  #,,
        subu    $sp,$sp,$3       #,,
        addiu   $2,$sp,8       # tmp256,,
        li      $15,65536             # 0x10000        # tmp223,
        addu    $15,$2,$15       # ivtmp.36, tmp256, tmp223
        move    $5,$2    # tmp257, tmp256
        li      $3,131072             # 0x20000        # tmp224,
        move    $4,$15   # ivtmp.36, ivtmp.36
        move    $2,$0    # i,
        addu    $3,$5,$3         # tmp225, tmp257, tmp224
        li      $7,16384                    # 0x4000         # tmp229,
$L2:
        sll     $6,$2,2    # tmp226, i,
        addu    $6,$3,$6         # tmp227, tmp225, tmp226
        sw      $2,0($4)     # i, MEM[(int *)_83]
        subu    $8,$7,$2         # _86, tmp229, i
        addiu   $2,$2,1        # i, i,
        sw      $8,0($5)     # _86, MEM[(int *)_87]
        sw      $0,0($6)     #, mat[i_39]
        addiu   $4,$4,4        # ivtmp.36, ivtmp.36,
        addiu   $5,$5,4        # ivtmp.37, ivtmp.37,
        bne     $2,$7,$L2
        nop
        li      $12,131072              # 0x20000        # tmp231,
        addiu   $2,$sp,8       # tmp259,,
        addiu   $8,$15,512     # ivtmp.29, ivtmp.36,
        move    $11,$0   # ivtmp.28,
        addu    $12,$2,$12       # tmp232, tmp259, tmp231
        li      $14,128           # 0x80   # tmp242,
        li      $24,16384             # 0x4000         # tmp252,
$L3:
        sll     $13,$11,2  # _76, ivtmp.28,
        addu    $13,$13,$15      # ivtmp.12, _76, ivtmp.36
        addiu   $10,$sp,8      # tmp260,,
        move    $9,$0    # j,
$L7:
        addu    $5,$9,$11        # _3, j, ivtmp.28
        sll     $5,$5,2    # tmp250, _3,
        move    $4,$10   # ivtmp.13, ivtmp.20
        move    $2,$13   # ivtmp.12, ivtmp.12
        addu    $5,$12,$5        # tmp234, tmp232, tmp250
$L4:
        lw      $3,0($2)             # MEM[(int *)_55], MEM[(int *)_55]
        lw      $7,0($4)             # MEM[(int *)_56], MEM[(int *)_56]
        lw      $6,0($5)             # _4, mat[_3]
        mul     $25,$3,$7  # tmp255, MEM[(int *)_55], MEM[(int *)_56]
        addiu   $2,$2,4        # ivtmp.12, ivtmp.12,
        addiu   $4,$4,512      # ivtmp.13, ivtmp.13,
        addu    $3,$25,$6        # _11, tmp255, _4
        sw      $3,0($5)     # _11, mat[_3]
        bne     $2,$8,$L4
        nop
        addiu   $9,$9,1        # j, j,
        addiu   $10,$10,4      # ivtmp.20, ivtmp.20,
        bne     $9,$14,$L7
        nop
        addiu   $11,$11,128    # ivtmp.28, ivtmp.28,
        addiu   $8,$2,512      # ivtmp.29, ivtmp.12,
        bne     $11,$24,$L3
        nop
        li      $8,196608             # 0x30000        #,
        ori     $8,$8,0x8  #,,
        move    $2,$0    #,
        addu    $sp,$sp,$8       #,,
li $v0, 10
syscall