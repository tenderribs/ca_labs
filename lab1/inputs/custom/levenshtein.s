# compiled with https://godbolt.org, https://stackoverflow.com/a/63386888
# Step 1 compile C to ASM: mips gcc 15.2.0 with flags -xc -O3 -march=mips32 -Wall -fverbose-asm -fno-delayed-branch
# Step 2 modify assembly: 1: add .text with j main, 2. replace j $31 with syscall
# Step 3 Assemble to Hex machine code using MARS simulator

.text
        j main

mmin:
        slt     $2,$6,$5   # tmp202, c, b
        movn    $5,$6,$2         #, _8, c, tmp202
        slt     $2,$4,$5   # tmp203, a, _8
        movz    $4,$5,$2         #, a, _8, tmp203
        move    $2,$4    #, a
        jr      $31
        nop
lcg:
        addiu   $sp,$sp,-16    #,,
        li      $3,1638400              # 0x190000       # tmp201,
        addiu   $3,$3,26125    # tmp200, tmp201,
        sw      $3,8($sp)    # tmp200, %sfp
        lw      $2,0($4)             # *state_5(D), *state_5(D)
        lw      $5,8($sp)          # tmp200, %sfp
        li      $3,1013841920                 # 0x3c6e0000     # tmp203,
        mul     $6,$2,$5   # tmp208, *state_5(D), tmp200
        ori     $3,$3,0xf35f       # tmp202, tmp203,
        addiu   $sp,$sp,16     #,,
        addu    $2,$6,$3         # <retval>, tmp208, tmp202
        sw      $2,0($4)     # <retval>, *state_5(D)
        jr      $31
        nop
main:
        li      $3,4194304              # 0x400000       #,
        ori     $3,$3,0x818        #,,
        subu    $sp,$sp,$3       #,,
        addiu   $3,$sp,8       # tmp294,,
        li      $2,4194304              # 0x400000       # tmp233,
        addu    $12,$3,$2        # tmp234, tmp294, tmp233
        addiu   $2,$2,1024     # tmp235, tmp233,
        addu    $11,$3,$2        # ivtmp.44, tmp295, tmp235
        li      $3,4194304              # 0x400000       # tmp306,
        ori     $3,$3,0x810        # tmp305, tmp306,
        li      $2,1638400              # 0x190000       # tmp241,
        addu    $3,$3,$sp        # tmp305, tmp305,
        addiu   $2,$2,26125    # tmp240, tmp241,
        li      $9,1013841920                 # 0x3c6e0000     # tmp243,
        li      $8,-859045888                 # 0xffffffffcccc0000     # tmp255,
        li      $7,429457408                        # 0x19990000     # tmp263,
        addiu   $14,$11,1024   # _122, ivtmp.44,
        sw      $0,2048($12)         #, edit_distance
        move    $6,$12   # ivtmp.53, tmp234
        move    $4,$11   # ivtmp.52, ivtmp.44
        li      $5,1337           # 0x539  # seed,
        li      $13,65                  # 0x41   # tmp238,
        sw      $2,0($3)     # tmp240, %sfp
        ori     $9,$9,0xf35f       # tmp242, tmp243,
        ori     $8,$8,0xcccd       # tmp254, tmp255,
        ori     $7,$7,0x999a       # tmp262, tmp263,
        li      $10,67                  # 0x43   # tmp288,
$L7:
        li      $2,4194304              # 0x400000       # tmp303,
        ori     $2,$2,0x810        # tmp302, tmp303,
        addu    $2,$2,$sp        # tmp302, tmp302,
        addiu   $6,$6,1        # ivtmp.53, ivtmp.53,
        lw      $2,0($2)             # tmp240, %sfp
        sb      $13,0($4)    # tmp238, MEM[(char *)_119]
        mul     $3,$5,$2   # tmp292, seed, tmp240
        addiu   $4,$4,1        # ivtmp.52, ivtmp.52,
        addu    $5,$3,$9         # seed, tmp292, tmp242
        mul     $3,$5,$8   # tmp253, seed, tmp254
        srl     $2,$3,1    # tmp256, tmp253,
        sll     $3,$3,31   # tmp257, tmp253,
        addu    $2,$2,$3         # tmp258, tmp256, tmp257
        sltu    $2,$2,$7         # tmp261, tmp258, tmp262
        li      $3,65                 # 0x41   # cstore_109,
        movn    $3,$10,$2        #, cstore_109, tmp288, tmp261
        sb      $3,-1($6)    # cstore_109, MEM <char> [(void *)_120]
        bne     $4,$14,$L7
        nop
        addiu   $10,$sp,8      # tmp299,,
        sw      $0,8($sp)    #, D[0]
        addiu   $3,$sp,12      # ivtmp.24,,
        li      $2,1                        # 0x1    # j,
        li      $4,1024           # 0x400  # tmp275,
$L8:
        sw      $2,0($3)     # j, MEM[(unsigned int *)_46]
        addiu   $2,$2,1        # j, j,
        addiu   $3,$3,4        # ivtmp.24, ivtmp.24,
        bne     $2,$4,$L8
        nop
        move    $13,$0   # i,
        li      $14,1024                    # 0x400  # tmp272,
        addiu   $13,$13,1      # i, i,
        addiu   $7,$12,1024    # _24, tmp234,
        beq     $13,$14,$L11
        nop
$L18:
        lw      $2,0($10)          # MEM[(unsigned int *)_17 + 4294963200B], MEM[(unsigned int *)_17 + 4294963200B]
        addiu   $10,$10,4096   # ivtmp.41, ivtmp.41,
        addiu   $2,$2,1        # _31, MEM[(unsigned int *)_17 + 4294963200B],
        addiu   $11,$11,1      # ivtmp.44, ivtmp.44,
        sw      $2,0($10)    # _31, MEM[(unsigned int *)_17]
        lb      $6,0($11)          # seq1_I_lsm0.16, MEM[(char *)_16]
        addiu   $3,$12,1       # ivtmp.33, tmp234,
        move    $2,$10   # ivtmp.35, ivtmp.41
$L10:
        lb      $5,0($3)             # MEM[(char *)_21], MEM[(char *)_21]
        lw      $4,-4096($2)                 # _73, MEM[(unsigned int *)_82 + 4294963200B]
        addiu   $3,$3,1        # ivtmp.33, ivtmp.33,
        beq     $5,$6,$L9
        nop
        lw      $5,-4092($2)                 # MEM[(unsigned int *)_82 + 4294963204B], MEM[(unsigned int *)_82 + 4294963204B]
        lw      $8,0($2)             # MEM[(unsigned int *)_82], MEM[(unsigned int *)_82]
        slt     $9,$4,$5   # tmp269, _73, MEM[(unsigned int *)_82 + 4294963204B]
        movz    $4,$5,$9         #, _63, MEM[(unsigned int *)_82 + 4294963204B], tmp269
        slt     $5,$8,$4   # tmp271, MEM[(unsigned int *)_82], _63
        movn    $4,$8,$5         #, _40, MEM[(unsigned int *)_82], tmp271
        addiu   $4,$4,1        # _73, _40,
$L9:
        sw      $4,4($2)     # _73, MEM[(unsigned int *)_82 + 4B]
        addiu   $2,$2,4        # ivtmp.35, ivtmp.35,
        bne     $7,$3,$L10
        nop
        addiu   $13,$13,1      # i, i,
        bne     $13,$14,$L18
        nop
$L11:
        li      $2,4194304              # 0x400000       # tmp276,
        addiu   $3,$sp,8       # tmp300,,
        addu    $3,$3,$2         # tmp277, tmp300, tmp276
        li      $8,4194304              # 0x400000       #,
        lw      $4,-4($3)          # _36, D[1048575]
        ori     $8,$8,0x818        #,,
        move    $2,$0    #,
        sw      $4,2048($3)  # _36, edit_distance
        addu    $sp,$sp,$8       #,,
li $v0, 10
syscall