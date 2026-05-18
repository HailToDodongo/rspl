## Auto-generated file, transpiled with RSPL
#define RDP_CMD_PRIM 0x3A00'0000
#define RDP_CMD_DEPTH 0x2E
#define RDP_CMD_RECT 0x36
#define RDP_CMD_TEX_RECT 0x24
#define RDP_CMD_TEX_RECT_FLIP 0x25
#define PARTICLE_INPUT_SIZE_S8 16
#define PARTICLE_INPUT_SIZE_S16 24
#define PARTICLE_MAX_COUNT 344
#define RDP_FLUSH_SIZE 528
#define RDP_TEX_FLUSH_SIZE 480
#define RDP_POS_MASK 0b1111'1111'1111
#define LOOP_16BIT 1
#define MATRIX_TEMP_MUL PARTICLE_BUFF
#define MATRIX_MV RDP_BUFF
#define MATRIX_SIZE 0x40
#include <rsp_queue.inc>
#include <rsp_rdpq.inc>
#include <rdpq_macros.h>

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
    RSPQ_DefineCommand TPXCmd_SyncT3D, 12
    RSPQ_DefineCommand TPXCmd_DrawColor, 8
    RSPQ_DefineCommand T3DCmd_MatrixStack, 8
    RSPQ_DefineCommand TPXCmd_SetDMEM, 8
    RSPQ_DefineCommand TPXCmd_DrawTextured, 8
  RSPQ_EndOverlayHeader

  RSPQ_BeginSavedState
    STATE_MEM_START:
    .align 3
    MATRIX_PROJ: .ds.b 64
    .align 3
    MATRIX_MVP: .ds.b 64
    .align 4
    SCREEN_SCALE_OFFSET: .half 0, 0, 0, 0, 0, 0, 16383, 255
    .align 3
    NORM_SCALE_W: .half 65535, 65535, 65535, 0
    .align 2
    MATRIX_STACK_PTR: .word 0
    .align 1
    PARTICLE_SCALE: .half 32767, 32767
    .align 1
    TEX_OFFSET: .half 0
    .align 1
    TILE_COUNT: .half 0
    .align 1
    BASE_SIZE: .half 128
    STATE_MEM_END:
  RSPQ_EndSavedState

.bss
  TEMP_STATE_MEM_START:
    .align 4
    PARTICLE_BUFF: .ds.b 2752
    .align 4
    RDP_BUFF: .ds.b 528
  TEMP_STATE_MEM_END:

.text
OVERLAY_CODE_START:

TPXCmd_SyncT3D:
  ori $t0, $zero, 127                                ## L:816  |      ^ | dma_in_async(MATRIX_PROJ, rdramMatrix, 128);
  ori $s4, $zero, %lo(MATRIX_PROJ)                   ## L:816  |      2 | dma_in_async(MATRIX_PROJ, rdramMatrix, 128);
  or $t2, $zero, $zero                               ## L:816  |      3 | dma_in_async(MATRIX_PROJ, rdramMatrix, 128);
  jal DMAExec                                        ## L:816  |      4 | dma_in_async(MATRIX_PROJ, rdramMatrix, 128); ## Args: $t0, $t1, $s0, $s4, $t2
  or $s0, $zero, $a0                                 ## L:816  |     *6 | dma_in_async(MATRIX_PROJ, rdramMatrix, 128);
  addiu $a0, $a0, 128                                ## L:818  |      7 | rdramMatrix += 128;
  ori $t0, $zero, 63                                 ## L:819  |      8 | dma_in_async(RDP_BUFF, rdramMatrix, 64);
  ori $s4, $zero, %lo(RDP_BUFF)                      ## L:819  |      9 | dma_in_async(RDP_BUFF, rdramMatrix, 64);
  or $t2, $zero, $zero                               ## L:819  |     10 | dma_in_async(RDP_BUFF, rdramMatrix, 64);
  jal DMAExec                                        ## L:819  |     11 | dma_in_async(RDP_BUFF, rdramMatrix, 64); ## Args: $t0, $t1, $s0, $s4, $t2
  or $s0, $zero, $a0                                 ## L:819  |    *13 | dma_in_async(RDP_BUFF, rdramMatrix, 64);
  ori $t0, $zero, 63                                 ## L:821  |     14 | dma_out_async(RDP_BUFF, addrRDRAM, 64);
  ori $s4, $zero, %lo(RDP_BUFF)                      ## L:821  |     15 | dma_out_async(RDP_BUFF, addrRDRAM, 64);
  addiu $t2, $zero, -32768                           ## L:821  |     16 | dma_out_async(RDP_BUFF, addrRDRAM, 64);
  jal DMAExec                                        ## L:821  |     17 | dma_out_async(RDP_BUFF, addrRDRAM, 64); ## Args: $t0, $t1, $s0, $s4, $t2
  lw $s0, %lo(MATRIX_STACK_PTR + 0)                  ## L:820  |    *19 | u32<$s0> addrRDRAM = load(MATRIX_STACK_PTR);
  or $s0, $zero, $a1                                 ## L:827  |     20 | dma_in(SCREEN_SCALE_OFFSET, rdramScreen);
  ori $t0, $zero, 15                                 ## L:827  |     21 | dma_in(SCREEN_SCALE_OFFSET, rdramScreen);
  ori $s4, $zero, %lo(SCREEN_SCALE_OFFSET)           ## L:827  |     22 | dma_in(SCREEN_SCALE_OFFSET, rdramScreen);
  addiu $t2, $zero, 12                               ## L:827  |     23 | dma_in(SCREEN_SCALE_OFFSET, rdramScreen);
  jal DMAExec                                        ## L:827  |     24 | dma_in(SCREEN_SCALE_OFFSET, rdramScreen); ## Args: $t0, $t1, $s0, $s4, $t2
  sh $a2, %lo(6 + NORM_SCALE_W)($zero)               ## L:823  |    *26 | store(wNorm, NORM_SCALE_W, 6);
  j RSPQ_Loop                                        ## L:828  |     27 | }
  nop                                                ## L:828  |    *29 | }
TPXCmd_DrawColor:
  ori $s4, $zero, %lo(PARTICLE_BUFF)                 ## L:832  |      ^ | u16<$s4> dmaDmem = PARTICLE_BUFF;
  andi $t0, $a0, 65535                               ## L:833  |      2 | u32<$t0> dmaSize = dataSize & 0xFFFF;
  addu $s7, $s4, $t0                                 ## L:848  |      3 | u32 ptrInEnd = dmaDmem + dmaSize;
  addiu $t0, $t0, -1                                 ## L:849  |      4 | dma_in_async(dmaDmem, rdramAddr, dmaSize);
  or $t2, $zero, $zero                               ## L:849  |      5 | dma_in_async(dmaDmem, rdramAddr, dmaSize);
  jal DMAExec                                        ## L:849  |      6 | dma_in_async(dmaDmem, rdramAddr, dmaSize); ## Args: $t0, $t1, $s0, $s4, $t2
  or $s0, $zero, $a1                                 ## L:849  |     *8 | dma_in_async(dmaDmem, rdramAddr, dmaSize);
  or $s6, $zero, $s4                                 ## L:851  |      9 | u32 ptrIn = dmaDmem;
  ori $s4, $zero, %lo(RDP_BUFF)                      ## L:853  |     10 | dmaDmem = RDP_BUFF;
  ori $s3, $zero, %lo(RDP_BUFF)                      ## L:854  |     11 | u16<$s3> dmaDmemEnd = RDP_BUFF;
  addiu $s5, $s3, 528                                ## L:855  |     12 | u16 dmaDmemFlush = dmaDmemEnd + 528;
  ori $at, $zero, %lo(MATRIX_MVP)                    ## L:857  |     13 | vec32 mat0 = load(MATRIX_MVP, 0x00).xyzwxyzw;
  ldv $v27, 0, 0, $at                                ## L:857  |     14 | vec32 mat0 = load(MATRIX_MVP, 0x00).xyzwxyzw;
  ldv $v27, 8, 0, $at                                ## L:857  |     15 | vec32 mat0 = load(MATRIX_MVP, 0x00).xyzwxyzw;
  ldv $v28, 0, 8, $at                                ## L:857  |     16 | vec32 mat0 = load(MATRIX_MVP, 0x00).xyzwxyzw;
  ldv $v28, 8, 8, $at                                ## L:857  |     17 | vec32 mat0 = load(MATRIX_MVP, 0x00).xyzwxyzw;
  ldv $v25, 0, 16, $at                               ## L:858  |     18 | vec32 mat1 = load(MATRIX_MVP, 0x10).xyzwxyzw;
  ldv $v25, 8, 16, $at                               ## L:858  |     19 | vec32 mat1 = load(MATRIX_MVP, 0x10).xyzwxyzw;
  ldv $v26, 0, 24, $at                               ## L:858  |     20 | vec32 mat1 = load(MATRIX_MVP, 0x10).xyzwxyzw;
  ldv $v26, 8, 24, $at                               ## L:858  |     21 | vec32 mat1 = load(MATRIX_MVP, 0x10).xyzwxyzw;
  ldv $v23, 0, 32, $at                               ## L:859  |     22 | vec32 mat2 = load(MATRIX_MVP, 0x20).xyzwxyzw;
  ldv $v23, 8, 32, $at                               ## L:859  |     23 | vec32 mat2 = load(MATRIX_MVP, 0x20).xyzwxyzw;
  ldv $v24, 0, 40, $at                               ## L:859  |     24 | vec32 mat2 = load(MATRIX_MVP, 0x20).xyzwxyzw;
  ldv $v24, 8, 40, $at                               ## L:859  |     25 | vec32 mat2 = load(MATRIX_MVP, 0x20).xyzwxyzw;
  ldv $v21, 0, 48, $at                               ## L:860  |     26 | vec32 mat3 = load(MATRIX_MVP, 0x30).xyzwxyzw;
  ldv $v21, 8, 48, $at                               ## L:860  |     27 | vec32 mat3 = load(MATRIX_MVP, 0x30).xyzwxyzw;
  ldv $v22, 0, 56, $at                               ## L:860  |     28 | vec32 mat3 = load(MATRIX_MVP, 0x30).xyzwxyzw;
  ldv $v22, 8, 56, $at                               ## L:860  |     29 | vec32 mat3 = load(MATRIX_MVP, 0x30).xyzwxyzw;
  ori $at, $zero, %lo(SCREEN_SCALE_OFFSET)           ## L:862  |     30 | vec32 screenSize:sint = load(SCREEN_SCALE_OFFSET).xyzwxyzw;
  ldv $v19, 0, 0, $at                                ## L:862  |     31 | vec32 screenSize:sint = load(SCREEN_SCALE_OFFSET).xyzwxyzw;
  ldv $v19, 8, 0, $at                                ## L:862  |     32 | vec32 screenSize:sint = load(SCREEN_SCALE_OFFSET).xyzwxyzw;
  vmudl $v20, $v00, $v31.e7                          ## L:864  |      ^ | screenSize >>= 8;
  vmadm $v19, $v19, $v31.e7                          ## L:864  |  ***36 | screenSize >>= 8;
  vmadn $v20, $v00, $v00                             ## L:864  |     37 | screenSize >>= 8;
  ldv $v18, 0, 8, $at                                ## L:866  |      ^ | vec16 screenOffset = load(SCREEN_SCALE_OFFSET, 0x08).xyzwxyzw;
  ldv $v18, 8, 8, $at                                ## L:866  |     38 | vec16 screenOffset = load(SCREEN_SCALE_OFFSET, 0x08).xyzwxyzw;
  ori $at, $zero, %lo(NORM_SCALE_W)                  ## L:867  |     39 | vec16 normScaleW = load(NORM_SCALE_W).xyzwxyzw;
  ldv $v17, 0, 0, $at                                ## L:867  |     40 | vec16 normScaleW = load(NORM_SCALE_W).xyzwxyzw;
  ldv $v17, 8, 0, $at                                ## L:867  |     41 | vec16 normScaleW = load(NORM_SCALE_W).xyzwxyzw;
  addiu $at, $zero, 4095                             ## L:1211 |     42 | screenMax.w = 0b1111'1111'1111;
  mtc2 $at, $v15.e3                                  ## L:1211 |    *44 | screenMax.w = 0b1111'1111'1111;
  lw $s2, %lo(RDPQ_SCISSOR_RECT + 4)                 ## L:1212 |     45 | u32 extMax = load(RDPQ_SCISSOR_RECT, 4);
  lw $s1, %lo(RDPQ_SCISSOR_RECT + 0)                 ## L:1213 |     46 | u32 extMin = load(RDPQ_SCISSOR_RECT, 0);
  srl $t4, $s2, 12                                   ## L:1215 |    *48 | temp1 = extMax >> 12;
  mtc2 $t4, $v15.e0                                  ## L:1216 |     49 | screenMax.x = temp1;  screenMax.X = temp1;
  mtc2 $t4, $v15.e4                                  ## L:1216 |     50 | screenMax.x = temp1;  screenMax.X = temp1;
  mtc2 $s2, $v15.e1                                  ## L:1217 |   **53 | screenMax.y = extMax; screenMax.Y = extMax;
  mtc2 $s2, $v15.e5                                  ## L:1217 |     54 | screenMax.y = extMax; screenMax.Y = extMax;
  srl $t4, $s1, 12                                   ## L:1219 |     55 | temp1 = extMin >> 12;
  mtc2 $t4, $v16.e0                                  ## L:1220 |    *57 | screenMin.x = temp1;  screenMin.X = temp1;
  mtc2 $t4, $v16.e4                                  ## L:1220 |     58 | screenMin.x = temp1;  screenMin.X = temp1;
  mtc2 $s1, $v16.e1                                  ## L:1221 |   **61 | screenMin.y = extMin; screenMin.Y = extMin;
  mtc2 $s1, $v16.e5                                  ## L:1221 |     62 | screenMin.y = extMin; screenMin.Y = extMin;
  vand $v15, $v15, $v15.e3                           ## L:1223 |      ^ | screenMax &= screenMax.w;
  vand $v16, $v16, $v15.e3                           ## L:1224 |  ***66 | screenMin &= screenMax.w;
  ori $at, $zero, %lo(PARTICLE_SCALE)                ## L:874  |      ^ | globalPartSize.xy = load(PARTICLE_SCALE).xy;
  llv $v13, 0, 0, $at                                ## L:874  |     67 | globalPartSize.xy = load(PARTICLE_SCALE).xy;
  llv $v13, 8, 0, $at                                ## L:875  |     68 | globalPartSize.XY = load(PARTICLE_SCALE).xy;
  ori $s2, $zero, %lo(RDP_BUFF)                      ## L:880  |     69 | u16 buffRdp = RDP_BUFF;
  addiu $s1, $s5, 24                                 ## L:881  |     70 | u16 buffRdpEnd = dmaDmemFlush + 24;
  lui $t7, 0x3A00                                    ## L:882  |     71 | cmdRdpColor = 0x3A00'0000;
  addiu $a0, $zero, 46                               ## L:883  |     72 | cmdRdpDepth = 0x2E;
  LABEL_TPXCmd_DrawColor_0001:
  sw $t7, 0($s2)                                     ## L:885  |     73 | store(cmdRdpColor, buffRdp, 0);
  sb $a0, 8($s2)                                     ## L:886  |     74 | store(cmdRdpDepth, buffRdp, 8);
  sh $zero, 14($s2)                                  ## L:887  |     75 | store(ZERO:u16, buffRdp, 0x0E);
  addiu $s2, $s2, 24                                 ## L:888  |     76 | buffRdp += 24;
  bne $s2, $s1, LABEL_TPXCmd_DrawColor_0001          ## L:888  |     77 | buffRdp += 24;
  nop                                                ## L:888  |    *79 | buffRdp += 24;
  LABEL_TPXCmd_DrawColor_0002:
  jal DMAWaitIdle                                    ## L:892  |     80 | dma_await();
  nop                                                ## L:892  |    *82 | dma_await();
  bgez $a1, LABEL_TPXCmd_DrawColor_0003              ## L:899  |     83 | if(rdramAddr < 0) {
  vxor $v10, $v00, $v30.e7                           ## L:896  |    *85 | const vec16 vecOne = 1;
  lqv $v09, 0, 0, $s6                                ## L:465  |     86 | vec16 posStart = load(ptrIn, 0x00);
  vmulf $v14, $v13, $v09.h3                          ## L:466  |  ***90 | localPartSize = globalPartSize:sfract * posStart:sfract.wwwwWWWW;
  vmudn $v12, $v28, $v09.h0                          ## L:92   |     91 | out = mat0  * vec.xxxxXXXX;
  vmadh $v11, $v27, $v09.h0                          ## L:92   |     92 | out = mat0  * vec.xxxxXXXX;
  vmadn $v12, $v26, $v09.h1                          ## L:93   |     93 | out = mat1 +* vec.yyyyYYYY;
  vmadh $v11, $v25, $v09.h1                          ## L:93   |     94 | out = mat1 +* vec.yyyyYYYY;
  vmadn $v12, $v24, $v09.h2                          ## L:94   |     95 | out = mat2 +* vec.zzzzZZZZ;
  vmadh $v11, $v23, $v09.h2                          ## L:94   |     96 | out = mat2 +* vec.zzzzZZZZ;
  vmadn $v12, $v22, $v30.e7                          ## L:95   |     97 | out = mat3 +* 1;
  vmadh $v11, $v21, $v30.e7                          ## L:95   |     98 | out = mat3 +* 1;
  vch $v29, $v11, $v11.h3                            ## L:474  | ***102 | temp1 = clip(posClip, posClip.wwwwWWWW);
  vcl $v29, $v12, $v12.h3                            ## L:474  |    103 | temp1 = clip(posClip, posClip.wwwwWWWW);
  cfc2 $t4, $vcc                                     ## L:474  |    104 | temp1 = clip(posClip, posClip.wwwwWWWW);
  LABEL_TPXCmd_DrawColor_0005:
  andi $t1, $t4, 1028                                ## L:481  |  **107 | clipA = temp1 & 0b0000'0100'0000'0100;
  andi $t5, $t4, 16448                               ## L:482  |    108 | clipB = temp1 & 0b0100'0000'0100'0000;
  vmudl $v12, $v12, $v17.v                           ## L:485  |      ^ | posClip *= normScaleW:ufract;
  vmadm $v11, $v11, $v17.v                           ## L:485  |    109 | posClip *= normScaleW:ufract;
  vmadn $v12, $v00, $v00                             ## L:485  |    110 | posClip *= normScaleW:ufract;
  vrcph $v07.e3, $v11.e3                             ## L:487  |  **113 | invW.w = invert_half(posClip).w;
  vrcpl $v08.e3, $v12.e3                             ## L:487  |    114 | invW.w = invert_half(posClip).w;
  vrcph $v07.e3, $v11.e7                             ## L:488  |    115 | invW.W = invert_half(posClip).W;
  vrcpl $v08.e7, $v12.e7                             ## L:488  |    116 | invW.W = invert_half(posClip).W;
  vrcph $v07.e7, $v00.e7                             ## L:488  |    117 | invW.W = invert_half(posClip).W;
  ori $at, $zero, %lo(BASE_SIZE)                     ## L:491  |      ^ | posClip:sint.w = load(BASE_SIZE).x;
  lsv $v11, 6, 0, $at                                ## L:491  |    118 | posClip:sint.w = load(BASE_SIZE).x;
  lsv $v11, 14, 0, $at                               ## L:492  |    119 | posClip:sint.W = load(BASE_SIZE).x;
  vmov $v12.e3, $v00.e0                              ## L:493  |      ^ | posClip:sfract.w = 0;
  vmov $v12.e7, $v00.e0                              ## L:494  |    120 | posClip:sfract.W = 0;
  vmudl $v29, $v12, $v08.h3                          ## L:497  | ***124 | posClip *= invW.wwwwWWWW;
  vmadm $v29, $v11, $v08.h3                          ## L:497  |    125 | posClip *= invW.wwwwWWWW;
  vmadn $v12, $v12, $v07.h3                          ## L:497  |    126 | posClip *= invW.wwwwWWWW;
  vmadh $v11, $v11, $v07.h3                          ## L:497  |    127 | posClip *= invW.wwwwWWWW;
  vmulf $v14, $v14, $v11.h3                          ## L:501  | ***131 | localPartSize:sfract *= posClip:sint.wwwwWWWW;
  vmudl $v29, $v12, $v20.v                           ## L:504  |    132 | vec32 posScreen = posClip * screenSize;
  vmadm $v29, $v11, $v20.v                           ## L:504  |    133 | vec32 posScreen = posClip * screenSize;
  vmadn $v08, $v12, $v19.v                           ## L:504  |    134 | vec32 posScreen = posClip * screenSize;
  vmadh $v07, $v11, $v19.v                           ## L:504  |    135 | vec32 posScreen = posClip * screenSize;
  vmadh $v06, $v18, $v10.v                           ## L:505  |    136 | vec16 posCenter = screenOffset:sint +* vecOne;
  vmadh $v05, $v10, $v14.v                           ## L:508  |    137 | vec16 posEnd = vecOne +* localPartSize:sint;
  vsubc $v09, $v06, $v14.v                           ## L:509  |  **140 | posStart = posCenter - localPartSize:sint;
  vlt $v05, $v05, $v15                               ## L:512  |    141 | posEnd = min(posEnd, screenMax);
  vge $v09, $v09, $v16                               ## L:513  |  **144 | posStart = max(posStart, screenMin);
  mfc2 $s2, $v09.e1                                  ## L:1190 | ***148 | outA = pos.y;
  andi $s2, $s2, 4095                                ## L:1191 |  **151 | outA &= 0b1111'1111'1111;
  mfc2 $k1, $v09.e0                                  ## L:1192 |    152 | u32 tmp = pos.x;
  sll $k1, $k1, 12                                   ## L:1193 |  **155 | tmp <<= 12;
  or $s2, $s2, $k1                                   ## L:1194 |    156 | outA |= tmp;
  mfc2 $s1, $v09.e5                                  ## L:1196 |    157 | outB = pos.Y;
  andi $s1, $s1, 4095                                ## L:1197 |  **160 | outB &= 0b1111'1111'1111;
  mfc2 $k1, $v09.e4                                  ## L:1198 |    161 | tmp = pos.X;
  sll $k1, $k1, 12                                   ## L:1199 |  **164 | tmp <<= 12;
  or $s1, $s1, $k1                                   ## L:1200 |    165 | outB |= tmp;
  mfc2 $fp, $v05.e1                                  ## L:1190 |    166 | outA = pos.y;
  andi $fp, $fp, 4095                                ## L:1191 |  **169 | outA &= 0b1111'1111'1111;
  mfc2 $k1, $v05.e0                                  ## L:1192 |    170 | u32 tmp = pos.x;
  sll $k1, $k1, 12                                   ## L:1193 |  **173 | tmp <<= 12;
  or $fp, $fp, $k1                                   ## L:1194 |    174 | outA |= tmp;
  mfc2 $sp, $v05.e5                                  ## L:1196 |    175 | outB = pos.Y;
  andi $sp, $sp, 4095                                ## L:1197 |  **178 | outB &= 0b1111'1111'1111;
  mfc2 $k1, $v05.e4                                  ## L:1198 |    179 | tmp = pos.X;
  sll $k1, $k1, 12                                   ## L:1199 |  **182 | tmp <<= 12;
  vlt $v04, $v09, $v05                               ## L:519  |      ^ | vec16 extend = posStart < posEnd;
  cfc2 $t4, $vcc                                     ## L:520  |    183 | temp1 = get_vcc();
  andi $a2, $t4, 3                                   ## L:522  |  **186 | temp0 = temp1 & 0b0000'0011;
  or $t1, $t1, $a2                                   ## L:523  |    187 | clipA |= temp0;
  andi $a2, $t4, 48                                  ## L:525  |    188 | temp0 = temp1 & 0b0011'0000;
  or $t5, $t5, $a2                                   ## L:526  |    189 | clipB |= temp0;
  ldv $v03, 0, 16, $s6                               ## L:530  |    190 | vec16 color = load(ptrIn, 16).xyzw;
  addiu $t2, $zero, 54                               ## L:534  |    191 | cmdRdpRect = 0x36;
  slv $v03, 0, 4, $s3                                ## L:542  |  **194 | store(color.xy, dmaDmemEnd, 4);
  ssv $v06, 4, 12, $s3                               ## L:546  |    195 | store(posCenter.z, dmaDmemEnd, 0x04, 8);
  sw $s2, 4 + 16($s3)                                ## L:550  |    196 | store(posA, dmaDmemEnd, 0x04, 16);
  sw $fp, 0 + 16($s3)                                ## L:551  |    197 | @Barrier("pos-cmd") store(posEndA, dmaDmemEnd, 0x00, 16); ## Barrier: 0x1
  sb $t2, 0 + 16($s3)                                ## L:552  |    198 | @Barrier("pos-cmd") store(cmdRdpRect, dmaDmemEnd, 0x00, 16); ## Barrier: 0x1
  ldv $v09, 0, 24, $s6                               ## L:557  |    199 | posStart.xyzw = load(ptrIn, 24).xyzw;
  ldv $v09, 8, 32, $s6                               ## L:558  |    200 | posStart.XYZW = load(ptrIn, 24).XYZW;
  vmulf $v14, $v13, $v09.h3                          ## L:559  | ***204 | localPartSize = globalPartSize:sfract * posStart:sfract.wwwwWWWW;
  addiu $at, $zero, 3                                ## L:566  |      ^ | if(clipA == 0b0000'0011) {
  bne $t1, $at, LABEL_TPXCmd_DrawColor_0007          ## L:566  |    205 | if(clipA == 0b0000'0011) {
  or $sp, $sp, $k1                                   ## L:1200 |   *207 | outB |= tmp;
  addiu $s3, $s3, 24                                 ## L:567  |    208 | dmaDmemEnd += 24;
  LABEL_TPXCmd_DrawColor_0007:
  addiu $at, $zero, 48                               ## L:577  |    209 | if(clipB == 0b0011'0000) {
  bne $t5, $at, LABEL_TPXCmd_DrawColor_0008          ## L:577  |    210 | if(clipB == 0b0011'0000) {
  addiu $s6, $s6, 24                                 ## L:571  |   *212 | ptrIn += 24;
  slv $v03, 4, 4, $s3                                ## L:578  |    213 | store(color.zw, dmaDmemEnd, 4);
  ssv $v06, 12, 12, $s3                              ## L:579  |    214 | store(posCenter.Z, dmaDmemEnd, 0x04, 8);
  sw $s1, 4 + 16($s3)                                ## L:581  |    215 | store(posB, dmaDmemEnd, 0x04, 16);
  sw $sp, 0 + 16($s3)                                ## L:582  |    216 | @Barrier("pos-cmd") store(posEndB, dmaDmemEnd, 0x00, 16); ## Barrier: 0x1
  sb $t2, 0 + 16($s3)                                ## L:583  |    217 | @Barrier("pos-cmd") store(cmdRdpRect, dmaDmemEnd, 0x00, 16); ## Barrier: 0x1
  addiu $s3, $s3, 24                                 ## L:585  |    218 | dmaDmemEnd += 24;
  LABEL_TPXCmd_DrawColor_0008:
  vmudn $v12, $v28, $v09.h0                          ## L:92   |      ^ | out = mat0  * vec.xxxxXXXX;
  vmadh $v11, $v27, $v09.h0                          ## L:92   |    219 | out = mat0  * vec.xxxxXXXX;
  vmadn $v12, $v26, $v09.h1                          ## L:93   |    220 | out = mat1 +* vec.yyyyYYYY;
  vmadh $v11, $v25, $v09.h1                          ## L:93   |    221 | out = mat1 +* vec.yyyyYYYY;
  vmadn $v12, $v24, $v09.h2                          ## L:94   |    222 | out = mat2 +* vec.zzzzZZZZ;
  vmadh $v11, $v23, $v09.h2                          ## L:94   |    223 | out = mat2 +* vec.zzzzZZZZ;
  vmadn $v12, $v22, $v30.e7                          ## L:95   |    224 | out = mat3 +* 1;
  sltu $at, $s3, $s5                                 ## L:592  |      ^ | if(dmaDmemEnd >= dmaDmemFlush) {
  bne $at, $zero, LABEL_TPXCmd_DrawColor_0009        ## L:592  |    225 | if(dmaDmemEnd >= dmaDmemFlush) {
  vmadh $v11, $v21, $v30.e7                          ## L:95   |   *227 | out = mat3 +* 1;
  jal RDPQ_Send                                      ## L:593  |    228 | RDPQ_Send(dmaDmem, dmaDmemEnd); ## Args: $s4, $s3
  nop                                                ## L:593  |   *230 | RDPQ_Send(dmaDmem, dmaDmemEnd);
  or $s3, $zero, $s4                                 ## L:594  |    231 | dmaDmemEnd = dmaDmem;
  LABEL_TPXCmd_DrawColor_0009:
  vch $v29, $v11, $v11.h3                            ## L:597  |      ^ | temp1 = clip(posClip, posClip.wwwwWWWW);
  vcl $v29, $v12, $v12.h3                            ## L:597  |    232 | temp1 = clip(posClip, posClip.wwwwWWWW);
  bne $s6, $s7, LABEL_TPXCmd_DrawColor_0005          ## L:597  |      ^ | temp1 = clip(posClip, posClip.wwwwWWWW);
  cfc2 $t4, $vcc                                     ## L:597  |   *234 | temp1 = clip(posClip, posClip.wwwwWWWW);
  LABEL_TPXCmd_DrawColor_0006:
  beq $zero, $zero, LABEL_TPXCmd_DrawColor_0004      ## L:597  |    235 | temp1 = clip(posClip, posClip.wwwwWWWW);
  nop                                                ## L:597  |   *237 | temp1 = clip(posClip, posClip.wwwwWWWW);
  LABEL_TPXCmd_DrawColor_0003:
  lpv $v09, 0, 0, $s6                                ## L:115  |    238 | vec16 posStart = load_vec_s8(ptrIn, 0x00);
  vmulf $v14, $v13, $v09.h3                          ## L:116  | ***242 | localPartSize = globalPartSize:sfract * posStart:sfract.wwwwWWWW;
  vmudm $v09, $v09, $v31.e7                          ## L:117  |    243 | posStart >>= 8;
  vmudn $v12, $v28, $v09.h0                          ## L:92   | ***247 | out = mat0  * vec.xxxxXXXX;
  vmadh $v11, $v27, $v09.h0                          ## L:92   |    248 | out = mat0  * vec.xxxxXXXX;
  vmadn $v12, $v26, $v09.h1                          ## L:93   |    249 | out = mat1 +* vec.yyyyYYYY;
  vmadh $v11, $v25, $v09.h1                          ## L:93   |    250 | out = mat1 +* vec.yyyyYYYY;
  vmadn $v12, $v24, $v09.h2                          ## L:94   |    251 | out = mat2 +* vec.zzzzZZZZ;
  vmadh $v11, $v23, $v09.h2                          ## L:94   |    252 | out = mat2 +* vec.zzzzZZZZ;
  vmadn $v12, $v22, $v30.e7                          ## L:95   |    253 | out = mat3 +* 1;
  vmadh $v11, $v21, $v30.e7                          ## L:95   |    254 | out = mat3 +* 1;
  vch $v29, $v11, $v11.h3                            ## L:121  | ***258 | temp1 = clip(posClip, posClip.wwwwWWWW);
  vcl $v29, $v12, $v12.h3                            ## L:121  |    259 | temp1 = clip(posClip, posClip.wwwwWWWW);
  cfc2 $t4, $vcc                                     ## L:121  |    260 | temp1 = clip(posClip, posClip.wwwwWWWW);
  LABEL_TPXCmd_DrawColor_000A:
  andi $t1, $t4, 1028                                ## L:128  |  **263 | clipA = temp1 & 0b0000'0100'0000'0100;
  andi $t5, $t4, 16448                               ## L:129  |    264 | clipB = temp1 & 0b0100'0000'0100'0000;
  vmudl $v12, $v12, $v17.v                           ## L:132  |      ^ | posClip *= normScaleW:ufract;
  vmadm $v11, $v11, $v17.v                           ## L:132  |    265 | posClip *= normScaleW:ufract;
  vmadn $v12, $v00, $v00                             ## L:132  |    266 | posClip *= normScaleW:ufract;
  vrcph $v07.e3, $v11.e3                             ## L:134  |  **269 | invW.w = invert_half(posClip).w;
  vrcpl $v08.e3, $v12.e3                             ## L:134  |    270 | invW.w = invert_half(posClip).w;
  vrcph $v07.e3, $v11.e7                             ## L:135  |    271 | invW.W = invert_half(posClip).W;
  vrcpl $v08.e7, $v12.e7                             ## L:135  |    272 | invW.W = invert_half(posClip).W;
  vrcph $v07.e7, $v00.e7                             ## L:135  |    273 | invW.W = invert_half(posClip).W;
  ori $at, $zero, %lo(BASE_SIZE)                     ## L:138  |      ^ | posClip:sint.w = load(BASE_SIZE).x;
  lsv $v11, 6, 0, $at                                ## L:138  |    274 | posClip:sint.w = load(BASE_SIZE).x;
  lsv $v11, 14, 0, $at                               ## L:139  |    275 | posClip:sint.W = load(BASE_SIZE).x;
  vmov $v12.e3, $v00.e0                              ## L:140  |      ^ | posClip:sfract.w = 0;
  vmov $v12.e7, $v00.e0                              ## L:141  |    276 | posClip:sfract.W = 0;
  vmudl $v29, $v12, $v08.h3                          ## L:144  | ***280 | posClip *= invW.wwwwWWWW;
  vmadm $v29, $v11, $v08.h3                          ## L:144  |    281 | posClip *= invW.wwwwWWWW;
  vmadn $v12, $v12, $v07.h3                          ## L:144  |    282 | posClip *= invW.wwwwWWWW;
  vmadh $v11, $v11, $v07.h3                          ## L:144  |    283 | posClip *= invW.wwwwWWWW;
  vmulf $v14, $v14, $v11.h3                          ## L:148  | ***287 | localPartSize:sfract *= posClip:sint.wwwwWWWW;
  vmudl $v29, $v12, $v20.v                           ## L:151  |    288 | vec32 posScreen = posClip * screenSize;
  vmadm $v29, $v11, $v20.v                           ## L:151  |    289 | vec32 posScreen = posClip * screenSize;
  vmadn $v08, $v12, $v19.v                           ## L:151  |    290 | vec32 posScreen = posClip * screenSize;
  vmadh $v07, $v11, $v19.v                           ## L:151  |    291 | vec32 posScreen = posClip * screenSize;
  vmadh $v06, $v18, $v10.v                           ## L:152  |    292 | vec16 posCenter = screenOffset:sint +* vecOne;
  vmadh $v05, $v10, $v14.v                           ## L:155  |    293 | vec16 posEnd = vecOne +* localPartSize:sint;
  vsubc $v09, $v06, $v14.v                           ## L:156  |  **296 | posStart = posCenter - localPartSize:sint;
  vlt $v05, $v05, $v15                               ## L:159  |    297 | posEnd = min(posEnd, screenMax);
  vge $v09, $v09, $v16                               ## L:160  |  **300 | posStart = max(posStart, screenMin);
  mfc2 $s2, $v09.e1                                  ## L:1190 | ***304 | outA = pos.y;
  andi $s2, $s2, 4095                                ## L:1191 |  **307 | outA &= 0b1111'1111'1111;
  mfc2 $k1, $v09.e0                                  ## L:1192 |    308 | u32 tmp = pos.x;
  sll $k1, $k1, 12                                   ## L:1193 |  **311 | tmp <<= 12;
  or $s2, $s2, $k1                                   ## L:1194 |    312 | outA |= tmp;
  mfc2 $s1, $v09.e5                                  ## L:1196 |    313 | outB = pos.Y;
  andi $s1, $s1, 4095                                ## L:1197 |  **316 | outB &= 0b1111'1111'1111;
  mfc2 $k1, $v09.e4                                  ## L:1198 |    317 | tmp = pos.X;
  sll $k1, $k1, 12                                   ## L:1199 |  **320 | tmp <<= 12;
  or $s1, $s1, $k1                                   ## L:1200 |    321 | outB |= tmp;
  mfc2 $fp, $v05.e1                                  ## L:1190 |    322 | outA = pos.y;
  andi $fp, $fp, 4095                                ## L:1191 |  **325 | outA &= 0b1111'1111'1111;
  mfc2 $k1, $v05.e0                                  ## L:1192 |    326 | u32 tmp = pos.x;
  sll $k1, $k1, 12                                   ## L:1193 |  **329 | tmp <<= 12;
  or $fp, $fp, $k1                                   ## L:1194 |    330 | outA |= tmp;
  mfc2 $sp, $v05.e5                                  ## L:1196 |    331 | outB = pos.Y;
  andi $sp, $sp, 4095                                ## L:1197 |  **334 | outB &= 0b1111'1111'1111;
  mfc2 $k1, $v05.e4                                  ## L:1198 |    335 | tmp = pos.X;
  sll $k1, $k1, 12                                   ## L:1199 |  **338 | tmp <<= 12;
  vlt $v04, $v09, $v05                               ## L:166  |      ^ | vec16 extend = posStart < posEnd;
  cfc2 $t4, $vcc                                     ## L:167  |    339 | temp1 = get_vcc();
  andi $a2, $t4, 3                                   ## L:169  |  **342 | temp0 = temp1 & 0b0000'0011;
  or $t1, $t1, $a2                                   ## L:170  |    343 | clipA |= temp0;
  andi $a2, $t4, 48                                  ## L:172  |    344 | temp0 = temp1 & 0b0011'0000;
  or $t5, $t5, $a2                                   ## L:173  |    345 | clipB |= temp0;
  ldv $v03, 0, 8, $s6                                ## L:179  |    346 | vec16 color = load(ptrIn, 8).xyzw;
  addiu $t2, $zero, 54                               ## L:181  |    347 | cmdRdpRect = 0x36;
  slv $v03, 0, 4, $s3                                ## L:189  |  **350 | store(color.xy, dmaDmemEnd, 4);
  ssv $v06, 4, 12, $s3                               ## L:193  |    351 | store(posCenter.z, dmaDmemEnd, 0x04, 8);
  sw $s2, 4 + 16($s3)                                ## L:197  |    352 | store(posA, dmaDmemEnd, 0x04, 16);
  sw $fp, 0 + 16($s3)                                ## L:198  |    353 | @Barrier("pos-cmd") store(posEndA, dmaDmemEnd, 0x00, 16); ## Barrier: 0x1
  sb $t2, 0 + 16($s3)                                ## L:199  |    354 | @Barrier("pos-cmd") store(cmdRdpRect, dmaDmemEnd, 0x00, 16); ## Barrier: 0x1
  lpv $v09, 0, 16, $s6                               ## L:208  |    355 | posStart = load_vec_s8(ptrIn, 16);
  vmulf $v14, $v13, $v09.h3                          ## L:209  | ***359 | localPartSize = globalPartSize:sfract * posStart:sfract.wwwwWWWW;
  vmudm $v09, $v09, $v31.e7                          ## L:210  |    360 | posStart >>= 8;
  addiu $at, $zero, 3                                ## L:213  |      ^ | if(clipA == 0b0000'0011) {
  bne $t1, $at, LABEL_TPXCmd_DrawColor_000C          ## L:213  |    361 | if(clipA == 0b0000'0011) {
  or $sp, $sp, $k1                                   ## L:1200 |   *363 | outB |= tmp;
  addiu $s3, $s3, 24                                 ## L:214  |    364 | dmaDmemEnd += 24;
  LABEL_TPXCmd_DrawColor_000C:
  addiu $at, $zero, 48                               ## L:224  |    365 | if(clipB == 0b0011'0000) {
  bne $t5, $at, LABEL_TPXCmd_DrawColor_000D          ## L:224  |    366 | if(clipB == 0b0011'0000) {
  addiu $s6, $s6, 16                                 ## L:220  |   *368 | ptrIn += 16;
  slv $v03, 4, 4, $s3                                ## L:225  |    369 | store(color.zw, dmaDmemEnd, 4);
  ssv $v06, 12, 12, $s3                              ## L:226  |    370 | store(posCenter.Z, dmaDmemEnd, 0x04, 8);
  sw $s1, 4 + 16($s3)                                ## L:228  |    371 | store(posB, dmaDmemEnd, 0x04, 16);
  sw $sp, 0 + 16($s3)                                ## L:229  |    372 | @Barrier("pos-cmd") store(posEndB, dmaDmemEnd, 0x00, 16); ## Barrier: 0x1
  sb $t2, 0 + 16($s3)                                ## L:230  |    373 | @Barrier("pos-cmd") store(cmdRdpRect, dmaDmemEnd, 0x00, 16); ## Barrier: 0x1
  addiu $s3, $s3, 24                                 ## L:232  |    374 | dmaDmemEnd += 24;
  LABEL_TPXCmd_DrawColor_000D:
  vmudn $v12, $v28, $v09.h0                          ## L:92   |      ^ | out = mat0  * vec.xxxxXXXX;
  vmadh $v11, $v27, $v09.h0                          ## L:92   |    375 | out = mat0  * vec.xxxxXXXX;
  vmadn $v12, $v26, $v09.h1                          ## L:93   |    376 | out = mat1 +* vec.yyyyYYYY;
  vmadh $v11, $v25, $v09.h1                          ## L:93   |    377 | out = mat1 +* vec.yyyyYYYY;
  vmadn $v12, $v24, $v09.h2                          ## L:94   |    378 | out = mat2 +* vec.zzzzZZZZ;
  vmadh $v11, $v23, $v09.h2                          ## L:94   |    379 | out = mat2 +* vec.zzzzZZZZ;
  vmadn $v12, $v22, $v30.e7                          ## L:95   |    380 | out = mat3 +* 1;
  sltu $at, $s3, $s5                                 ## L:239  |      ^ | if(dmaDmemEnd >= dmaDmemFlush) {
  bne $at, $zero, LABEL_TPXCmd_DrawColor_000E        ## L:239  |    381 | if(dmaDmemEnd >= dmaDmemFlush) {
  vmadh $v11, $v21, $v30.e7                          ## L:95   |   *383 | out = mat3 +* 1;
  jal RDPQ_Send                                      ## L:240  |    384 | RDPQ_Send(dmaDmem, dmaDmemEnd); ## Args: $s4, $s3
  nop                                                ## L:240  |   *386 | RDPQ_Send(dmaDmem, dmaDmemEnd);
  or $s3, $zero, $s4                                 ## L:241  |    387 | dmaDmemEnd = dmaDmem;
  LABEL_TPXCmd_DrawColor_000E:
  vch $v29, $v11, $v11.h3                            ## L:244  |      ^ | temp1 = clip(posClip, posClip.wwwwWWWW);
  vcl $v29, $v12, $v12.h3                            ## L:244  |    388 | temp1 = clip(posClip, posClip.wwwwWWWW);
  bne $s6, $s7, LABEL_TPXCmd_DrawColor_000A          ## L:244  |      ^ | temp1 = clip(posClip, posClip.wwwwWWWW);
  cfc2 $t4, $vcc                                     ## L:244  |   *390 | temp1 = clip(posClip, posClip.wwwwWWWW);
  LABEL_TPXCmd_DrawColor_0004:
  j RDPQ_Send                                        ## L:907  |    391 | goto RDPQ_Send;
  ori $ra, $zero, %lo(RSPQ_Loop)                     ## L:906  |   *393 | RA = RSPQ_Loop;
mulMat4Mat4:
  ldv $v01, 0, 0, $s3                                ## L:932  |      ^ | vec32 matL0 = load(addrMatL, 0x00).xyzwxyzw;
  ldv $v01, 8, 0, $s3                                ## L:932  |      2 | vec32 matL0 = load(addrMatL, 0x00).xyzwxyzw;
  ldv $v02, 0, 8, $s3                                ## L:932  |      3 | vec32 matL0 = load(addrMatL, 0x00).xyzwxyzw;
  ldv $v02, 8, 8, $s3                                ## L:932  |      4 | vec32 matL0 = load(addrMatL, 0x00).xyzwxyzw;
  ldv $v03, 0, 16, $s3                               ## L:933  |      5 | vec32 matL1 = load(addrMatL, 0x10).xyzwxyzw;
  ldv $v03, 8, 16, $s3                               ## L:933  |      6 | vec32 matL1 = load(addrMatL, 0x10).xyzwxyzw;
  ldv $v04, 0, 24, $s3                               ## L:933  |      7 | vec32 matL1 = load(addrMatL, 0x10).xyzwxyzw;
  ldv $v04, 8, 24, $s3                               ## L:933  |      8 | vec32 matL1 = load(addrMatL, 0x10).xyzwxyzw;
  ldv $v05, 0, 32, $s3                               ## L:934  |      9 | vec32 matL2 = load(addrMatL, 0x20).xyzwxyzw;
  ldv $v05, 8, 32, $s3                               ## L:934  |     10 | vec32 matL2 = load(addrMatL, 0x20).xyzwxyzw;
  ldv $v06, 0, 40, $s3                               ## L:934  |     11 | vec32 matL2 = load(addrMatL, 0x20).xyzwxyzw;
  ldv $v06, 8, 40, $s3                               ## L:934  |     12 | vec32 matL2 = load(addrMatL, 0x20).xyzwxyzw;
  ldv $v07, 0, 48, $s3                               ## L:935  |     13 | vec32 matL3 = load(addrMatL, 0x30).xyzwxyzw;
  ldv $v07, 8, 48, $s3                               ## L:935  |     14 | vec32 matL3 = load(addrMatL, 0x30).xyzwxyzw;
  ldv $v08, 0, 56, $s3                               ## L:935  |     15 | vec32 matL3 = load(addrMatL, 0x30).xyzwxyzw;
  ldv $v08, 8, 56, $s3                               ## L:935  |     16 | vec32 matL3 = load(addrMatL, 0x30).xyzwxyzw;
  ldv $v09, 0, 0, $s4                                ## L:939  |     17 | matR01.xyzw = load(addrMatR, 0x00).xyzw;
  ldv $v10, 0, 8, $s4                                ## L:939  |     18 | matR01.xyzw = load(addrMatR, 0x00).xyzw;
  ldv $v09, 8, 16, $s4                               ## L:940  |     19 | matR01.XYZW = load(addrMatR, 0x10).xyzw;
  ldv $v10, 8, 24, $s4                               ## L:940  |     20 | matR01.XYZW = load(addrMatR, 0x10).xyzw;
  ldv $v11, 0, 32, $s4                               ## L:941  |     21 | matR23.xyzw = load(addrMatR, 0x20).xyzw;
  ldv $v12, 0, 40, $s4                               ## L:941  |     22 | matR23.xyzw = load(addrMatR, 0x20).xyzw;
  ldv $v11, 8, 48, $s4                               ## L:942  |     23 | matR23.XYZW = load(addrMatR, 0x30).xyzw;
  ldv $v12, 8, 56, $s4                               ## L:942  |     24 | matR23.XYZW = load(addrMatR, 0x30).xyzw;
  vmudl $v29, $v02, $v10.h0                          ## L:945  |      ^ | tmp    = matL0  * matR01.xxxxXXXX;
  vmadm $v29, $v01, $v10.h0                          ## L:945  |     25 | tmp    = matL0  * matR01.xxxxXXXX;
  vmadn $v14, $v02, $v09.h0                          ## L:945  |     26 | tmp    = matL0  * matR01.xxxxXXXX;
  vmadh $v13, $v01, $v09.h0                          ## L:945  |     27 | tmp    = matL0  * matR01.xxxxXXXX;
  vmadl $v29, $v04, $v10.h1                          ## L:946  |     28 | tmp    = matL1 +* matR01.yyyyYYYY;
  vmadm $v29, $v03, $v10.h1                          ## L:946  |     29 | tmp    = matL1 +* matR01.yyyyYYYY;
  vmadn $v14, $v04, $v09.h1                          ## L:946  |     30 | tmp    = matL1 +* matR01.yyyyYYYY;
  vmadh $v13, $v03, $v09.h1                          ## L:946  |     31 | tmp    = matL1 +* matR01.yyyyYYYY;
  vmadl $v29, $v06, $v10.h2                          ## L:947  |     32 | tmp    = matL2 +* matR01.zzzzZZZZ;
  vmadm $v29, $v05, $v10.h2                          ## L:947  |     33 | tmp    = matL2 +* matR01.zzzzZZZZ;
  vmadn $v14, $v06, $v09.h2                          ## L:947  |     34 | tmp    = matL2 +* matR01.zzzzZZZZ;
  vmadh $v13, $v05, $v09.h2                          ## L:947  |     35 | tmp    = matL2 +* matR01.zzzzZZZZ;
  vmadl $v29, $v08, $v10.h3                          ## L:948  |     36 | matR01 = matL3 +* matR01.wwwwWWWW;
  vmadm $v29, $v07, $v10.h3                          ## L:948  |     37 | matR01 = matL3 +* matR01.wwwwWWWW;
  vmadn $v10, $v08, $v09.h3                          ## L:948  |     38 | matR01 = matL3 +* matR01.wwwwWWWW;
  vmadh $v09, $v07, $v09.h3                          ## L:948  |     39 | matR01 = matL3 +* matR01.wwwwWWWW;
  vmudl $v29, $v02, $v12.h0                          ## L:950  |     40 | tmp    = matL0  * matR23.xxxxXXXX;
  vmadm $v29, $v01, $v12.h0                          ## L:950  |     41 | tmp    = matL0  * matR23.xxxxXXXX;
  vmadn $v14, $v02, $v11.h0                          ## L:950  |     42 | tmp    = matL0  * matR23.xxxxXXXX;
  vmadh $v13, $v01, $v11.h0                          ## L:950  |     43 | tmp    = matL0  * matR23.xxxxXXXX;
  vmadl $v29, $v04, $v12.h1                          ## L:951  |     44 | tmp    = matL1 +* matR23.yyyyYYYY;
  vmadm $v29, $v03, $v12.h1                          ## L:951  |     45 | tmp    = matL1 +* matR23.yyyyYYYY;
  vmadn $v14, $v04, $v11.h1                          ## L:951  |     46 | tmp    = matL1 +* matR23.yyyyYYYY;
  vmadh $v13, $v03, $v11.h1                          ## L:951  |     47 | tmp    = matL1 +* matR23.yyyyYYYY;
  vmadl $v29, $v06, $v12.h2                          ## L:952  |     48 | tmp    = matL2 +* matR23.zzzzZZZZ;
  vmadm $v29, $v05, $v12.h2                          ## L:952  |     49 | tmp    = matL2 +* matR23.zzzzZZZZ;
  vmadn $v14, $v06, $v11.h2                          ## L:952  |     50 | tmp    = matL2 +* matR23.zzzzZZZZ;
  vmadh $v13, $v05, $v11.h2                          ## L:952  |     51 | tmp    = matL2 +* matR23.zzzzZZZZ;
  vmadl $v29, $v08, $v12.h3                          ## L:953  |     52 | matR23 = matL3 +* matR23.wwwwWWWW;
  vmadm $v29, $v07, $v12.h3                          ## L:953  |     53 | matR23 = matL3 +* matR23.wwwwWWWW;
  vmadn $v12, $v08, $v11.h3                          ## L:953  |     54 | matR23 = matL3 +* matR23.wwwwWWWW;
  vmadh $v11, $v07, $v11.h3                          ## L:953  |     55 | matR23 = matL3 +* matR23.wwwwWWWW;
  sdv $v10, 0, 8, $s2                                ## L:955  |      ^ | store(matR01.xyzw, addrOut, 0x00);
  sdv $v09, 8, 16, $s2                               ## L:956  |     56 | store(matR01.XYZW, addrOut, 0x10);
  sdv $v10, 8, 24, $s2                               ## L:956  |     57 | store(matR01.XYZW, addrOut, 0x10);
  sdv $v11, 0, 32, $s2                               ## L:957  |    *59 | store(matR23.xyzw, addrOut, 0x20);
  sdv $v12, 0, 40, $s2                               ## L:957  |     60 | store(matR23.xyzw, addrOut, 0x20);
  sdv $v11, 8, 48, $s2                               ## L:958  |     61 | store(matR23.XYZW, addrOut, 0x30);
  sdv $v12, 8, 56, $s2                               ## L:958  |     62 | store(matR23.XYZW, addrOut, 0x30);
  jr $ra                                             ## L:959  |     63 | }
  sdv $v09, 0, 0, $s2                                ## L:955  |    *65 | store(matR01.xyzw, addrOut, 0x00);
T3DCmd_MatrixStack:
  andi $s7, $a0, 1                                   ## L:1005 |      ^ | u16 doMultiply = stackAdvance & 0b01;
  andi $s6, $a0, 2                                   ## L:1006 |      2 | u16 onlyStackMove = stackAdvance & 0b10;
  lh $a0, %lo(RSPQ_DMEM_BUFFER  -7)($gp)             ## L:1008 |      3 | stackAdvance:s16 = load_arg(1);
  lw $s5, %lo(MATRIX_STACK_PTR + 0)                  ## L:1010 |      4 | s32 stackPtr = load(MATRIX_STACK_PTR);
  addu $s5, $s5, $a0                                 ## L:1011 |    **7 | stackPtr += stackAdvance;
  bne $s6, $zero, RSPQ_Loop                          ## L:1015 |      8 | if(onlyStackMove)exit;
  sw $s5, %lo(MATRIX_STACK_PTR)($zero)               ## L:1012 |    *10 | store(stackPtr, MATRIX_STACK_PTR);
  bgez $a0, LABEL_T3DCmd_MatrixStack_0010            ## L:1020 |     11 | if(stackAdvance < 0)addressMat = stackPtr;
  ori $s4, $zero, %lo(RDP_BUFF)                      ## L:1016 |    *13 | dmaDest = RDP_BUFF;
  or $a1, $zero, $s5                                 ## L:1020 |     14 | if(stackAdvance < 0)addressMat = stackPtr;
  LABEL_T3DCmd_MatrixStack_0010:
  lui $at, 0xFF                                      ## L:1021 |     15 | u32<$s0> addrRDRAM = addressMat & 0xFFFFFF;
  ori $at, $at, 0xFFFF                               ## L:1021 |     16 | u32<$s0> addrRDRAM = addressMat & 0xFFFFFF;
  ori $t0, $zero, 63                                 ## L:1023 |     17 | dma_in(dmaDest, addrRDRAM, 0x40);
  addiu $t2, $zero, 12                               ## L:1023 |     18 | dma_in(dmaDest, addrRDRAM, 0x40);
  jal DMAExec                                        ## L:1023 |     19 | dma_in(dmaDest, addrRDRAM, 0x40); ## Args: $t0, $t1, $s0, $s4, $t2
  and $s0, $a1, $at                                  ## L:1021 |    *21 | u32<$s0> addrRDRAM = addressMat & 0xFFFFFF;
  beq $s7, $zero, LABEL_T3DCmd_MatrixStack_0011      ## L:1030 |     22 | if(doMultiply) {
  nop                                                ## L:1030 |    *24 | if(doMultiply) {
  addiu $s0, $s5, 65472                              ## L:1033 |     25 | addrRDRAM = stackPtr -  0x40;
  ori $t0, $zero, 63                                 ## L:1034 |     26 | dma_in(dmaDest, addrRDRAM, 0x40);
  addiu $t2, $zero, 12                               ## L:1034 |     27 | dma_in(dmaDest, addrRDRAM, 0x40);
  jal DMAExec                                        ## L:1034 |     28 | dma_in(dmaDest, addrRDRAM, 0x40); ## Args: $t0, $t1, $s0, $s4, $t2
  ori $s4, $zero, %lo(PARTICLE_BUFF)                 ## L:1032 |    *30 | dmaDest = PARTICLE_BUFF;
  ori $s3, $zero, %lo(PARTICLE_BUFF)                 ## L:1038 |     31 | u32<$s3> mulLeft = PARTICLE_BUFF;
  ori $s4, $zero, %lo(RDP_BUFF)                      ## L:1039 |     32 | dmaDest = RDP_BUFF;
  jal mulMat4Mat4                                    ## L:1040 |     33 | mulMat4Mat4(mulDest, mulLeft, dmaDest); ## Args: $s2, $s3, $s4
  ori $s2, $zero, %lo(RDP_BUFF)                      ## L:1037 |    *35 | u32<$s2> mulDest = RDP_BUFF;
  LABEL_T3DCmd_MatrixStack_0011:
  ori $t0, $zero, 63                                 ## L:1045 |     36 | dma_out(dmaDest, addrRDRAM, 0x40);
  addiu $t2, $zero, -32756                           ## L:1045 |     37 | dma_out(dmaDest, addrRDRAM, 0x40);
  jal DMAExec                                        ## L:1045 |     38 | dma_out(dmaDest, addrRDRAM, 0x40); ## Args: $t0, $t1, $s0, $s4, $t2
  or $s0, $zero, $s5                                 ## L:1044 |    *40 | addrRDRAM = stackPtr;
  ori $s3, $zero, %lo(MATRIX_PROJ)                   ## L:1053 |     41 | u32<$s3> mulLeft = MATRIX_PROJ;
  ori $s4, $zero, %lo(RDP_BUFF)                      ## L:1062 |     42 | u32<$s4> mulRight = RDP_BUFF;
  jal mulMat4Mat4                                    ## L:1063 |     43 | mulMat4Mat4(mulDest, mulLeft, mulRight); ## Args: $s2, $s3, $s4
  ori $s2, $zero, %lo(MATRIX_MVP)                    ## L:1049 |    *45 | u32<$s2> mulDest = MATRIX_MVP;
  j RSPQ_Loop                                        ## L:1064 |     46 | }
  nop                                                ## L:1064 |    *48 | }
TPXCmd_SetDMEM:
  andi $s7, $a0, 32768                               ## L:1069 |      ^ | u32 is16bit = addrDMEM & 0x8000;
  bne $s7, $zero, RSPQ_Loop                          ## L:1072 |      2 | if(is16bit)goto RSPQ_Loop;
  sh $a1, ($a0)                                      ## L:1070 |     *4 | store(value:u16, addrDMEM);
  j RSPQ_Loop                                        ## L:1075 |      5 | }
  sw $a1, ($a0)                                      ## L:1074 |     *7 | store(value:u32, addrDMEM);
TPXCmd_DrawTextured:
  ori $s4, $zero, %lo(PARTICLE_BUFF)                 ## L:1080 |      ^ | u16<$s4> dmaDmem = PARTICLE_BUFF;
  andi $t0, $a0, 65535                               ## L:1081 |      2 | u32<$t0> dmaSize = dataSize & 0xFFFF;
  addu $s7, $s4, $t0                                 ## L:1096 |      3 | u32 ptrInEnd = dmaDmem + dmaSize;
  addiu $t0, $t0, -1                                 ## L:1097 |      4 | dma_in_async(dmaDmem, rdramAddr, dmaSize);
  or $t2, $zero, $zero                               ## L:1097 |      5 | dma_in_async(dmaDmem, rdramAddr, dmaSize);
  jal DMAExec                                        ## L:1097 |      6 | dma_in_async(dmaDmem, rdramAddr, dmaSize); ## Args: $t0, $t1, $s0, $s4, $t2
  or $s0, $zero, $a1                                 ## L:1097 |     *8 | dma_in_async(dmaDmem, rdramAddr, dmaSize);
  or $s6, $zero, $s4                                 ## L:1099 |      9 | u32 ptrIn = dmaDmem;
  ori $s4, $zero, %lo(RDP_BUFF)                      ## L:1101 |     10 | dmaDmem = RDP_BUFF;
  ori $s3, $zero, %lo(RDP_BUFF)                      ## L:1102 |     11 | u16<$s3> dmaDmemEnd = RDP_BUFF;
  addiu $s5, $s3, 480                                ## L:1103 |     12 | u16 dmaDmemFlush = dmaDmemEnd + 480;
  ori $at, $zero, %lo(MATRIX_MVP)                    ## L:1105 |     13 | vec32 mat0 = load(MATRIX_MVP, 0x00).xyzwxyzw;
  ldv $v27, 0, 0, $at                                ## L:1105 |     14 | vec32 mat0 = load(MATRIX_MVP, 0x00).xyzwxyzw;
  ldv $v27, 8, 0, $at                                ## L:1105 |     15 | vec32 mat0 = load(MATRIX_MVP, 0x00).xyzwxyzw;
  ldv $v28, 0, 8, $at                                ## L:1105 |     16 | vec32 mat0 = load(MATRIX_MVP, 0x00).xyzwxyzw;
  ldv $v28, 8, 8, $at                                ## L:1105 |     17 | vec32 mat0 = load(MATRIX_MVP, 0x00).xyzwxyzw;
  ldv $v25, 0, 16, $at                               ## L:1106 |     18 | vec32 mat1 = load(MATRIX_MVP, 0x10).xyzwxyzw;
  ldv $v25, 8, 16, $at                               ## L:1106 |     19 | vec32 mat1 = load(MATRIX_MVP, 0x10).xyzwxyzw;
  ldv $v26, 0, 24, $at                               ## L:1106 |     20 | vec32 mat1 = load(MATRIX_MVP, 0x10).xyzwxyzw;
  ldv $v26, 8, 24, $at                               ## L:1106 |     21 | vec32 mat1 = load(MATRIX_MVP, 0x10).xyzwxyzw;
  ldv $v23, 0, 32, $at                               ## L:1107 |     22 | vec32 mat2 = load(MATRIX_MVP, 0x20).xyzwxyzw;
  ldv $v23, 8, 32, $at                               ## L:1107 |     23 | vec32 mat2 = load(MATRIX_MVP, 0x20).xyzwxyzw;
  ldv $v24, 0, 40, $at                               ## L:1107 |     24 | vec32 mat2 = load(MATRIX_MVP, 0x20).xyzwxyzw;
  ldv $v24, 8, 40, $at                               ## L:1107 |     25 | vec32 mat2 = load(MATRIX_MVP, 0x20).xyzwxyzw;
  ldv $v21, 0, 48, $at                               ## L:1108 |     26 | vec32 mat3 = load(MATRIX_MVP, 0x30).xyzwxyzw;
  ldv $v21, 8, 48, $at                               ## L:1108 |     27 | vec32 mat3 = load(MATRIX_MVP, 0x30).xyzwxyzw;
  ldv $v22, 0, 56, $at                               ## L:1108 |     28 | vec32 mat3 = load(MATRIX_MVP, 0x30).xyzwxyzw;
  ldv $v22, 8, 56, $at                               ## L:1108 |     29 | vec32 mat3 = load(MATRIX_MVP, 0x30).xyzwxyzw;
  ori $at, $zero, %lo(SCREEN_SCALE_OFFSET)           ## L:1110 |     30 | vec32 screenSize:sint = load(SCREEN_SCALE_OFFSET).xyzwxyzw;
  ldv $v19, 0, 0, $at                                ## L:1110 |     31 | vec32 screenSize:sint = load(SCREEN_SCALE_OFFSET).xyzwxyzw;
  ldv $v19, 8, 0, $at                                ## L:1110 |     32 | vec32 screenSize:sint = load(SCREEN_SCALE_OFFSET).xyzwxyzw;
  vmudl $v20, $v00, $v31.e7                          ## L:1112 |      ^ | screenSize >>= 8;
  vmadm $v19, $v19, $v31.e7                          ## L:1112 |  ***36 | screenSize >>= 8;
  vmadn $v20, $v00, $v00                             ## L:1112 |     37 | screenSize >>= 8;
  ldv $v18, 0, 8, $at                                ## L:1114 |      ^ | vec16 screenOffset = load(SCREEN_SCALE_OFFSET, 0x08).xyzwxyzw;
  ldv $v18, 8, 8, $at                                ## L:1114 |     38 | vec16 screenOffset = load(SCREEN_SCALE_OFFSET, 0x08).xyzwxyzw;
  ori $at, $zero, %lo(NORM_SCALE_W)                  ## L:1115 |     39 | vec16 normScaleW = load(NORM_SCALE_W).xyzwxyzw;
  ldv $v17, 0, 0, $at                                ## L:1115 |     40 | vec16 normScaleW = load(NORM_SCALE_W).xyzwxyzw;
  ldv $v17, 8, 0, $at                                ## L:1115 |     41 | vec16 normScaleW = load(NORM_SCALE_W).xyzwxyzw;
  addiu $at, $zero, 4095                             ## L:1211 |     42 | screenMax.w = 0b1111'1111'1111;
  mtc2 $at, $v15.e3                                  ## L:1211 |    *44 | screenMax.w = 0b1111'1111'1111;
  lw $s2, %lo(RDPQ_SCISSOR_RECT + 4)                 ## L:1212 |     45 | u32 extMax = load(RDPQ_SCISSOR_RECT, 4);
  lw $s1, %lo(RDPQ_SCISSOR_RECT + 0)                 ## L:1213 |     46 | u32 extMin = load(RDPQ_SCISSOR_RECT, 0);
  srl $t4, $s2, 12                                   ## L:1215 |    *48 | temp1 = extMax >> 12;
  mtc2 $t4, $v15.e0                                  ## L:1216 |     49 | screenMax.x = temp1;  screenMax.X = temp1;
  mtc2 $t4, $v15.e4                                  ## L:1216 |     50 | screenMax.x = temp1;  screenMax.X = temp1;
  mtc2 $s2, $v15.e1                                  ## L:1217 |   **53 | screenMax.y = extMax; screenMax.Y = extMax;
  mtc2 $s2, $v15.e5                                  ## L:1217 |     54 | screenMax.y = extMax; screenMax.Y = extMax;
  srl $t4, $s1, 12                                   ## L:1219 |     55 | temp1 = extMin >> 12;
  mtc2 $t4, $v16.e0                                  ## L:1220 |    *57 | screenMin.x = temp1;  screenMin.X = temp1;
  mtc2 $t4, $v16.e4                                  ## L:1220 |     58 | screenMin.x = temp1;  screenMin.X = temp1;
  mtc2 $s1, $v16.e1                                  ## L:1221 |   **61 | screenMin.y = extMin; screenMin.Y = extMin;
  mtc2 $s1, $v16.e5                                  ## L:1221 |     62 | screenMin.y = extMin; screenMin.Y = extMin;
  vand $v15, $v15, $v15.e3                           ## L:1223 |      ^ | screenMax &= screenMax.w;
  vand $v16, $v16, $v15.e3                           ## L:1224 |  ***66 | screenMin &= screenMax.w;
  ori $at, $zero, %lo(PARTICLE_SCALE)                ## L:1122 |      ^ | globalPartSize.xy = load(PARTICLE_SCALE).xy;
  llv $v13, 0, 0, $at                                ## L:1122 |     67 | globalPartSize.xy = load(PARTICLE_SCALE).xy;
  llv $v13, 8, 0, $at                                ## L:1123 |     68 | globalPartSize.XY = load(PARTICLE_SCALE).xy;
  ori $s2, $zero, %lo(RDP_BUFF)                      ## L:1128 |     69 | u16 buffRdp = RDP_BUFF;
  addiu $s1, $s5, 32                                 ## L:1129 |     70 | u16 buffRdpEnd = dmaDmemFlush + 32;
  lui $t7, 0x3A00                                    ## L:1130 |     71 | cmdRdpColor = 0x3A00'0000;
  addiu $a0, $zero, 46                               ## L:1131 |     72 | cmdRdpDepth = 0x2E;
  LABEL_TPXCmd_DrawTextured_0013:
  sw $t7, 0($s2)                                     ## L:1133 |     73 | store(cmdRdpColor, buffRdp, 0);
  sb $a0, 8($s2)                                     ## L:1134 |     74 | store(cmdRdpDepth, buffRdp, 8);
  sh $zero, 14($s2)                                  ## L:1135 |     75 | store(ZERO:u16, buffRdp, 0x0E);
  addiu $s2, $s2, 32                                 ## L:1136 |     76 | buffRdp += 32;
  bne $s2, $s1, LABEL_TPXCmd_DrawTextured_0013       ## L:1136 |     77 | buffRdp += 32;
  nop                                                ## L:1136 |    *79 | buffRdp += 32;
  LABEL_TPXCmd_DrawTextured_0014:
  vxor $v12, $v00, $v00.e0                           ## L:1143 |     80 | vec16 texMirrorMask = 0;
  vxor $v11, $v00, $v00.e0                           ## L:1144 |     81 | vec16 texMirrorCompare = 0;
  lhu $s1, %lo(TILE_COUNT + 0)                       ## L:1147 |      ^ | u16 tiles = load(TILE_COUNT);
  addu $fp, $s1, $s1                                 ## L:1148 |   **84 | u16 tilesEnd = tiles + tiles;
  vmov $v12.e5, $v30.e7                              ## L:1150 |      ^ | texMirrorMask.Y = 1;
  vmov $v12.e1, $v30.e7                              ## L:1151 |     85 | texMirrorMask.y = 1;
  mtc2 $s1, $v12.e2                                  ## L:1152 |     86 | texMirrorMask.z = tiles;
  mtc2 $fp, $v12.e3                                  ## L:1153 |     87 | texMirrorMask.w = tilesEnd;
  addiu $fp, $fp, 65535                              ## L:1155 |     88 | tilesEnd -= 1;
  mtc2 $fp, $v12.e0                                  ## L:1156 |    *90 | texMirrorMask.x = tilesEnd;
  mtc2 $fp, $v12.e4                                  ## L:1157 |     91 | texMirrorMask.X = tilesEnd;
  vmov $v12.e7, $v31.e7                              ## L:1158 |     92 | texMirrorMask.W = 0x100;
  vmudn $v12, $v12, $v12.e7                          ## L:1159 |  ***96 | texMirrorMask = texMirrorMask * texMirrorMask.W;
  vsubc $v29, $v12, $v30.e7                          ## L:1161 | ***100 | VTEMP = texMirrorMask - 1;
  vmov $v12.e3, $v29.e3                              ## L:1162 | ***104 | texMirrorMask.w = VTEMP.w;
  vor $v11, $v00, $v29.e2                            ## L:1163 |    105 | texMirrorCompare = VTEMP.z;
  jal DMAWaitIdle                                    ## L:1166 |      ^ | dma_await();
  lh $s2, %lo(TEX_OFFSET + 0)                        ## L:1141 |   *107 | s16 texOffset = load(TEX_OFFSET);
  bgez $a1, LABEL_TPXCmd_DrawTextured_0015           ## L:1171 |    108 | if(rdramAddr < 0) {
  nop                                                ## L:1171 |   *110 | if(rdramAddr < 0) {
  lqv $v08, 0, 0, $s6                                ## L:606  |    111 | vec16 posStart = load(ptrIn, 0x00);
  vmulf $v14, $v13, $v08.h3                          ## L:607  | ***115 | localPartSize = globalPartSize:sfract * posStart:sfract.wwwwWWWW;
  vxor $v07, $v00, $v30.e7                           ## L:614  |    116 | const vec16 vecOne = 1;
  vmudn $v10, $v28, $v08.h0                          ## L:92   |    117 | out = mat0  * vec.xxxxXXXX;
  vmadh $v09, $v27, $v08.h0                          ## L:92   |    118 | out = mat0  * vec.xxxxXXXX;
  vmadn $v10, $v26, $v08.h1                          ## L:93   |    119 | out = mat1 +* vec.yyyyYYYY;
  vmadh $v09, $v25, $v08.h1                          ## L:93   |    120 | out = mat1 +* vec.yyyyYYYY;
  vmadn $v10, $v24, $v08.h2                          ## L:94   |    121 | out = mat2 +* vec.zzzzZZZZ;
  vmadh $v09, $v23, $v08.h2                          ## L:94   |    122 | out = mat2 +* vec.zzzzZZZZ;
  vmadn $v10, $v22, $v30.e7                          ## L:95   |    123 | out = mat3 +* 1;
  vmadh $v09, $v21, $v30.e7                          ## L:95   |    124 | out = mat3 +* 1;
  vch $v29, $v09, $v09.h3                            ## L:619  | ***128 | temp1 = clip(posClip, posClip.wwwwWWWW);
  vcl $v29, $v10, $v10.h3                            ## L:619  |    129 | temp1 = clip(posClip, posClip.wwwwWWWW);
  cfc2 $t4, $vcc                                     ## L:619  |    130 | temp1 = clip(posClip, posClip.wwwwWWWW);
  LABEL_TPXCmd_DrawTextured_0017:
  andi $t1, $t4, 1028                                ## L:626  |  **133 | clipA = temp1 & 0b0000'0100'0000'0100;
  andi $t5, $t4, 16448                               ## L:627  |    134 | clipB = temp1 & 0b0100'0000'0100'0000;
  vmudl $v10, $v10, $v17.v                           ## L:630  |      ^ | posClip *= normScaleW:ufract;
  vmadm $v09, $v09, $v17.v                           ## L:630  |    135 | posClip *= normScaleW:ufract;
  vmadn $v10, $v00, $v00                             ## L:630  |    136 | posClip *= normScaleW:ufract;
  vrcph $v05.e3, $v09.e3                             ## L:632  |  **139 | invW.w = invert_half(posClip).w;
  vrcpl $v06.e3, $v10.e3                             ## L:632  |    140 | invW.w = invert_half(posClip).w;
  vrcph $v05.e3, $v09.e7                             ## L:633  |    141 | invW.W = invert_half(posClip).W;
  vrcpl $v06.e7, $v10.e7                             ## L:633  |    142 | invW.W = invert_half(posClip).W;
  vrcph $v05.e7, $v00.e7                             ## L:633  |    143 | invW.W = invert_half(posClip).W;
  ori $at, $zero, %lo(BASE_SIZE)                     ## L:636  |      ^ | posClip:sint.w = load(BASE_SIZE).x;
  lsv $v09, 6, 0, $at                                ## L:636  |    144 | posClip:sint.w = load(BASE_SIZE).x;
  lsv $v09, 14, 0, $at                               ## L:637  |    145 | posClip:sint.W = load(BASE_SIZE).x;
  vmov $v10.e3, $v00.e0                              ## L:638  |      ^ | posClip:sfract.w = 0;
  vmov $v10.e7, $v00.e0                              ## L:639  |    146 | posClip:sfract.W = 0;
  vmudl $v29, $v10, $v06.h3                          ## L:642  | ***150 | posClip *= invW.wwwwWWWW;
  vmadm $v29, $v09, $v06.h3                          ## L:642  |    151 | posClip *= invW.wwwwWWWW;
  vmadn $v10, $v10, $v05.h3                          ## L:642  |    152 | posClip *= invW.wwwwWWWW;
  vmadh $v09, $v09, $v05.h3                          ## L:642  |    153 | posClip *= invW.wwwwWWWW;
  vmulf $v14, $v14, $v09.h3                          ## L:645  | ***157 | localPartSize:sfract *= posClip:sint.wwwwWWWW;
  vmudl $v29, $v10, $v20.v                           ## L:648  |    158 | vec32 posScreen = posClip * screenSize;
  vmadm $v29, $v09, $v20.v                           ## L:648  |    159 | vec32 posScreen = posClip * screenSize;
  vmadn $v04, $v10, $v19.v                           ## L:648  |    160 | vec32 posScreen = posClip * screenSize;
  vmadh $v03, $v09, $v19.v                           ## L:648  |    161 | vec32 posScreen = posClip * screenSize;
  vmadh $v02, $v18, $v07.v                           ## L:649  |    162 | vec16 posCenter = screenOffset:sint +* vecOne;
  vmadh $v04, $v07, $v14.v                           ## L:654  |    163 | vec16 posEnd = vecOne +* localPartSize:sint;
  vsubc $v08, $v02, $v14.v                           ## L:655  |  **166 | posStart = posCenter - localPartSize:sint;
  vmudn $v14, $v14, $v30.e6                          ## L:659  |    167 | localPartSize *= 2;
  vrcp $v06.e0, $v14.e0                              ## L:660  | ***171 | invW.x = invert_half(localPartSize).x;
  vrcph $v05.e0, $v14.e0                             ## L:660  |    172 | invW.x = invert_half(localPartSize).x;
  vrcp $v06.e1, $v14.e1                              ## L:661  |    173 | invW.y = invert_half(localPartSize).y;
  vrcph $v05.e1, $v14.e1                             ## L:661  |    174 | invW.y = invert_half(localPartSize).y;
  vrcp $v06.e4, $v14.e4                              ## L:662  |    175 | invW.X = invert_half(localPartSize).X;
  vrcph $v05.e4, $v14.e4                             ## L:662  |    176 | invW.X = invert_half(localPartSize).X;
  vrcp $v06.e5, $v14.e5                              ## L:663  |    177 | invW.Y = invert_half(localPartSize).Y;
  vrcph $v05.e5, $v14.e5                             ## L:663  |    178 | invW.Y = invert_half(localPartSize).Y;
  vor $v03, $v00, $v05                               ## L:664  | ***182 | vec16 uvDelta = invW:sint;
  vmudh $v01, $v08, $v05.v                           ## L:668  |    183 | vec16 uvStart = posStart * invW:sint;
  vmudm $v01, $v01, $v31.e6                          ## L:669  | ***187 | uvStart >>= 7;
  vlt $v01, $v01, $v00.e0                            ## L:670  | ***191 | uvStart = uvStart < 0;
  lb $a2, 7($s6)                                     ## L:674  |      ^ | temp0:s8 = load(ptrIn, 7);
  lb $t4, 15($s6)                                    ## L:675  |    192 | temp1:s8 = load(ptrIn, 15);
  addu $a2, $a2, $s2                                 ## L:680  |   *194 | temp0 += texOffset; temp0 <<= 3;
  sll $a2, $a2, 3                                    ## L:680  |    195 | temp0 += texOffset; temp0 <<= 3;
  addu $t4, $t4, $s2                                 ## L:681  |    196 | temp1 += texOffset; temp1 <<= 3;
  sll $t4, $t4, 3                                    ## L:681  |    197 | temp1 += texOffset; temp1 <<= 3;
  vsubc $v06, $v00, $v01.v                           ## L:692  |      ^ | vec16 uvStartNeg = VZERO - uvStart;
  vxor $v05, $v00, $v00.e0                           ## L:694  |    198 | vec16 texOffsetTotal = 0;
  mtc2 $a2, $v05.e0                                  ## L:695  |    199 | texOffsetTotal.x = temp0:s16;
  mtc2 $t4, $v05.e4                                  ## L:696  |    200 | texOffsetTotal.X = temp1:s16;
  vand $v05, $v05, $v12.e3                           ## L:697  | ***204 | texOffsetTotal &= texMirrorMask.w;
  vge $v29, $v11, $v05.h0                            ## L:700  | ***208 | uvStart = texMirrorCompare >= texOffsetTotal.xxxxXXXX ? uvStartNeg : uvStart;
  vmrg $v01, $v06, $v01                              ## L:700  |    209 | uvStart = texMirrorCompare >= texOffsetTotal.xxxxXXXX ? uvStartNeg : uvStart;
  vlt $v04, $v04, $v15                               ## L:703  |    210 | posEnd = min(posEnd, screenMax);
  vge $v08, $v08, $v16                               ## L:704  |    211 | posStart = max(posStart, screenMin);
  vaddc $v01, $v01, $v05.v                           ## L:706  |   *213 | uvStart += texOffsetTotal;
  vsubc $v06, $v12, $v01.v                           ## L:711  | ***217 | uvStartNeg = texMirrorMask - uvStart;
  vaddc $v06, $v06, $v12.e2                          ## L:712  | ***221 | uvStartNeg += texMirrorMask.z;
  vge $v29, $v11, $v05.h0                            ## L:714  |    222 | uvStart = texMirrorCompare >= texOffsetTotal.xxxxXXXX ? uvStart : uvStartNeg;
  vmrg $v01, $v01, $v06                              ## L:714  |  **225 | uvStart = texMirrorCompare >= texOffsetTotal.xxxxXXXX ? uvStart : uvStartNeg;
  mfc2 $s1, $v08.e1                                  ## L:1190 |      ^ | outA = pos.y;
  andi $s1, $s1, 4095                                ## L:1191 |  **228 | outA &= 0b1111'1111'1111;
  mfc2 $k0, $v08.e0                                  ## L:1192 |    229 | u32 tmp = pos.x;
  sll $k0, $k0, 12                                   ## L:1193 |  **232 | tmp <<= 12;
  or $s1, $s1, $k0                                   ## L:1194 |    233 | outA |= tmp;
  mfc2 $fp, $v08.e5                                  ## L:1196 |    234 | outB = pos.Y;
  andi $fp, $fp, 4095                                ## L:1197 |  **237 | outB &= 0b1111'1111'1111;
  mfc2 $k0, $v08.e4                                  ## L:1198 |    238 | tmp = pos.X;
  sll $k0, $k0, 12                                   ## L:1199 |  **241 | tmp <<= 12;
  or $fp, $fp, $k0                                   ## L:1200 |    242 | outB |= tmp;
  mfc2 $sp, $v04.e1                                  ## L:1190 |    243 | outA = pos.y;
  andi $sp, $sp, 4095                                ## L:1191 |  **246 | outA &= 0b1111'1111'1111;
  mfc2 $k0, $v04.e0                                  ## L:1192 |    247 | u32 tmp = pos.x;
  sll $k0, $k0, 12                                   ## L:1193 |  **250 | tmp <<= 12;
  or $sp, $sp, $k0                                   ## L:1194 |    251 | outA |= tmp;
  mfc2 $k1, $v04.e5                                  ## L:1196 |    252 | outB = pos.Y;
  andi $k1, $k1, 4095                                ## L:1197 |  **255 | outB &= 0b1111'1111'1111;
  mfc2 $k0, $v04.e4                                  ## L:1198 |    256 | tmp = pos.X;
  sll $k0, $k0, 12                                   ## L:1199 |  **259 | tmp <<= 12;
  vlt $v06, $v08, $v04                               ## L:721  |      ^ | vec16 extend = posStart < posEnd;
  cfc2 $t4, $vcc                                     ## L:722  |    260 | temp1 = get_vcc();
  andi $a2, $t4, 3                                   ## L:724  |  **263 | temp0 = temp1 & 0b0000'0011;
  or $t1, $t1, $a2                                   ## L:725  |    264 | clipA |= temp0;
  andi $a2, $t4, 48                                  ## L:727  |    265 | temp0 = temp1 & 0b0011'0000;
  or $t5, $t5, $a2                                   ## L:728  |    266 | clipB |= temp0;
  ldv $v05, 0, 16, $s6                               ## L:732  |    267 | vec16 color = load(ptrIn, 16).xyzw;
  addiu $t2, $zero, 36                               ## L:737  |    268 | cmdRdpRect = 0x24;
  slv $v05, 0, 4, $s3                                ## L:745  |  **271 | store(color.xy, dmaDmemEnd, 4);
  ssv $v02, 4, 12, $s3                               ## L:749  |    272 | store(posCenter.z, dmaDmemEnd, 0x04, 8);
  sw $s1, 4 + 16($s3)                                ## L:754  |    273 | store(posA, dmaDmemEnd, 0x04, 16);
  sw $sp, 0 + 16($s3)                                ## L:755  |    274 | @Barrier("pos-cmd") store(posEndA, dmaDmemEnd, 0x00, 16); ## Barrier: 0x1
  sb $t2, 0 + 16($s3)                                ## L:756  |    275 | @Barrier("pos-cmd") store(cmdRdpRect, dmaDmemEnd, 0x00, 16); ## Barrier: 0x1
  slv $v01, 0, 24, $s3                               ## L:757  |    276 | store(uvStart.xy, dmaDmemEnd, 0x08, 16);
  slv $v03, 0, 28, $s3                               ## L:758  |    277 | store(uvDelta.xy, dmaDmemEnd, 0x0C, 16);
  ldv $v08, 0, 24, $s6                               ## L:762  |    278 | posStart.xyzw = load(ptrIn, 24).xyzw;
  ldv $v08, 8, 32, $s6                               ## L:763  |    279 | posStart.XYZW = load(ptrIn, 24).XYZW;
  vmulf $v14, $v13, $v08.h3                          ## L:764  | ***283 | localPartSize = globalPartSize:sfract * posStart:sfract.wwwwWWWW;
  addiu $at, $zero, 3                                ## L:771  |      ^ | if(clipA == 0b0000'0011) {
  bne $t1, $at, LABEL_TPXCmd_DrawTextured_0019       ## L:771  |    284 | if(clipA == 0b0000'0011) {
  or $k1, $k1, $k0                                   ## L:1200 |   *286 | outB |= tmp;
  addiu $s3, $s3, 32                                 ## L:772  |    287 | dmaDmemEnd += 32;
  LABEL_TPXCmd_DrawTextured_0019:
  addiu $at, $zero, 48                               ## L:782  |    288 | if(clipB == 0b0011'0000) {
  bne $t5, $at, LABEL_TPXCmd_DrawTextured_001A       ## L:782  |    289 | if(clipB == 0b0011'0000) {
  addiu $s6, $s6, 24                                 ## L:776  |   *291 | ptrIn += 24;
  slv $v05, 4, 4, $s3                                ## L:783  |    292 | store(color.zw, dmaDmemEnd, 4);
  ssv $v02, 12, 12, $s3                              ## L:784  |    293 | store(posCenter.Z, dmaDmemEnd, 0x04, 8);
  sw $fp, 4 + 16($s3)                                ## L:786  |    294 | store(posB, dmaDmemEnd, 0x04, 16);
  sw $k1, 0 + 16($s3)                                ## L:787  |    295 | @Barrier("pos-cmd") store(posEndB, dmaDmemEnd, 0x00, 16); ## Barrier: 0x1
  sb $t2, 0 + 16($s3)                                ## L:788  |    296 | @Barrier("pos-cmd") store(cmdRdpRect, dmaDmemEnd, 0x00, 16); ## Barrier: 0x1
  slv $v01, 8, 24, $s3                               ## L:790  |    297 | store(uvStart.XY, dmaDmemEnd, 0x08, 16);
  slv $v03, 8, 28, $s3                               ## L:791  |    298 | store(uvDelta.XY, dmaDmemEnd, 0x0C, 16);
  addiu $s3, $s3, 32                                 ## L:793  |    299 | dmaDmemEnd += 32;
  LABEL_TPXCmd_DrawTextured_001A:
  vmudn $v10, $v28, $v08.h0                          ## L:92   |      ^ | out = mat0  * vec.xxxxXXXX;
  vmadh $v09, $v27, $v08.h0                          ## L:92   |    300 | out = mat0  * vec.xxxxXXXX;
  vmadn $v10, $v26, $v08.h1                          ## L:93   |    301 | out = mat1 +* vec.yyyyYYYY;
  vmadh $v09, $v25, $v08.h1                          ## L:93   |    302 | out = mat1 +* vec.yyyyYYYY;
  vmadn $v10, $v24, $v08.h2                          ## L:94   |    303 | out = mat2 +* vec.zzzzZZZZ;
  vmadh $v09, $v23, $v08.h2                          ## L:94   |    304 | out = mat2 +* vec.zzzzZZZZ;
  vmadn $v10, $v22, $v30.e7                          ## L:95   |    305 | out = mat3 +* 1;
  sltu $at, $s3, $s5                                 ## L:800  |      ^ | if(dmaDmemEnd >= dmaDmemFlush) {
  bne $at, $zero, LABEL_TPXCmd_DrawTextured_001B     ## L:800  |    306 | if(dmaDmemEnd >= dmaDmemFlush) {
  vmadh $v09, $v21, $v30.e7                          ## L:95   |   *308 | out = mat3 +* 1;
  jal RDPQ_Send                                      ## L:801  |    309 | RDPQ_Send(dmaDmem, dmaDmemEnd); ## Args: $s4, $s3
  nop                                                ## L:801  |   *311 | RDPQ_Send(dmaDmem, dmaDmemEnd);
  or $s3, $zero, $s4                                 ## L:802  |    312 | dmaDmemEnd = dmaDmem;
  LABEL_TPXCmd_DrawTextured_001B:
  vch $v29, $v09, $v09.h3                            ## L:805  |      ^ | temp1 = clip(posClip, posClip.wwwwWWWW);
  vcl $v29, $v10, $v10.h3                            ## L:805  |    313 | temp1 = clip(posClip, posClip.wwwwWWWW);
  bne $s6, $s7, LABEL_TPXCmd_DrawTextured_0017       ## L:805  |      ^ | temp1 = clip(posClip, posClip.wwwwWWWW);
  cfc2 $t4, $vcc                                     ## L:805  |   *315 | temp1 = clip(posClip, posClip.wwwwWWWW);
  LABEL_TPXCmd_DrawTextured_0018:
  beq $zero, $zero, LABEL_TPXCmd_DrawTextured_0016   ## L:805  |    316 | temp1 = clip(posClip, posClip.wwwwWWWW);
  nop                                                ## L:805  |   *318 | temp1 = clip(posClip, posClip.wwwwWWWW);
  LABEL_TPXCmd_DrawTextured_0015:
  lpv $v08, 0, 0, $s6                                ## L:256  |    319 | vec16 posStart = load_vec_s8(ptrIn, 0x00);
  vmulf $v14, $v13, $v08.h3                          ## L:257  | ***323 | localPartSize = globalPartSize:sfract * posStart:sfract.wwwwWWWW;
  vmudm $v08, $v08, $v31.e7                          ## L:258  |    324 | posStart >>= 8;
  vxor $v07, $v00, $v30.e7                           ## L:261  |    325 | const vec16 vecOne = 1;
  vmudn $v10, $v28, $v08.h0                          ## L:92   |  **328 | out = mat0  * vec.xxxxXXXX;
  vmadh $v09, $v27, $v08.h0                          ## L:92   |    329 | out = mat0  * vec.xxxxXXXX;
  vmadn $v10, $v26, $v08.h1                          ## L:93   |    330 | out = mat1 +* vec.yyyyYYYY;
  vmadh $v09, $v25, $v08.h1                          ## L:93   |    331 | out = mat1 +* vec.yyyyYYYY;
  vmadn $v10, $v24, $v08.h2                          ## L:94   |    332 | out = mat2 +* vec.zzzzZZZZ;
  vmadh $v09, $v23, $v08.h2                          ## L:94   |    333 | out = mat2 +* vec.zzzzZZZZ;
  vmadn $v10, $v22, $v30.e7                          ## L:95   |    334 | out = mat3 +* 1;
  vmadh $v09, $v21, $v30.e7                          ## L:95   |    335 | out = mat3 +* 1;
  vch $v29, $v09, $v09.h3                            ## L:266  | ***339 | temp1 = clip(posClip, posClip.wwwwWWWW);
  vcl $v29, $v10, $v10.h3                            ## L:266  |    340 | temp1 = clip(posClip, posClip.wwwwWWWW);
  cfc2 $t4, $vcc                                     ## L:266  |    341 | temp1 = clip(posClip, posClip.wwwwWWWW);
  LABEL_TPXCmd_DrawTextured_001C:
  andi $t1, $t4, 1028                                ## L:273  |  **344 | clipA = temp1 & 0b0000'0100'0000'0100;
  andi $t5, $t4, 16448                               ## L:274  |    345 | clipB = temp1 & 0b0100'0000'0100'0000;
  vmudl $v10, $v10, $v17.v                           ## L:277  |      ^ | posClip *= normScaleW:ufract;
  vmadm $v09, $v09, $v17.v                           ## L:277  |    346 | posClip *= normScaleW:ufract;
  vmadn $v10, $v00, $v00                             ## L:277  |    347 | posClip *= normScaleW:ufract;
  vrcph $v05.e3, $v09.e3                             ## L:279  |  **350 | invW.w = invert_half(posClip).w;
  vrcpl $v06.e3, $v10.e3                             ## L:279  |    351 | invW.w = invert_half(posClip).w;
  vrcph $v05.e3, $v09.e7                             ## L:280  |    352 | invW.W = invert_half(posClip).W;
  vrcpl $v06.e7, $v10.e7                             ## L:280  |    353 | invW.W = invert_half(posClip).W;
  vrcph $v05.e7, $v00.e7                             ## L:280  |    354 | invW.W = invert_half(posClip).W;
  ori $at, $zero, %lo(BASE_SIZE)                     ## L:283  |      ^ | posClip:sint.w = load(BASE_SIZE).x;
  lsv $v09, 6, 0, $at                                ## L:283  |    355 | posClip:sint.w = load(BASE_SIZE).x;
  lsv $v09, 14, 0, $at                               ## L:284  |    356 | posClip:sint.W = load(BASE_SIZE).x;
  vmov $v10.e3, $v00.e0                              ## L:285  |      ^ | posClip:sfract.w = 0;
  vmov $v10.e7, $v00.e0                              ## L:286  |    357 | posClip:sfract.W = 0;
  vmudl $v29, $v10, $v06.h3                          ## L:289  | ***361 | posClip *= invW.wwwwWWWW;
  vmadm $v29, $v09, $v06.h3                          ## L:289  |    362 | posClip *= invW.wwwwWWWW;
  vmadn $v10, $v10, $v05.h3                          ## L:289  |    363 | posClip *= invW.wwwwWWWW;
  vmadh $v09, $v09, $v05.h3                          ## L:289  |    364 | posClip *= invW.wwwwWWWW;
  vmulf $v14, $v14, $v09.h3                          ## L:292  | ***368 | localPartSize:sfract *= posClip:sint.wwwwWWWW;
  vmudl $v29, $v10, $v20.v                           ## L:295  |    369 | vec32 posScreen = posClip * screenSize;
  vmadm $v29, $v09, $v20.v                           ## L:295  |    370 | vec32 posScreen = posClip * screenSize;
  vmadn $v04, $v10, $v19.v                           ## L:295  |    371 | vec32 posScreen = posClip * screenSize;
  vmadh $v03, $v09, $v19.v                           ## L:295  |    372 | vec32 posScreen = posClip * screenSize;
  vmadh $v02, $v18, $v07.v                           ## L:296  |    373 | vec16 posCenter = screenOffset:sint +* vecOne;
  vmadh $v04, $v07, $v14.v                           ## L:301  |    374 | vec16 posEnd = vecOne +* localPartSize:sint;
  vsubc $v08, $v02, $v14.v                           ## L:302  |  **377 | posStart = posCenter - localPartSize:sint;
  vmudn $v14, $v14, $v30.e6                          ## L:306  |    378 | localPartSize *= 2;
  vrcp $v06.e0, $v14.e0                              ## L:307  | ***382 | invW.x = invert_half(localPartSize).x;
  vrcph $v05.e0, $v14.e0                             ## L:307  |    383 | invW.x = invert_half(localPartSize).x;
  vrcp $v06.e1, $v14.e1                              ## L:308  |    384 | invW.y = invert_half(localPartSize).y;
  vrcph $v05.e1, $v14.e1                             ## L:308  |    385 | invW.y = invert_half(localPartSize).y;
  vrcp $v06.e4, $v14.e4                              ## L:309  |    386 | invW.X = invert_half(localPartSize).X;
  vrcph $v05.e4, $v14.e4                             ## L:309  |    387 | invW.X = invert_half(localPartSize).X;
  vrcp $v06.e5, $v14.e5                              ## L:310  |    388 | invW.Y = invert_half(localPartSize).Y;
  vrcph $v05.e5, $v14.e5                             ## L:310  |    389 | invW.Y = invert_half(localPartSize).Y;
  vor $v03, $v00, $v05                               ## L:311  | ***393 | vec16 uvDelta = invW:sint;
  vmudh $v01, $v08, $v05.v                           ## L:315  |    394 | vec16 uvStart = posStart * invW:sint;
  vmudm $v01, $v01, $v31.e6                          ## L:316  | ***398 | uvStart >>= 7;
  vlt $v01, $v01, $v00.e0                            ## L:317  | ***402 | uvStart = uvStart < 0;
  lb $a2, 11($s6)                                    ## L:324  |      ^ | temp0:s8 = load(ptrIn, 0x0B);
  lb $t4, 15($s6)                                    ## L:325  |    403 | temp1:s8 = load(ptrIn, 0x0F);
  addu $a2, $a2, $s2                                 ## L:327  |   *405 | temp0 += texOffset; temp0 <<= 3;
  sll $a2, $a2, 3                                    ## L:327  |    406 | temp0 += texOffset; temp0 <<= 3;
  addu $t4, $t4, $s2                                 ## L:328  |    407 | temp1 += texOffset; temp1 <<= 3;
  sll $t4, $t4, 3                                    ## L:328  |    408 | temp1 += texOffset; temp1 <<= 3;
  vsubc $v06, $v00, $v01.v                           ## L:339  |      ^ | vec16 uvStartNeg = VZERO - uvStart;
  vxor $v05, $v00, $v00.e0                           ## L:341  |    409 | vec16 texOffsetTotal = 0;
  mtc2 $a2, $v05.e0                                  ## L:342  |    410 | texOffsetTotal.x = temp0:s16;
  mtc2 $t4, $v05.e4                                  ## L:343  |    411 | texOffsetTotal.X = temp1:s16;
  vand $v05, $v05, $v12.e3                           ## L:344  | ***415 | texOffsetTotal &= texMirrorMask.w;
  vge $v29, $v11, $v05.h0                            ## L:347  | ***419 | uvStart = texMirrorCompare >= texOffsetTotal.xxxxXXXX ? uvStartNeg : uvStart;
  vmrg $v01, $v06, $v01                              ## L:347  |    420 | uvStart = texMirrorCompare >= texOffsetTotal.xxxxXXXX ? uvStartNeg : uvStart;
  vlt $v04, $v04, $v15                               ## L:350  |    421 | posEnd = min(posEnd, screenMax);
  vge $v08, $v08, $v16                               ## L:351  |    422 | posStart = max(posStart, screenMin);
  vaddc $v01, $v01, $v05.v                           ## L:353  |   *424 | uvStart += texOffsetTotal;
  vsubc $v06, $v12, $v01.v                           ## L:358  | ***428 | uvStartNeg = texMirrorMask - uvStart;
  vaddc $v06, $v06, $v12.e2                          ## L:359  | ***432 | uvStartNeg += texMirrorMask.z;
  vge $v29, $v11, $v05.h0                            ## L:361  |    433 | uvStart = texMirrorCompare >= texOffsetTotal.xxxxXXXX ? uvStart : uvStartNeg;
  vmrg $v01, $v01, $v06                              ## L:361  |  **436 | uvStart = texMirrorCompare >= texOffsetTotal.xxxxXXXX ? uvStart : uvStartNeg;
  mfc2 $s1, $v08.e1                                  ## L:1190 |      ^ | outA = pos.y;
  andi $s1, $s1, 4095                                ## L:1191 |  **439 | outA &= 0b1111'1111'1111;
  mfc2 $k0, $v08.e0                                  ## L:1192 |    440 | u32 tmp = pos.x;
  sll $k0, $k0, 12                                   ## L:1193 |  **443 | tmp <<= 12;
  or $s1, $s1, $k0                                   ## L:1194 |    444 | outA |= tmp;
  mfc2 $fp, $v08.e5                                  ## L:1196 |    445 | outB = pos.Y;
  andi $fp, $fp, 4095                                ## L:1197 |  **448 | outB &= 0b1111'1111'1111;
  mfc2 $k0, $v08.e4                                  ## L:1198 |    449 | tmp = pos.X;
  sll $k0, $k0, 12                                   ## L:1199 |  **452 | tmp <<= 12;
  or $fp, $fp, $k0                                   ## L:1200 |    453 | outB |= tmp;
  mfc2 $sp, $v04.e1                                  ## L:1190 |    454 | outA = pos.y;
  andi $sp, $sp, 4095                                ## L:1191 |  **457 | outA &= 0b1111'1111'1111;
  mfc2 $k0, $v04.e0                                  ## L:1192 |    458 | u32 tmp = pos.x;
  sll $k0, $k0, 12                                   ## L:1193 |  **461 | tmp <<= 12;
  or $sp, $sp, $k0                                   ## L:1194 |    462 | outA |= tmp;
  mfc2 $k1, $v04.e5                                  ## L:1196 |    463 | outB = pos.Y;
  andi $k1, $k1, 4095                                ## L:1197 |  **466 | outB &= 0b1111'1111'1111;
  mfc2 $k0, $v04.e4                                  ## L:1198 |    467 | tmp = pos.X;
  sll $k0, $k0, 12                                   ## L:1199 |  **470 | tmp <<= 12;
  vlt $v06, $v08, $v04                               ## L:368  |      ^ | vec16 extend = posStart < posEnd;
  cfc2 $t4, $vcc                                     ## L:369  |    471 | temp1 = get_vcc();
  andi $a2, $t4, 3                                   ## L:371  |  **474 | temp0 = temp1 & 0b0000'0011;
  or $t1, $t1, $a2                                   ## L:372  |    475 | clipA |= temp0;
  andi $a2, $t4, 48                                  ## L:374  |    476 | temp0 = temp1 & 0b0011'0000;
  or $t5, $t5, $a2                                   ## L:375  |    477 | clipB |= temp0;
  ldv $v05, 0, 8, $s6                                ## L:381  |    478 | vec16 color = load(ptrIn, 8).xyzw;
  addiu $t2, $zero, 36                               ## L:384  |    479 | cmdRdpRect = 0x24;
  slv $v05, 0, 4, $s3                                ## L:392  |  **482 | store(color.xy, dmaDmemEnd, 4);
  ssv $v02, 4, 12, $s3                               ## L:396  |    483 | store(posCenter.z, dmaDmemEnd, 0x04, 8);
  sw $s1, 4 + 16($s3)                                ## L:401  |    484 | store(posA, dmaDmemEnd, 0x04, 16);
  sw $sp, 0 + 16($s3)                                ## L:402  |    485 | @Barrier("pos-cmd") store(posEndA, dmaDmemEnd, 0x00, 16); ## Barrier: 0x1
  sb $t2, 0 + 16($s3)                                ## L:403  |    486 | @Barrier("pos-cmd") store(cmdRdpRect, dmaDmemEnd, 0x00, 16); ## Barrier: 0x1
  slv $v01, 0, 24, $s3                               ## L:404  |    487 | store(uvStart.xy, dmaDmemEnd, 0x08, 16);
  slv $v03, 0, 28, $s3                               ## L:405  |    488 | store(uvDelta.xy, dmaDmemEnd, 0x0C, 16);
  lpv $v08, 0, 16, $s6                               ## L:413  |    489 | posStart = load_vec_s8(ptrIn, 16);
  vmulf $v14, $v13, $v08.h3                          ## L:414  | ***493 | localPartSize = globalPartSize:sfract * posStart:sfract.wwwwWWWW;
  vmudm $v08, $v08, $v31.e7                          ## L:415  |    494 | posStart >>= 8;
  addiu $at, $zero, 3                                ## L:418  |      ^ | if(clipA == 0b0000'0011) {
  bne $t1, $at, LABEL_TPXCmd_DrawTextured_001E       ## L:418  |    495 | if(clipA == 0b0000'0011) {
  or $k1, $k1, $k0                                   ## L:1200 |   *497 | outB |= tmp;
  addiu $s3, $s3, 32                                 ## L:419  |    498 | dmaDmemEnd += 32;
  LABEL_TPXCmd_DrawTextured_001E:
  addiu $at, $zero, 48                               ## L:429  |    499 | if(clipB == 0b0011'0000) {
  bne $t5, $at, LABEL_TPXCmd_DrawTextured_001F       ## L:429  |    500 | if(clipB == 0b0011'0000) {
  addiu $s6, $s6, 16                                 ## L:425  |   *502 | ptrIn += 16;
  slv $v05, 4, 4, $s3                                ## L:430  |    503 | store(color.zw, dmaDmemEnd, 4);
  ssv $v02, 12, 12, $s3                              ## L:431  |    504 | store(posCenter.Z, dmaDmemEnd, 0x04, 8);
  sw $fp, 4 + 16($s3)                                ## L:433  |    505 | store(posB, dmaDmemEnd, 0x04, 16);
  sw $k1, 0 + 16($s3)                                ## L:434  |    506 | @Barrier("pos-cmd") store(posEndB, dmaDmemEnd, 0x00, 16); ## Barrier: 0x1
  sb $t2, 0 + 16($s3)                                ## L:435  |    507 | @Barrier("pos-cmd") store(cmdRdpRect, dmaDmemEnd, 0x00, 16); ## Barrier: 0x1
  slv $v01, 8, 24, $s3                               ## L:437  |    508 | store(uvStart.XY, dmaDmemEnd, 0x08, 16);
  slv $v03, 8, 28, $s3                               ## L:438  |    509 | store(uvDelta.XY, dmaDmemEnd, 0x0C, 16);
  addiu $s3, $s3, 32                                 ## L:440  |    510 | dmaDmemEnd += 32;
  LABEL_TPXCmd_DrawTextured_001F:
  vmudn $v10, $v28, $v08.h0                          ## L:92   |      ^ | out = mat0  * vec.xxxxXXXX;
  vmadh $v09, $v27, $v08.h0                          ## L:92   |    511 | out = mat0  * vec.xxxxXXXX;
  vmadn $v10, $v26, $v08.h1                          ## L:93   |    512 | out = mat1 +* vec.yyyyYYYY;
  vmadh $v09, $v25, $v08.h1                          ## L:93   |    513 | out = mat1 +* vec.yyyyYYYY;
  vmadn $v10, $v24, $v08.h2                          ## L:94   |    514 | out = mat2 +* vec.zzzzZZZZ;
  vmadh $v09, $v23, $v08.h2                          ## L:94   |    515 | out = mat2 +* vec.zzzzZZZZ;
  vmadn $v10, $v22, $v30.e7                          ## L:95   |    516 | out = mat3 +* 1;
  sltu $at, $s3, $s5                                 ## L:447  |      ^ | if(dmaDmemEnd >= dmaDmemFlush) {
  bne $at, $zero, LABEL_TPXCmd_DrawTextured_0020     ## L:447  |    517 | if(dmaDmemEnd >= dmaDmemFlush) {
  vmadh $v09, $v21, $v30.e7                          ## L:95   |   *519 | out = mat3 +* 1;
  jal RDPQ_Send                                      ## L:448  |    520 | RDPQ_Send(dmaDmem, dmaDmemEnd); ## Args: $s4, $s3
  nop                                                ## L:448  |   *522 | RDPQ_Send(dmaDmem, dmaDmemEnd);
  or $s3, $zero, $s4                                 ## L:449  |    523 | dmaDmemEnd = dmaDmem;
  LABEL_TPXCmd_DrawTextured_0020:
  vch $v29, $v09, $v09.h3                            ## L:452  |      ^ | temp1 = clip(posClip, posClip.wwwwWWWW);
  vcl $v29, $v10, $v10.h3                            ## L:452  |    524 | temp1 = clip(posClip, posClip.wwwwWWWW);
  bne $s6, $s7, LABEL_TPXCmd_DrawTextured_001C       ## L:452  |      ^ | temp1 = clip(posClip, posClip.wwwwWWWW);
  cfc2 $t4, $vcc                                     ## L:452  |   *526 | temp1 = clip(posClip, posClip.wwwwWWWW);
  LABEL_TPXCmd_DrawTextured_0016:
  j RDPQ_Send                                        ## L:1179 |    527 | goto RDPQ_Send;
  ori $ra, $zero, %lo(RSPQ_Loop)                     ## L:1178 |   *529 | RA = RSPQ_Loop;

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