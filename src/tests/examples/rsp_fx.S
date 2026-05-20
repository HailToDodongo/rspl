## Auto-generated file, transpiled with RSPL
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define SCREEN_STRIDE (320 * 4)
#define BUFF_SIZE_RGBA16 (320 * 240 * 2)
#define BUFF_SIZE_RGBA32 (320 * 240 * 4)
#define BLUR_WIDTH 80
#define BLUR_HEIGHT 60
#define BLUR_STRIDE (80 * 4)
#define BLUR_STRIDE_PADDED (80 * 4 + 32)
#include <rsp_queue.inc>

.set noreorder
.set noat
.set nomacro

#undef zero
#undef at
#undef v0
#undef v1
#undef a0
#undef a1
#undef a2
#undef a3
#undef t0
#undef t1
#undef t2
#undef t3
#undef t4
#undef t5
#undef t6
#undef t7
#undef s0
#undef s1
#undef s2
#undef s3
#undef s4
#undef s5
#undef s6
#undef s7
#undef t8
#undef t9
#undef k0
#undef k1
#undef gp
#undef sp
#undef fp
#undef ra
.equ hex.$zero, 0
.equ hex.$at, 1
.equ hex.$v0, 2
.equ hex.$v1, 3
.equ hex.$a0, 4
.equ hex.$a1, 5
.equ hex.$a2, 6
.equ hex.$a3, 7
.equ hex.$t0, 8
.equ hex.$t1, 9
.equ hex.$t2, 10
.equ hex.$t3, 11
.equ hex.$t4, 12
.equ hex.$t5, 13
.equ hex.$t6, 14
.equ hex.$t7, 15
.equ hex.$s0, 16
.equ hex.$s1, 17
.equ hex.$s2, 18
.equ hex.$s3, 19
.equ hex.$s4, 20
.equ hex.$s5, 21
.equ hex.$s6, 22
.equ hex.$s7, 23
.equ hex.$t8, 24
.equ hex.$t9, 25
.equ hex.$k0, 26
.equ hex.$k1, 27
.equ hex.$gp, 28
.equ hex.$sp, 29
.equ hex.$fp, 30
.equ hex.$ra, 31
#define vco 0
#define vcc 1
#define vce 2

.data
  RSPQ_BeginOverlayHeader
    RSPQ_DefineCommand Cmd_HDRBloom, 16
    RSPQ_DefineCommand Cmd_Blur, 12
    RSPQ_DefineCommand Cmd_Downscale, 8
  RSPQ_EndOverlayHeader

  RSPQ_EmptySavedState

.bss
  TEMP_STATE_MEM_START:
    .align 2
    BUFF_IN_A: .ds.b 1280
    .align 2
    BUFF_IN_B: .ds.b 1280
    .align 3
    SAFE_SPACE_0: .ds.b 16
    .align 2
    BUFF_BLOOM: .ds.b 320
    .align 4
    SAFE_SPACE_1: .ds.b 16
    .align 4
    SAFE_SPACE_2: .ds.b 16
    .align 2
    BUFF_BLOOM_B: .ds.b 320
    .align 4
    SAFE_SPACE_3: .ds.b 16
  TEMP_STATE_MEM_END:

.text
OVERLAY_CODE_START:

Cmd_HDRBloom:
  addiu $at, $zero, 31744                            ## L:72   |      ^ | VCONST_RGB.x = 0b0'11111'00000'00000;
  mtc2 $at, $v28.e0                                  ## L:72   |      2 | VCONST_RGB.x = 0b0'11111'00000'00000;
  lui $at, 0x04                                      ## L:74   |      3 | u32 ptrInEnd = ptrIn + (320 * 240 * 4);
  ori $at, $at, 0xB000                               ## L:74   |      4 | u32 ptrInEnd = ptrIn + (320 * 240 * 4);
  addu $s7, $a0, $at                                 ## L:74   |      5 | u32 ptrInEnd = ptrIn + (320 * 240 * 4);
  ori $s6, $zero, %lo(BUFF_IN_A)                     ## L:75   |      6 | u16 buffDMEM = BUFF_IN_A;
  or $s5, $zero, $zero                               ## L:78   |      7 | u8 bloomPhase = 0;
  vxor $v27, $v00, $v00.e0                           ## L:79   |      ^ | vec16 bloomFactorY = 0;
  vxor $v26, $v00, $v00.e0                           ## L:81   |      8 | vec16 bloomFactorA = 0;
  addiu $at, $zero, 8192                             ## L:82   |      ^ | bloomFactorA:sfract.X = 0.25;
  mtc2 $at, $v26.e4                                  ## L:82   |      9 | bloomFactorA:sfract.X = 0.25;
  vmov $v26.e5, $v26.e4                              ## L:83   |  ***13 | bloomFactorA.Y = bloomFactorA.X;
  vmov $v26.e6, $v26.e4                              ## L:84   |  ***17 | bloomFactorA.Z = bloomFactorA.X;
  srl $s4, $a3, 16                                   ## L:86   |      ^ | u16 factorU16 = factor >> 16;
  LABEL_Cmd_HDRBloom_0001:
  mfc0 $ra, COP0_DMA_BUSY                            ## L:91   |     18 | RA = get_dma_busy();
  mtc2 $s4, $v26.e3                                  ## L:93   |     19 | bloomFactorA.w = factorU16;
  mtc2 $a3, $v26.e7                                  ## L:94   |   **22 | bloomFactorA.W = factor:u16;
  bne $ra, $zero, LABEL_Cmd_HDRBloom_0001            ## L:94   |     23 | bloomFactorA.W = factor:u16;
  or $s3, $zero, $zero                               ## L:92   |    *25 | sumRGB = 0;
  LABEL_Cmd_HDRBloom_0003:
  addiu $s2, $zero, 1279                             ## L:100  |     26 | u32 dmaSize = (320 * 4) - 1;
  or $s1, $zero, $s6                                 ## L:101  |     27 | u32 ptrDMEM = buffDMEM;
  mtc0 $s1, COP0_DMA_SPADDR                          ## L:32   |     28 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $a0, COP0_DMA_RAMADDR                         ## L:33   |     29 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  ori $s0, $zero, %lo(BUFF_BLOOM)                    ## L:105  |     30 | u16 dmemBloom = BUFF_BLOOM;
  bne $s5, $zero, LABEL_Cmd_HDRBloom_0005            ## L:110  |     31 | if(bloomPhase == 0) {
  mtc0 $s2, COP0_DMA_READ                            ## L:34   |    *33 | @Barrier("DMA") set_dma_read(size); ## Barrier: 0x1
  LABEL_Cmd_HDRBloom_0006:
  mfc0 $ra, COP0_DMA_BUSY                            ## L:113  |    *35 | RA = get_dma_busy();
  vsubc $v27, $v00, $v31.e2                          ## L:115  |      ^ | bloomFactorY = VZERO - 0x2000;
  addiu $fp, $zero, 640                              ## L:116  |     36 | bloomDmaSize = (80 * 4)*2;
  bne $ra, $zero, LABEL_Cmd_HDRBloom_0006            ## L:116  |    *38 | bloomDmaSize = (80 * 4)*2;
  addiu $s5, $zero, 4                                ## L:114  |    *40 | bloomPhase = 4;
  LABEL_Cmd_HDRBloom_0007:
  mtc0 $s0, COP0_DMA_SPADDR                          ## L:32   |     41 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $a2, COP0_DMA_RAMADDR                         ## L:33   |     42 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  mtc0 $fp, COP0_DMA_READ                            ## L:34   |   **45 | @Barrier("DMA") set_dma_read(size); ## Barrier: 0x1
  addiu $a2, $a2, 320                                ## L:120  |     46 | ptrBloom += (80 * 4);
  LABEL_Cmd_HDRBloom_0005:
  addiu $s5, $s5, 65535                              ## L:123  |     47 | bloomPhase -= 1;
  addiu $a0, $a0, 1280                               ## L:124  |     48 | ptrIn += (320 * 4);
  vaddc $v27, $v27, $v31.e2                          ## L:129  |      ^ | bloomFactorY += 0x2000;
  LABEL_Cmd_HDRBloom_0008:
  mfc0 $ra, COP0_DMA_BUSY                            ## L:132  |     49 | RA = get_dma_busy();
  or $fp, $zero, $s1                                 ## L:134  |     50 | dmemIn = ptrDMEM;
  addiu $k1, $s1, 1280                               ## L:135  |     51 | dmemInEnd = ptrDMEM + 320*4;
  bne $ra, $zero, LABEL_Cmd_HDRBloom_0008            ## L:135  |     52 | dmemInEnd = ptrDMEM + 320*4;
  ori $sp, $zero, %lo(BUFF_IN_B)                     ## L:133  |    *54 | dmemOut = BUFF_IN_B;
  LABEL_Cmd_HDRBloom_0009:
  ori $k0, $zero, %lo(RSPQ_SCRATCH_MEM)              ## L:141  |     55 | u16 tmpMemA = RSPQ_SCRATCH_MEM;
  addiu $t9, $k0, 1                                  ## L:142  |     56 | u16 tmpMemB = tmpMemA + 1;
  addiu $t8, $k0, 2                                  ## L:143  |     57 | u16 tmpMemC = tmpMemA + 2;
  luv $v25, 0, 0, $s0                                ## L:148  |     58 | bloom0 = load_vec_u8(dmemBloom, 0);
  luv $v21, 0, 320, $s0                              ## L:149  |     59 | bloom0Next = load_vec_u8(dmemBloom, 320);
  luv $v23, 0, 8, $s0                                ## L:151  |     60 | bloom2 = load_vec_u8(dmemBloom, 8);
  luv $v20, 0, 328, $s0                              ## L:152  |     61 | bloom2Next = load_vec_u8(dmemBloom, 328);
  addiu $s0, $s0, 8                                  ## L:154  |    *63 | dmemBloom += 8;
  vsubc $v15, $v21, $v25.v                           ## L:156  |      ^ | vec16 diffY_A = bloom0Next - bloom0;
  vsubc $v14, $v20, $v23.v                           ## L:157  |    *65 | vec16 diffY_B = bloom2Next - bloom2;
  luv $v13, 0, 0, $fp                                ## L:159  |      ^ | vec16 pixel0 = load_vec_u8(dmemIn, 0);
  vmudm $v29, $v13, $v26.e7                          ## L:55   |  ***69 | asm_op("vmudm", VTEMP, pixel, bloomFactorA.W);
  vmadh $v13, $v13, $v26.e3                          ## L:56   |     70 | asm_op("vmadh", pixel, pixel, bloomFactorA.w);
  LABEL_Cmd_HDRBloom_000A:
  luv $v12, 0, 8, $fp                                ## L:164  |      ^ | vec16 pixel1 = load_vec_u8(dmemIn, 8);
  luv $v11, 0, 16, $fp                               ## L:165  |     71 | vec16 pixel2 = load_vec_u8(dmemIn, 16);
  luv $v10, 0, 24, $fp                               ## L:166  |     72 | vec16 pixel3 = load_vec_u8(dmemIn, 24);
  vmudn $v19, $v18, $v30.e6                          ## L:170  |      ^ | pixelOut = pixelR:uint * 2;
  vmadl $v19, $v17, $v31.e3                          ## L:171  |     73 | asm_op("vmadl", pixelOut, pixelG, VSHIFT8.w);
  vmadl $v19, $v16, $v30.e0                          ## L:172  |     74 | asm_op("vmadl", pixelOut, pixelB, VSHIFT.x);
  vmudh $v25, $v25, $v30.e7                          ## L:175  |     75 | bloom0:sint = bloom0:sint * 1;
  vmacf $v25, $v15, $v27.v                           ## L:176  |   **78 | bloom0:sfract = diffY_A:sfract +* bloomFactorY:sfract;
  sqv $v19, 0, -16, $sp                              ## L:179  |      ^ | store(pixelOut, dmemOut, -16);
  vmudh $v23, $v23, $v30.e7                          ## L:181  |     79 | bloom2:sint = bloom2:sint * 1;
  vmacf $v23, $v14, $v27.v                           ## L:182  |     80 | bloom2:sfract = diffY_B:sfract +* bloomFactorY:sfract;
  vmudm $v29, $v12, $v26.e7                          ## L:55   |     81 | asm_op("vmudm", VTEMP, pixel, bloomFactorA.W);
  vmadh $v12, $v12, $v26.e3                          ## L:56   |     82 | asm_op("vmadh", pixel, pixel, bloomFactorA.w);
  ori $at, $zero, %lo(RSPQ_SCRATCH_MEM)              ## L:190  |      ^ | @Barrier("tmp_0") store(bloom0, RSPQ_SCRATCH_MEM, 0); ## Barrier: 0x2
  sqv $v25, 0, 0, $at                                ## L:190  |     83 | @Barrier("tmp_0") store(bloom0, RSPQ_SCRATCH_MEM, 0); ## Barrier: 0x2
  sdv $v23, 0, 24, $at                               ## L:191  |     84 | @Barrier("tmp_2") store(bloom2.xyzw, RSPQ_SCRATCH_MEM, 24); ## Barrier: 0x4
  vor $v20, $v00, $v23                               ## L:193  |      ^ | @Barrier("tmp_2") bloom2Next = bloom2; ## Barrier: 0x4
  vor $v23, $v00, $v25                               ## L:194  |     85 | @Barrier("tmp_0") bloom2 = bloom0; ## Barrier: 0x2
  ldv $v25, 8, 0, $at                                ## L:196  |      ^ | @Barrier("tmp_0") bloom0.XYZW = load(RSPQ_SCRATCH_MEM, 0).xyzw; ## Barrier: 0x2
  ldv $v23, 0, 8, $at                                ## L:197  |     86 | @Barrier("tmp_0") bloom2.xyzw = load(RSPQ_SCRATCH_MEM, 8).xyzw; ## Barrier: 0x2
  ldv $v20, 8, 24, $at                               ## L:198  |     87 | @Barrier("tmp_2") bloom2Next.XYZW = load(RSPQ_SCRATCH_MEM, 24).xyzw; ## Barrier: 0x4
  vmudm $v29, $v11, $v26.e7                          ## L:55   |      ^ | asm_op("vmudm", VTEMP, pixel, bloomFactorA.W);
  vmadh $v11, $v11, $v26.e3                          ## L:56   |     88 | asm_op("vmadh", pixel, pixel, bloomFactorA.w);
  vsubc $v09, $v23, $v25.v                           ## L:205  |    *90 | vec16 diffX_A = bloom2 - bloom0;
  vmudm $v29, $v10, $v26.e7                          ## L:55   |     91 | asm_op("vmudm", VTEMP, pixel, bloomFactorA.W);
  vmadh $v10, $v10, $v26.e3                          ## L:56   |     92 | asm_op("vmadh", pixel, pixel, bloomFactorA.w);
  vmudh $v24, $v25, $v30.e7                          ## L:212  |     93 | bloom1:sint = bloom0:sint * 1;
  vmacf $v08, $v09, $v26.v                           ## L:213  |     94 | diffX_A_first:sfract = diffX_A:sfract +* bloomFactorA:sfract;
  vmacf $v24, $v09, $v31.e1                          ## L:214  |     95 | bloom1:sfract = diffX_A:sfract +* 0x4000;
  vmadh $v12, $v12, $v30.e7                          ## L:215  |     96 | pixel1:sint = pixel1:sint +* 1;
  vsubc $v06, $v20, $v23.v                           ## L:217  |     97 | vec16 diffX_B = bloom2Next - bloom2;
  vadd $v13, $v08, $v13.v                            ## L:220  |     98 | pixel0:sint = diffX_A_first:sint + pixel0:sint;
  vmudh $v22, $v23, $v30.e7                          ## L:222  |     99 | bloom3:sint = bloom2:sint * 1;
  vmacf $v07, $v06, $v26.v                           ## L:223  |   *101 | diffX_B_first:sfract = diffX_B:sfract +* bloomFactorA:sfract;
  vmacf $v22, $v06, $v31.e1                          ## L:224  |    102 | bloom3:sfract = diffX_B:sfract +* 0x4000;
  vmadh $v10, $v10, $v30.e7                          ## L:225  |    103 | pixel3:sint = pixel3:sint +* 1;
  vadd $v11, $v07, $v11.v                            ## L:227  |   *105 | pixel2:sint = diffX_B_first:sint + pixel2:sint;
  lbu $t7, 8($k0)                                    ## L:233  |      ^ | tmp0 = load(tmpMemA, 8);
  lbu $t6, 9($k0)                                    ## L:234  |    106 | tmp1 = load(tmpMemA, 9);
  lbu $t5, 10($k0)                                   ## L:235  |    107 | tmp2 = load(tmpMemA, 10);
  lbu $t4, 24($k0)                                   ## L:236  |    108 | tmp3 = load(tmpMemA, 24);
  lbu $t3, 25($k0)                                   ## L:237  |    109 | tmp4 = load(tmpMemA, 25);
  lbu $t2, 26($k0)                                   ## L:238  |    110 | tmp5 = load(tmpMemA, 26);
  addu $t1, $t7, $t6                                 ## L:239  |    111 | sumRGB += tmp0 + tmp1 + tmp2 + tmp3 + tmp4 + tmp5;
  addu $t1, $t1, $t5                                 ## L:239  |    112 | sumRGB += tmp0 + tmp1 + tmp2 + tmp3 + tmp4 + tmp5;
  addu $t1, $t1, $t4                                 ## L:239  |    113 | sumRGB += tmp0 + tmp1 + tmp2 + tmp3 + tmp4 + tmp5;
  addu $t1, $t1, $t3                                 ## L:239  |    114 | sumRGB += tmp0 + tmp1 + tmp2 + tmp3 + tmp4 + tmp5;
  addu $t1, $t1, $t2                                 ## L:239  |    115 | sumRGB += tmp0 + tmp1 + tmp2 + tmp3 + tmp4 + tmp5;
  suv $v13, 0, 0, $k0                                ## L:243  |    116 | @Barrier("rgba16") store_vec_u8(pixel0, tmpMemA, 0); ## Barrier: 0x8
  suv $v12, 0, 8, $k0                                ## L:244  |    117 | @Barrier("rgba16") store_vec_u8(pixel1, tmpMemA, 8); ## Barrier: 0x8
  suv $v11, 0, 16, $k0                               ## L:245  |    118 | @Barrier("rgba16") store_vec_u8(pixel2, tmpMemA, 16); ## Barrier: 0x8
  suv $v10, 0, 24, $k0                               ## L:246  |    119 | @Barrier("rgba16") store_vec_u8(pixel3, tmpMemA, 24); ## Barrier: 0x8
  luv $v13, 0, 32, $fp                               ## L:248  |    120 | pixel0 = load_vec_u8(dmemIn, 32);
  vmudm $v29, $v13, $v26.e7                          ## L:55   | ***124 | asm_op("vmudm", VTEMP, pixel, bloomFactorA.W);
  vmadh $v13, $v13, $v26.e3                          ## L:56   |    125 | asm_op("vmadh", pixel, pixel, bloomFactorA.w);
  lfv $v18, 0, 0, $k0                                ## L:255  |      ^ | asm_op("lfv", pixelR, 0,  0, tmpMemA); ## Barrier: 0x8
  lfv $v18, 8, 16, $k0                               ## L:257  |    126 | asm_op("lfv", pixelR, 8, 16, tmpMemA); ## Barrier: 0x8
  vand $v18, $v18, $v28.e0                           ## L:258  | ***130 | pixelR &= VCONST_RGB.x;
  lfv $v17, 0, 0, $t9                                ## L:261  |      ^ | asm_op("lfv", pixelG, 0,  0, tmpMemB); ## Barrier: 0x8
  lfv $v17, 8, 16, $t9                               ## L:263  |    131 | asm_op("lfv", pixelG, 8, 16, tmpMemB); ## Barrier: 0x8
  vand $v17, $v17, $v28.e0                           ## L:264  | ***135 | pixelG &= VCONST_RGB.x;
  lfv $v16, 0, 0, $t8                                ## L:267  |      ^ | asm_op("lfv", pixelB, 0,  0, tmpMemC); ## Barrier: 0x8
  lfv $v16, 8, 16, $t8                               ## L:269  |    136 | asm_op("lfv", pixelB, 8, 16, tmpMemC); ## Barrier: 0x8
  luv $v25, 0, 0, $s0                                ## L:273  |    137 | bloom0 = load_vec_u8(dmemBloom, 0);
  luv $v23, 0, 8, $s0                                ## L:274  |    138 | bloom2 = load_vec_u8(dmemBloom, 8);
  luv $v20, 0, 328, $s0                              ## L:275  |    139 | bloom2Next = load_vec_u8(dmemBloom, 328);
  addiu $s0, $s0, 8                                  ## L:277  |    140 | dmemBloom += 8;
  vor $v15, $v00, $v14                               ## L:279  |      ^ | diffY_A = diffY_B;
  vsubc $v14, $v20, $v23.v                           ## L:280  |  **143 | diffY_B = bloom2Next - bloom2;
  addiu $fp, $fp, 32                                 ## L:282  |      ^ | dmemIn += 32;
  addiu $sp, $sp, 16                                 ## L:283  |    144 | dmemOut += 16;
  bne $fp, $k1, LABEL_Cmd_HDRBloom_000A              ## L:283  |    145 | dmemOut += 16;
  addu $s3, $s3, $t1                                 ## L:239  |   *147 | sumRGB += tmp0 + tmp1 + tmp2 + tmp3 + tmp4 + tmp5;
  LABEL_Cmd_HDRBloom_000B:
  vmudn $v19, $v18, $v30.e6                          ## L:287  |    148 | pixelOut = pixelR:uint * 2;
  vmadl $v19, $v17, $v31.e3                          ## L:288  |    149 | asm_op("vmadl", pixelOut, pixelG, VSHIFT8.w);
  vmadl $v19, $v16, $v30.e0                          ## L:289  |    150 | asm_op("vmadl", pixelOut, pixelB, VSHIFT.x);
  slv $v00, 0, -8, $sp                               ## L:292  |      ^ | store(VZERO.xy, dmemOut, -8);
  addiu $t7, $zero, 639                              ## L:294  |    151 | u32 dmaSize = (320 * 2) - 1;
  addiu $s1, $sp, 64896                              ## L:295  |    152 | ptrDMEM = dmemOut - 320 * 2;
  mtc0 $s1, COP0_DMA_SPADDR                          ## L:40   |    153 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $a1, COP0_DMA_RAMADDR                         ## L:41   |    154 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  mtc0 $t7, COP0_DMA_WRITE                           ## L:42   |  **157 | @Barrier("DMA") set_dma_write(size); ## Barrier: 0x1
  addiu $a1, $a1, 640                                ## L:298  |    158 | ptrOut += (320 * 2);
  bne $a0, $s7, LABEL_Cmd_HDRBloom_0003              ## L:298  |    159 | ptrOut += (320 * 2);
  sdv $v19, 0, -16, $sp                              ## L:291  |   *161 | store(pixelOut.xyzw, dmemOut, -16);
  LABEL_Cmd_HDRBloom_0004:
  addiu $s2, $zero, 7                                ## L:303  |    162 | u16 dmaSize = 8 - 1;
  mtc0 $s6, COP0_DMA_SPADDR                          ## L:40   |    163 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $a0, COP0_DMA_RAMADDR                         ## L:41   |    164 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  mtc0 $s2, COP0_DMA_WRITE                           ## L:42   |  **167 | @Barrier("DMA") set_dma_write(size); ## Barrier: 0x1
  j RSPQ_Loop                                        ## L:43   |    168 | }
  sw $s3, 0($s6)                                     ## L:302  |  **171 | store(sumRGB, buffDMEM, 0);
Cmd_Blur:
  addiu $s7, $a0, 18880                              ## L:370  |      ^ | u32 ptrRDRAMInEnd = ptrRDRAMIn + ((80 * 4) * (60 - 1));
  addiu $s6, $zero, 60                               ## L:371  |      2 | u32 rowsLeft = 60;
  addiu $s5, $zero, 319                              ## L:373  |      3 | u16 dmaSize = (80 * 4) - 1;
  or $s4, $zero, $zero                               ## L:374  |      4 | u16 dmemInOffset = 0;
  addiu $s3, $zero, 1056                             ## L:375  |      5 | u16 dmemInOffsetEnd = (80 * 4 + 32) * 3;
  mtc2 $a2, $v29.e0                                  ## L:377  |      6 | VTEMP.x = thresholdBrightness;
  srl $a2, $a2, 16                                   ## L:378  |      7 | thresholdBrightness >>= 16;
  mtc2 $a2, $v29.e1                                  ## L:379  |     *9 | VTEMP.y = thresholdBrightness;
  LABEL_Cmd_Blur_000C:
  mfc0 $ra, COP0_DMA_BUSY                            ## L:386  |     10 | RA = get_dma_busy();
  ori $s2, $zero, %lo(BUFF_IN_A)                     ## L:387  |     11 | dmemInA = BUFF_IN_A;
  or $s0, $zero, $s2                                 ## L:389  |     12 | lastLoadedDmemIn = dmemInA;
  bne $ra, $zero, LABEL_Cmd_Blur_000C                ## L:389  |     13 | lastLoadedDmemIn = dmemInA;
  addiu $s1, $s2, 1072                               ## L:388  |    *15 | dmemOutStart = dmemInA + ((80 * 4 + 32)*3) + 16;
  LABEL_Cmd_Blur_000D:
  mtc0 $s2, COP0_DMA_SPADDR                          ## L:32   |     16 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $a0, COP0_DMA_RAMADDR                         ## L:33   |     17 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  mtc0 $s5, COP0_DMA_READ                            ## L:34   |   **20 | @Barrier("DMA") set_dma_read(size); ## Barrier: 0x1
  addiu $s2, $s2, 352                                ## L:395  |     21 | dmemInA += (80 * 4 + 32);
  LABEL_Cmd_Blur_000E:
  mfc0 $ra, COP0_DMA_BUSY                            ## L:48   |    *23 | RA = get_dma_busy();
  bne $ra, $zero, LABEL_Cmd_Blur_000E                ## L:48   |   **26 | RA = get_dma_busy();
  nop                                                ## L:48   |    *28 | RA = get_dma_busy();
  LABEL_Cmd_Blur_000F:
  mtc0 $s2, COP0_DMA_SPADDR                          ## L:32   |     29 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $a0, COP0_DMA_RAMADDR                         ## L:33   |     30 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  mtc0 $s5, COP0_DMA_READ                            ## L:34   |   **33 | @Barrier("DMA") set_dma_read(size); ## Barrier: 0x1
  addiu $s2, $s2, 352                                ## L:399  |     34 | dmemInA += (80 * 4 + 32);
  addiu $a0, $a0, 320                                ## L:400  |     35 | ptrRDRAMIn += (80 * 4);
  LABEL_Cmd_Blur_0010:
  mfc0 $ra, COP0_DMA_BUSY                            ## L:48   |     36 | RA = get_dma_busy();
  bne $ra, $zero, LABEL_Cmd_Blur_0010                ## L:48   |   **39 | RA = get_dma_busy();
  nop                                                ## L:48   |    *41 | RA = get_dma_busy();
  LABEL_Cmd_Blur_0011:
  mtc0 $s2, COP0_DMA_SPADDR                          ## L:32   |     42 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $a0, COP0_DMA_RAMADDR                         ## L:33   |     43 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  mtc0 $s5, COP0_DMA_READ                            ## L:34   |   **46 | @Barrier("DMA") set_dma_read(size); ## Barrier: 0x1
  addiu $a0, $a0, 320                                ## L:404  |     47 | ptrRDRAMIn += (80 * 4);
  LABEL_Cmd_Blur_0012:
  mfc0 $ra, COP0_DMA_BUSY                            ## L:48   |    *49 | RA = get_dma_busy();
  bne $ra, $zero, LABEL_Cmd_Blur_0012                ## L:48   |   **52 | RA = get_dma_busy();
  nop                                                ## L:48   |    *54 | RA = get_dma_busy();
  LABEL_Cmd_Blur_0013:
  lw $fp, 316($s2)                                   ## L:408  |     55 | u32 tmp1 = load(dmemInA, 316);
  sw $fp, 320($s2)                                   ## L:409  |   **58 | store(tmp1, dmemInA, 320);
  addiu $s2, $s2, 65184                              ## L:411  |     59 | dmemInA -= (80 * 4 + 32);
  lw $sp, 316($s2)                                   ## L:412  |     60 | u32 tmp2 = load(dmemInA, 316);
  sw $sp, 320($s2)                                   ## L:413  |   **63 | store(tmp2, dmemInA, 320);
  addiu $fp, $zero, 960                              ## L:420  |     64 | u16 thresholdStride = (80 * 4) * 3;
  vxor $v28, $v00, $v00.e0                           ## L:422  |      ^ | vec16 maskR = 0; maskR.x = 1;
  vmov $v28.e0, $v30.e7                              ## L:422  |     65 | vec16 maskR = 0; maskR.x = 1;
  vxor $v27, $v00, $v00.e0                           ## L:423  |     66 | vec16 maskG = 0; maskG.y = 1;
  vmov $v27.e1, $v30.e7                              ## L:423  |     67 | vec16 maskG = 0; maskG.y = 1;
  vxor $v26, $v00, $v00.e0                           ## L:424  |     68 | vec16 maskB = 0; maskB.z = 1;
  vmov $v26.e2, $v30.e7                              ## L:424  |     69 | vec16 maskB = 0; maskB.z = 1;
  vxor $v25, $v00, $v00.e0                           ## L:426  |     70 | vec16 maskR1 = 0; maskR1.X = 1;
  vmov $v25.e4, $v30.e7                              ## L:426  |     71 | vec16 maskR1 = 0; maskR1.X = 1;
  vxor $v24, $v00, $v00.e0                           ## L:427  |     72 | vec16 maskG1 = 0; maskG1.Y = 1;
  vmov $v24.e5, $v30.e7                              ## L:427  |     73 | vec16 maskG1 = 0; maskG1.Y = 1;
  vxor $v23, $v00, $v00.e0                           ## L:428  |     74 | vec16 maskB1 = 0; maskB1.Z = 1;
  vmov $v23.e6, $v30.e7                              ## L:428  |     75 | vec16 maskB1 = 0; maskB1.Z = 1;
  ori $sp, $zero, %lo(RSPQ_SCRATCH_MEM)              ## L:430  |      ^ | u16 temp = RSPQ_SCRATCH_MEM;
  LABEL_Cmd_Blur_0016:
  mfc0 $ra, COP0_DMA_BUSY                            ## L:437  |     76 | RA = get_dma_busy();
  ori $s2, $zero, %lo(BUFF_IN_A)                     ## L:438  |     77 | dmemInA = BUFF_IN_A;
  or $k1, $zero, $s1                                 ## L:441  |     78 | dmemOutCurr = dmemOutStart;
  addiu $t9, $zero, 352                              ## L:442  |     79 | dmemInBOffset = (80 * 4 + 32);
  bne $ra, $zero, LABEL_Cmd_Blur_0016                ## L:442  |     80 | dmemInBOffset = (80 * 4 + 32);
  addiu $s2, $s2, 4                                  ## L:439  |    *82 | dmemInA += 4;
  LABEL_Cmd_Blur_0017:
  vxor $v22, $v00, $v00.e0                           ## L:448  |     83 | sumA = 0;
  beq $a2, $zero, LABEL_Cmd_Blur_0018                ## L:450  |      ^ | if(thresholdBrightness)
  addiu $t8, $s2, 704                                ## L:445  |    *85 | u16 dmemInC = dmemInA + (80 * 4 + 32)*2;
  or $t7, $zero, $s0                                 ## L:453  |     86 | u16 ptrClamp = lastLoadedDmemIn;
  LABEL_Cmd_Blur_0019:
  addiu $fp, $zero, 320                              ## L:457  |     87 | thresholdStride = (80 * 4);
  addu $t6, $s0, $fp                                 ## L:458  |     88 | ptrClampEnd = lastLoadedDmemIn + thresholdStride;
  luv $v18, 0, 0, $t7                                ## L:460  |     89 | vec16 p0 = load_vec_u8(ptrClamp, 0);
  luv $v17, 0, 8, $t7                                ## L:461  |     90 | vec16 p1 = load_vec_u8(ptrClamp, 8);
  luv $v16, 0, 16, $t7                               ## L:462  |     91 | vec16 p2 = load_vec_u8(ptrClamp, 16);
  luv $v15, 0, 24, $t7                               ## L:463  |     92 | vec16 p3 = load_vec_u8(ptrClamp, 24);
  luv $v14, 0, 32, $t7                               ## L:464  |     93 | vec16 p4 = load_vec_u8(ptrClamp, 32);
  luv $v13, 0, 40, $t7                               ## L:465  |     94 | vec16 p5 = load_vec_u8(ptrClamp, 40);
  luv $v12, 0, 48, $t7                               ## L:466  |     95 | vec16 p6 = load_vec_u8(ptrClamp, 48);
  luv $v11, 0, 56, $t7                               ## L:467  |     96 | vec16 p7 = load_vec_u8(ptrClamp, 56);
  luv $v10, 0, 64, $t7                               ## L:468  |     97 | vec16 p8 = load_vec_u8(ptrClamp, 64);
  luv $v09, 0, 72, $t7                               ## L:469  |     98 | vec16 p9 = load_vec_u8(ptrClamp, 72);
  vlt $v19, $v18, $v29.e1                            ## L:358  |      ^ | tmp = pixel < VTEMP.y;
  vmrg $v18, $v00, $v18                              ## L:359  |     99 | pixel = select(VZERO, pixel);
  vlt $v19, $v17, $v29.e1                            ## L:358  |    100 | tmp = pixel < VTEMP.y;
  vmrg $v17, $v00, $v17                              ## L:359  |    101 | pixel = select(VZERO, pixel);
  vlt $v19, $v16, $v29.e1                            ## L:358  |    102 | tmp = pixel < VTEMP.y;
  vmrg $v16, $v00, $v16                              ## L:359  |    103 | pixel = select(VZERO, pixel);
  vlt $v19, $v15, $v29.e1                            ## L:358  |    104 | tmp = pixel < VTEMP.y;
  vmrg $v15, $v00, $v15                              ## L:359  |    105 | pixel = select(VZERO, pixel);
  vlt $v19, $v14, $v29.e1                            ## L:358  |    106 | tmp = pixel < VTEMP.y;
  vmrg $v14, $v00, $v14                              ## L:359  |    107 | pixel = select(VZERO, pixel);
  vlt $v19, $v13, $v29.e1                            ## L:358  |    108 | tmp = pixel < VTEMP.y;
  vmrg $v13, $v00, $v13                              ## L:359  |    109 | pixel = select(VZERO, pixel);
  vlt $v19, $v12, $v29.e1                            ## L:358  |    110 | tmp = pixel < VTEMP.y;
  vmrg $v12, $v00, $v12                              ## L:359  |    111 | pixel = select(VZERO, pixel);
  vlt $v19, $v11, $v29.e1                            ## L:358  |    112 | tmp = pixel < VTEMP.y;
  vmrg $v11, $v00, $v11                              ## L:359  |    113 | pixel = select(VZERO, pixel);
  vlt $v19, $v10, $v29.e1                            ## L:358  |    114 | tmp = pixel < VTEMP.y;
  vmrg $v10, $v00, $v10                              ## L:359  |    115 | pixel = select(VZERO, pixel);
  vlt $v19, $v09, $v29.e1                            ## L:358  |    116 | tmp = pixel < VTEMP.y;
  vmrg $v09, $v00, $v09                              ## L:359  |    117 | pixel = select(VZERO, pixel);
  suv $v18, 0, 0, $t7                                ## L:482  |      ^ | store_vec_u8(p0, ptrClamp, 0);
  suv $v17, 0, 8, $t7                                ## L:483  |    118 | store_vec_u8(p1, ptrClamp, 8);
  suv $v16, 0, 16, $t7                               ## L:484  |    119 | store_vec_u8(p2, ptrClamp, 16);
  suv $v15, 0, 24, $t7                               ## L:485  |    120 | store_vec_u8(p3, ptrClamp, 24);
  suv $v14, 0, 32, $t7                               ## L:486  |    121 | store_vec_u8(p4, ptrClamp, 32);
  suv $v13, 0, 40, $t7                               ## L:487  |    122 | store_vec_u8(p5, ptrClamp, 40);
  suv $v12, 0, 48, $t7                               ## L:488  |    123 | store_vec_u8(p6, ptrClamp, 48);
  suv $v11, 0, 56, $t7                               ## L:489  |    124 | store_vec_u8(p7, ptrClamp, 56);
  suv $v10, 0, 64, $t7                               ## L:490  |    125 | store_vec_u8(p8, ptrClamp, 64);
  suv $v09, 0, 72, $t7                               ## L:491  |    126 | store_vec_u8(p9, ptrClamp, 72);
  addiu $t7, $t7, 80                                 ## L:493  |    127 | ptrClamp += 80;
  bne $t7, $t6, LABEL_Cmd_Blur_0019                  ## L:493  |    128 | ptrClamp += 80;
  nop                                                ## L:493  |   *130 | ptrClamp += 80;
  LABEL_Cmd_Blur_0018:
  lw $t7, 316($s0)                                   ## L:499  |    131 | u32 tmp1 = load(lastLoadedDmemIn, 316);
  sw $t7, 320($s0)                                   ## L:500  |  **134 | store(tmp1, lastLoadedDmemIn, 320);
  addiu $a1, $a1, 320                                ## L:505  |    135 | ptrRDRAMOut += (80 * 4);
  addiu $k0, $s1, 320                                ## L:506  |    136 | dmemOutEnd = dmemOutStart + (80 * 4);
  luv $v15, 0, 0, $s2                                ## L:510  |    137 | vec16 pixel_y0_x0  = load_vec_u8(dmemInA, 0);
  luv $v14, 0, 8, $s2                                ## L:511  |    138 | vec16 pixel_y0_x2  = load_vec_u8(dmemInA, 8);
  luv $v13, 0, 352, $s2                              ## L:513  |    139 | vec16 pixel_y1_x0 = load_vec_u8(dmemInA, 352);
  luv $v12, 0, 360, $s2                              ## L:514  |    140 | vec16 pixel_y1_x2 = load_vec_u8(dmemInA, 360);
  luv $v11, 0, 0, $t8                                ## L:516  |    141 | vec16 pixel_y2_x0 = load_vec_u8(dmemInC, 0);
  luv $v10, 0, 8, $t8                                ## L:517  |    142 | vec16 pixel_y2_x2 = load_vec_u8(dmemInC, 8);
  LABEL_Cmd_Blur_001B:
  suv $v17, 0, -16, $k1                              ## L:520  |  **145 | store_vec_u8(res0, dmemOutCurr, -16);
  suv $v16, 0, -8, $k1                               ## L:521  |    146 | store_vec_u8(res1, dmemOutCurr, -8);
  vmulu $v20, $v14, $v29.e0                          ## L:527  |      ^ | sumC:ufract = pixel_y0_x2:ufract  * VTEMP:ufract.x;
  vmacu $v20, $v12, $v29.e0                          ## L:528  |    147 | sumC:ufract = pixel_y1_x2:ufract +* VTEMP:ufract.x;
  vmacu $v20, $v10, $v29.e0                          ## L:529  |    148 | sumC:ufract = pixel_y2_x2:ufract +* VTEMP:ufract.x;
  sdv $v20, 0, 8, $sp                                ## L:531  | ***152 | @Barrier("group2") store(sumC.xyzw, temp, 8); ## Barrier: 0x10
  vmulu $v21, $v15, $v29.e0                          ## L:534  |      ^ | sumB:ufract = pixel_y0_x0:ufract  * VTEMP:ufract.x;
  vmacu $v21, $v13, $v29.e0                          ## L:535  |    153 | sumB:ufract = pixel_y1_x0:ufract +* VTEMP:ufract.x;
  vmacu $v21, $v11, $v29.e0                          ## L:536  |    154 | sumB:ufract = pixel_y2_x0:ufract +* VTEMP:ufract.x;
  sdv $v21, 8, 0, $sp                                ## L:538  | ***158 | @Barrier("group2") store(sumB.XYZW, temp, 0); ## Barrier: 0x10
  vmadh $v17, $v28, $v22.e4                          ## L:543  |      ^ | res0:sint = maskR:sint +* sumA:sint.X;
  vmadh $v17, $v27, $v22.e5                          ## L:544  |    159 | res0:sint = maskG:sint +* sumA:sint.Y;
  vmadh $v17, $v26, $v22.e6                          ## L:545  |    160 | res0:sint = maskB:sint +* sumA:sint.Z;
  vmadh $v17, $v25, $v21.e0                          ## L:547  |    161 | res0:sint = maskR1:sint +* sumB:sint.x;
  vmadh $v17, $v24, $v21.e1                          ## L:548  |    162 | res0:sint = maskG1:sint +* sumB:sint.y;
  vmadh $v17, $v23, $v21.e2                          ## L:549  |    163 | res0:sint = maskB1:sint +* sumB:sint.z;
  vmadh $v17, $v22, $v30.e7                          ## L:551  |    164 | res0:sint = sumA:sint +* 1;
  lqv $v09, 0, 0, $sp                                ## L:554  |      ^ | @Barrier("group2") group2 = load(temp, 0); ## Barrier: 0x10
  vmudh $v22, $v20, $v30.e7                          ## L:556  |    165 | sumA:sint = sumC:sint * 1;
  vmadh $v16, $v21, $v30.e7                          ## L:557  |    166 | res1:sint = sumB:sint +* 1;
  addiu $s2, $s2, 16                                 ## L:560  |      ^ | dmemInA += 16;
  addiu $t8, $t8, 16                                 ## L:561  |    167 | dmemInC += 16;
  luv $v15, 0, 0, $s2                                ## L:564  |    168 | pixel_y0_x0 = load_vec_u8(dmemInA, 0);
  luv $v14, 0, 8, $s2                                ## L:565  |    169 | pixel_y0_x2 = load_vec_u8(dmemInA, 8);
  luv $v13, 0, 352, $s2                              ## L:567  |    170 | pixel_y1_x0 = load_vec_u8(dmemInA, 352);
  luv $v12, 0, 360, $s2                              ## L:568  |    171 | pixel_y1_x2 = load_vec_u8(dmemInA, 360);
  luv $v11, 0, 0, $t8                                ## L:570  |    172 | pixel_y2_x0 = load_vec_u8(dmemInC, 0);
  luv $v10, 0, 8, $t8                                ## L:571  |    173 | pixel_y2_x2 = load_vec_u8(dmemInC, 8);
  addiu $k1, $k1, 16                                 ## L:573  |    174 | dmemOutCurr += 16;
  bne $k1, $k0, LABEL_Cmd_Blur_001B                  ## L:573  |    175 | dmemOutCurr += 16;
  vmadh $v16, $v09, $v30.e7                          ## L:558  |   *177 | res1:sint = group2:sint +* 1;
  LABEL_Cmd_Blur_001C:
  suv $v17, 0, -16, $k1                              ## L:576  |    178 | store_vec_u8(res0, dmemOutCurr, -16);
  suv $v16, 0, -8, $k1                               ## L:577  |  **181 | store_vec_u8(res1, dmemOutCurr, -8);
  mtc0 $s1, COP0_DMA_SPADDR                          ## L:40   |    182 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $a1, COP0_DMA_RAMADDR                         ## L:41   |    183 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  mtc0 $s5, COP0_DMA_WRITE                           ## L:42   |  **186 | @Barrier("DMA") set_dma_write(size); ## Barrier: 0x1
  addiu $s6, $s6, 65535                              ## L:581  |    187 | rowsLeft -= 1;
  LABEL_Cmd_Blur_001D:
  mfc0 $ra, COP0_DMA_BUSY                            ## L:584  |   *189 | RA = get_dma_busy();
  bne $ra, $zero, LABEL_Cmd_Blur_001D                ## L:585  |  **192 | lastLoadedDmemIn = dmemInOffset + BUFF_IN_A;
  addiu $s0, $s4, %lo(BUFF_IN_A)                     ## L:585  |   *194 | lastLoadedDmemIn = dmemInOffset + BUFF_IN_A;
  LABEL_Cmd_Blur_001E:
  beq $s6, $zero, RSPQ_Loop                          ## L:588  |    195 | if(rowsLeft == 0)exit;
  nop                                                ## L:588  |   *197 | if(rowsLeft == 0)exit;
  mtc0 $s0, COP0_DMA_SPADDR                          ## L:32   |    198 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $a0, COP0_DMA_RAMADDR                         ## L:33   |    199 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  addiu $s4, $s4, 352                                ## L:592  |    200 | dmemInOffset += (80 * 4 + 32);
  beq $a0, $s7, LABEL_Cmd_Blur_0020                  ## L:594  |    201 | if(ptrRDRAMIn != ptrRDRAMInEnd) {
  mtc0 $s5, COP0_DMA_READ                            ## L:34   |   *203 | @Barrier("DMA") set_dma_read(size); ## Barrier: 0x1
  addiu $a0, $a0, 320                                ## L:595  |    204 | ptrRDRAMIn += (80 * 4);
  LABEL_Cmd_Blur_0020:
  bne $s4, $s3, LABEL_Cmd_Blur_0016                  ## L:598  |    205 | if(dmemInOffset == dmemInOffsetEnd) {
  nop                                                ## L:598  |   *207 | if(dmemInOffset == dmemInOffsetEnd) {
  or $s4, $zero, $zero                               ## L:599  |    208 | dmemInOffset = 0;
  LABEL_Cmd_Blur_0021:
  j LABEL_Cmd_Blur_0016                              ## L:599  |    209 | dmemInOffset = 0;
  nop                                                ## L:599  |   *211 | dmemInOffset = 0;
Cmd_Downscale:
  lui $at, 0x04                                      ## L:623  |      ^ | u32 ptrRDRAMInEnd = ptrRDRAMIn + (320 * 240 * 4);
  ori $at, $at, 0xB000                               ## L:623  |      2 | u32 ptrRDRAMInEnd = ptrRDRAMIn + (320 * 240 * 4);
  addu $s7, $a0, $at                                 ## L:623  |      3 | u32 ptrRDRAMInEnd = ptrRDRAMIn + (320 * 240 * 4);
  addiu $a1, $a1, 65216                              ## L:625  |      4 | prtRDRAMOut -= (80 * 4);
  lui $s6, 0x5000                                    ## L:627  |      5 | u16 dmaSizeIn = (
  ori $s6, $s6, 0x14FF                               ## L:627  |      6 | u16 dmaSizeIn = (
  addiu $s5, $zero, 319                              ## L:632  |      7 | u16 dmaSizeOut = (80 * 4) - 1;
  addiu $at, $zero, 4096                             ## L:635  |      8 | sumFactor:sfract.x = 0.125;
  mtc2 $at, $v28.e0                                  ## L:635  |      9 | sumFactor:sfract.x = 0.125;
  ori $s4, $zero, %lo(BUFF_BLOOM)                    ## L:637  |     10 | u16 dmemOutA = BUFF_BLOOM;
  ori $s3, $zero, %lo(BUFF_BLOOM_B)                  ## L:638  |     11 | u16 dmemOutB = BUFF_BLOOM_B;
  LABEL_Cmd_Downscale_0024:
  mfc0 $ra, COP0_DMA_BUSY                            ## L:646  |     12 | RA = get_dma_busy();
  ori $s0, $zero, %lo(BUFF_IN_B)                     ## L:648  |     13 | dmemInB = BUFF_IN_B;
  bne $ra, $zero, LABEL_Cmd_Downscale_0024           ## L:648  |    *15 | dmemInB = BUFF_IN_B;
  ori $s1, $zero, %lo(BUFF_IN_A)                     ## L:647  |    *17 | dmemInA = BUFF_IN_A;
  LABEL_Cmd_Downscale_0025:
  mtc0 $s1, COP0_DMA_SPADDR                          ## L:32   |     18 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $a0, COP0_DMA_RAMADDR                         ## L:33   |     19 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  mtc0 $s6, COP0_DMA_READ                            ## L:34   |   **22 | @Barrier("DMA") set_dma_read(size); ## Barrier: 0x1
  addiu $a0, $a0, 5120                               ## L:654  |     23 | ptrRDRAMIn += (320 * 4) * 4;
  LABEL_Cmd_Downscale_0026:
  mfc0 $ra, COP0_DMA_BUSY                            ## L:657  |    *25 | RA = get_dma_busy();
  or $s2, $zero, $s4                                 ## L:659  |     26 | dmemOut = dmemOutA;
  bne $ra, $zero, LABEL_Cmd_Downscale_0026           ## L:660  |    *28 | dmemOutEnd = dmemOut + (80 * 4);
  addiu $fp, $s2, 320                                ## L:660  |    *30 | dmemOutEnd = dmemOut + (80 * 4);
  LABEL_Cmd_Downscale_0027:
  mtc0 $s3, COP0_DMA_SPADDR                          ## L:40   |     31 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $a1, COP0_DMA_RAMADDR                         ## L:41   |     32 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  mtc0 $s5, COP0_DMA_WRITE                           ## L:42   |   **35 | @Barrier("DMA") set_dma_write(size); ## Barrier: 0x1
  xor $s4, $s4, $s3                                  ## L:664  |     36 | swap(dmemOutA, dmemOutB);
  xor $s3, $s4, $s3                                  ## L:664  |     37 | swap(dmemOutA, dmemOutB);
  xor $s4, $s4, $s3                                  ## L:664  |     38 | swap(dmemOutA, dmemOutB);
  addiu $s1, $s1, 1280                               ## L:668  |     39 | dmemInA += 1280;
  addiu $s0, $s0, 1280                               ## L:669  |     40 | dmemInB += 1280;
  lqv $v27, 0, -32, $s1                              ## L:671  |     41 | vec16 a = load(dmemInA, -32);
  sqv $v27, 0, -16, $s1                              ## L:672  |  ***45 | store(a, dmemInA, -16);
  addiu $s1, $s1, 64256                              ## L:674  |     46 | dmemInA -= 1280;
  addiu $s0, $s0, 64256                              ## L:675  |     47 | dmemInB -= 1280;
  LABEL_Cmd_Downscale_0028:
  vadd $v27, $v25, $v21.v                            ## L:685  |      ^ | resA:sint = p0_AB:sint + p1_AB:sint;
  vadd $v26, $v17, $v13.v                            ## L:686  |     48 | resB:sint = p2_AB:sint + p3_AB:sint;
  vlt $v29, $v27, $v28.e1                            ## L:689  |   **51 | resA = resA < sumFactor.y ? VZERO : resA;
  vmrg $v27, $v00, $v27                              ## L:689  |     52 | resA = resA < sumFactor.y ? VZERO : resA;
  vlt $v29, $v26, $v28.e1                            ## L:690  |     53 | resB = resB < sumFactor.y ? VZERO : resB;
  vmrg $v26, $v00, $v26                              ## L:690  |   **56 | resB = resB < sumFactor.y ? VZERO : resB;
  suv $v27, 0, -16, $s2                              ## L:692  |      ^ | store_vec_u8(resA, dmemOut, -16);
  suv $v26, 0, -8, $s2                               ## L:693  |  ***60 | store_vec_u8(resB, dmemOut, -8);
  luv $v25, 0, 0, $s1                                ## L:700  |     61 | p0_AB = load_vec_u8(dmemInA, 0);
  luv $v24, 0, 8, $s1                                ## L:701  |     62 | p0_CD = load_vec_u8(dmemInA, 8);
  luv $v23, 0, 0, $s0                                ## L:702  |     63 | p0_EF = load_vec_u8(dmemInB, 0);
  luv $v22, 0, 8, $s0                                ## L:703  |     64 | p0_GH = load_vec_u8(dmemInB, 8);
  luv $v21, 0, 16, $s1                               ## L:705  |     65 | p1_AB = load_vec_u8(dmemInA, 16);
  luv $v20, 0, 24, $s1                               ## L:706  |     66 | p1_CD = load_vec_u8(dmemInA, 24);
  luv $v19, 0, 16, $s0                               ## L:707  |     67 | p1_EF = load_vec_u8(dmemInB, 16);
  luv $v18, 0, 24, $s0                               ## L:708  |     68 | p1_GH = load_vec_u8(dmemInB, 24);
  luv $v17, 0, 32, $s1                               ## L:710  |     69 | p2_AB = load_vec_u8(dmemInA, 32);
  luv $v16, 0, 40, $s1                               ## L:711  |     70 | p2_CD = load_vec_u8(dmemInA, 40);
  luv $v15, 0, 32, $s0                               ## L:712  |     71 | p2_EF = load_vec_u8(dmemInB, 32);
  luv $v14, 0, 40, $s0                               ## L:713  |     72 | p2_GH = load_vec_u8(dmemInB, 40);
  luv $v13, 0, 48, $s1                               ## L:715  |     73 | p3_AB = load_vec_u8(dmemInA, 48);
  luv $v12, 0, 56, $s1                               ## L:716  |     74 | p3_CD = load_vec_u8(dmemInA, 56);
  luv $v11, 0, 48, $s0                               ## L:717  |     75 | p3_EF = load_vec_u8(dmemInB, 48);
  luv $v10, 0, 56, $s0                               ## L:718  |     76 | p3_GH = load_vec_u8(dmemInB, 56);
  vmulf $v25, $v25, $v28.e0                          ## L:608  |      ^ | a:sfract = a:sfract * sumFactor:sfract.x;
  vmacf $v25, $v24, $v28.e0                          ## L:609  |     77 | a:sfract = b:sfract +* sumFactor:sfract.x;
  vmacf $v25, $v23, $v28.e0                          ## L:610  |     78 | a:sfract = c:sfract +* sumFactor:sfract.x;
  vmacf $v25, $v22, $v28.e0                          ## L:611  |     79 | a:sfract = d:sfract +* sumFactor:sfract.x;
  vmulf $v21, $v21, $v28.e0                          ## L:608  |     80 | a:sfract = a:sfract * sumFactor:sfract.x;
  vmacf $v21, $v20, $v28.e0                          ## L:609  |     81 | a:sfract = b:sfract +* sumFactor:sfract.x;
  vmacf $v21, $v19, $v28.e0                          ## L:610  |     82 | a:sfract = c:sfract +* sumFactor:sfract.x;
  vmacf $v21, $v18, $v28.e0                          ## L:611  |     83 | a:sfract = d:sfract +* sumFactor:sfract.x;
  vmulf $v17, $v17, $v28.e0                          ## L:608  |     84 | a:sfract = a:sfract * sumFactor:sfract.x;
  vmacf $v17, $v16, $v28.e0                          ## L:609  |     85 | a:sfract = b:sfract +* sumFactor:sfract.x;
  vmacf $v17, $v15, $v28.e0                          ## L:610  |     86 | a:sfract = c:sfract +* sumFactor:sfract.x;
  vmacf $v17, $v14, $v28.e0                          ## L:611  |     87 | a:sfract = d:sfract +* sumFactor:sfract.x;
  vmulf $v13, $v13, $v28.e0                          ## L:608  |     88 | a:sfract = a:sfract * sumFactor:sfract.x;
  vmacf $v13, $v12, $v28.e0                          ## L:609  |     89 | a:sfract = b:sfract +* sumFactor:sfract.x;
  vmacf $v13, $v11, $v28.e0                          ## L:610  |     90 | a:sfract = c:sfract +* sumFactor:sfract.x;
  vmacf $v13, $v10, $v28.e0                          ## L:611  |     91 | a:sfract = d:sfract +* sumFactor:sfract.x;
  sdv $v25, 8, 0, $s1                                ## L:730  |      ^ | @Barrier("tmp0") store(p0_AB.XYZW, dmemInA, 0); ## Barrier: 0x20
  vmov $v25.e4, $v21.e0                              ## L:731  |     92 | @Barrier("tmp0") p0_AB.X = p1_AB.x; ## Barrier: 0x20
  vmov $v25.e5, $v21.e1                              ## L:732  |     93 | @Barrier("tmp0") p0_AB.Y = p1_AB.y; ## Barrier: 0x20
  vmov $v25.e6, $v21.e2                              ## L:733  |     94 | @Barrier("tmp0") p0_AB.Z = p1_AB.z; ## Barrier: 0x20
  ldv $v21, 0, 0, $s1                                ## L:734  |      ^ | @Barrier("tmp0") p1_AB.xyzw = load(dmemInA, 0).xyzw; ## Barrier: 0x20
  sdv $v17, 8, 8, $s1                                ## L:736  |     95 | @Barrier("tmp1") store(p2_AB.XYZW, dmemInA, 8); ## Barrier: 0x40
  vmov $v17.e4, $v13.e0                              ## L:737  |      ^ | @Barrier("tmp1") p2_AB.X = p3_AB.x; ## Barrier: 0x40
  vmov $v17.e5, $v13.e1                              ## L:738  |     96 | @Barrier("tmp1") p2_AB.Y = p3_AB.y; ## Barrier: 0x40
  vmov $v17.e6, $v13.e2                              ## L:739  |     97 | @Barrier("tmp1") p2_AB.Z = p3_AB.z; ## Barrier: 0x40
  ldv $v13, 0, 8, $s1                                ## L:740  |      ^ | @Barrier("tmp1") p3_AB.xyzw = load(dmemInA, 8).xyzw; ## Barrier: 0x40
  addiu $s2, $s2, 16                                 ## L:742  |     98 | dmemOut += 16;
  addiu $s0, $s0, 64                                 ## L:744  |     99 | dmemInB += 64;
  addiu $a1, $a1, 16                                 ## L:745  |    100 | prtRDRAMOut += 16;
  bne $s2, $fp, LABEL_Cmd_Downscale_0028             ## L:745  |    101 | prtRDRAMOut += 16;
  addiu $s1, $s1, 64                                 ## L:743  |   *103 | dmemInA += 64;
  LABEL_Cmd_Downscale_0029:
  vadd $v27, $v25, $v21.v                            ## L:749  |    104 | resA:sint = p0_AB:sint + p1_AB:sint;
  vadd $v26, $v17, $v13.v                            ## L:750  |    105 | resB:sint = p2_AB:sint + p3_AB:sint;
  suv $v26, 0, -8, $s2                               ## L:753  | ***109 | store_vec_u8(resB, dmemOut, -8);
  bne $a0, $s7, LABEL_Cmd_Downscale_0024             ## L:753  |    110 | store_vec_u8(resB, dmemOut, -8);
  suv $v27, 0, -16, $s2                              ## L:752  |   *112 | store_vec_u8(resA, dmemOut, -16);
  LABEL_Cmd_Downscale_0023:
  mtc0 $s3, COP0_DMA_SPADDR                          ## L:40   |    113 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x1
  mtc0 $a1, COP0_DMA_RAMADDR                         ## L:41   |    114 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x1
  j RSPQ_Loop                                        ## L:43   |    115 | }
  mtc0 $s5, COP0_DMA_WRITE                           ## L:42   |  **118 | @Barrier("DMA") set_dma_write(size); ## Barrier: 0x1

OVERLAY_CODE_END:

#define zero $0
#define v0 $2
#define v1 $3
#define a0 $4
#define a1 $5
#define a2 $6
#define a3 $7
#define t0 $8
#define t1 $9
#define t2 $10
#define t3 $11
#define t4 $12
#define t5 $13
#define t6 $14
#define t7 $15
#define s0 $16
#define s1 $17
#define s2 $18
#define s3 $19
#define s4 $20
#define s5 $21
#define s6 $22
#define s7 $23
#define t8 $24
#define t9 $25
#define k0 $26
#define k1 $27
#define gp $28
#define sp $29
#define fp $30
#define ra $31

.set at
.set macro