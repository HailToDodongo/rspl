## Auto-generated file, transpiled with RSPL
#define VERT_BUFFER_COUNT 70
#define LIGHT_COUNT 7
#define RSPQ_PROFILE_MANUAL_SLOT 1
#define RDPQ_TRIANGLE_CUSTOM_VTX 1
#define VTX_ATTR_XY 0x00
#define VTX_ATTR_Z 0x04
#define VTX_ATTR_CLIPFLAGS 0x06
#define VTX_ATTR_REJFLAGS 0x07
#define VTX_ATTR_RGBA 0x08
#define VTX_ATTR_ST 0x0C
#define VTX_ATTR_CLIPPOSi 0x10
#define VTX_ATTR_Wi 0x16
#define VTX_ATTR_CLIPPOSf 0x18
#define VTX_ATTR_Wf 0x1E
#define VTX_ATTR_INVWi 0x20
#define VTX_ATTR_INVWf 0x22
#define RDPQ_TRIANGLE_VTX1 a0
#define RDPQ_TRIANGLE_VTX2 a1
#define RDPQ_TRIANGLE_VTX3 a2
#define RDPQ_TRIANGLE_VTX1_DMEM 0,v1
#define RDPQ_TRIANGLE_VTX2_DMEM 4,v1
#define RDPQ_TRIANGLE_VTX3_DMEM 2,v1
#define TRI_SIZE 36
#define TRI_SIZE_2 72
#define TRI_SIZE_3 108
#define RDP_CMD_MAX_SIZE 176
#define LIGHT_SIZE 16
#define DRAW_TRIS 1
#define DP_STATUS_END_VALID 512
#define RDPQ_DYNAMIC_BUFFER_SIZE 65536
#define HAS_NORMAL_MATRIX 1
#define MATRIX_TEMP_MUL CLIP_BUFFER_RESULT
#define MATRIX_SIZE 0x40
#include <rsp_queue.inc>
#include <rdpq_macros.h>
#include "./rspq_triangle.inc"

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
    RSPQ_DefineCommand T3DCmd_TriDraw, 8
    RSPQ_DefineCommand T3DCmd_SetScreenSize, 16
    RSPQ_DefineCommand T3DCmd_MatrixStack, 8
    RSPQ_DefineCommand T3DCmd_SetWord, 8
    RSPQ_DefineCommand T3DCmd_VertLoad, 12
    RSPQ_DefineCommand T3DCmd_LightSet, 16
    RSPQ_DefineCommand T3DCmd_RenderMode, 8
    RSPQ_DefineCommand T3DCmd_MatProjectionSet, 4
    RSPQ_DefineCommand RSPQ_Loop, 4
    RSPQ_DefineCommand T3DCmd_SetFogState, 4
    RSPQ_DefineCommand T3DCmd_TriSync, 4
    RSPQ_DefineCommand T3DCmd_TriDraw_Strip, 8
    RSPQ_DefineCommand T3DCmd_TriDraw_Seq, 8
    RSPQ_DefineCommand T3DCmd_Patch, 12
  RSPQ_EndOverlayHeader

  RSPQ_BeginSavedState
    STATE_MEM_START:
    .align 3
    MATRIX_PROJ: .ds.b 64
    .align 3
    MATRIX_MVP: .ds.b 64
    .align 3
    MATRIX_MV: .ds.b 64
    .align 3
    MATRIX_NORMAL: .ds.b 24
    .align 1
    FOG_SCALE_OFFSET: .half 0, 0, 32767, 0
    .align 4
    SCREEN_SCALE_OFFSET: .half 0, 0, 0, 0, 0, 0, 16383, 255
    .align 4
    NORMAL_MASK_SHIFT: .half 63488, 2016, 31, 0, 1, 32, 2048, 0
    .align 3
    CLIPPING_PLANES: .byte 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, -1, 0, 1, 0, -1
    .align 1
    RDPQ_TRI_BUFF_OFFSET: .half 0
    .align 1
    SCREEN_UVGEN_SCALE: .half 0
    .align 3
    NORM_SCALE_W: .half 65535, 65535, 65535, 0
    .align 1
    CLIP_RA: .half 0
    .align 1
    UV_GEN_PARAMS: .half 0, 0
    .align 1
    COLOR_EXPOSURE: .half 128
    .align 2
    SEGMENT_TABLE: .word 0, 0, 0, 0, 0, 0, 0, 0
    .align 2
    COLOR_AMBIENT: .ds.b 8
    .align 2
    LIGHT_DIR_COLOR: .ds.b 112
    .align 2
    TRI_COMMAND: .word 0
    .align 2
    MATRIX_STACK_PTR: .word 0
    .align 1
    FOG_STORE_OFFSET: .half 72
    FACE_CULLING: .byte 0
    ACTIVE_LIGHT_SIZE: .byte 0
    .align 1
    VERTEX_FX_FUNC: .half 0
    .align 1
    CLIP_CODE_SIZE: .half 0
    .align 2
    CLIP_CODE_ADDR: .word 0
    .align 2
    CLIP_CODE_ORG_ADDR: .word 0
    .align 4
    VERT_BUFFER: .ds.b 2520
    STATE_MEM_END:
  RSPQ_EndSavedState

.bss
  TEMP_STATE_MEM_START:
    .align 4
    CLIP_BUFFER_TMP: .ds.b 252
    .align 4
    CLIP_BUFFER_RESULT: .ds.b 288
  TEMP_STATE_MEM_END:

.text
OVERLAY_CODE_START:

.align 3
CLIPPING_CODE_TARGET:
  lhu $t0, %lo(CLIP_CODE_SIZE + 0)                   ## L:306  |      ^ | u16<$t0> codeSize = load(CLIP_CODE_SIZE);
  sb $t0, %lo(CLIP_CODE_ADDR)($zero)                 ## L:309  |    **4 | store(codeSize:u8, CLIP_CODE_ADDR);
  addiu $t0, $t0, -1                                 ## L:313  |      5 | dma_in(targetImem, codeAddr, codeSize);
  addiu $t2, $zero, 12                               ## L:313  |      6 | dma_in(targetImem, codeAddr, codeSize);
  jal DMAExec                                        ## L:313  |      7 | dma_in(targetImem, codeAddr, codeSize); ## Args: $t0, $t1, $s0, $s4, $t2
  lw $s0, %lo(CLIP_CODE_ADDR + 0)                    ## L:305  |     *9 | u32<$s0> codeAddr = load(CLIP_CODE_ADDR);
  j CLIPPING_CODE_TARGET                             ## L:314  |     10 | goto CLIPPING_CODE_TARGET;
  nop                                                ## L:314  |    *12 | goto CLIPPING_CODE_TARGET;
T3DCmd_VertLoad:
  srl $s4, $a2, 16                                   ## L:332  |      ^ | u32<$s4> prt3d = addressInOut >> 16;
  andi $t0, $a0, 65535                               ## L:334  |      2 | u32<$t0> copySize = bufferSize & 0xFFFF;
  addu $s7, $s4, $t0                                 ## L:335  |      3 | u32 ptr3dEnd = prt3d + copySize;
  srl $s6, $a1, 24                                   ## L:279  |      4 | u32 segment = addrIn >> 24;
  lw $s6, %lo(SEGMENT_TABLE)($s6)                    ## L:280  |      5 | segment = load(segment, SEGMENT_TABLE);
  addiu $t0, $t0, -1                                 ## L:339  |      6 | dma_in_async(prt3d, dmaAddrRDRAM, copySize);
  or $t2, $zero, $zero                               ## L:339  |      7 | dma_in_async(prt3d, dmaAddrRDRAM, copySize);
  jal DMAExec                                        ## L:339  |      8 | dma_in_async(prt3d, dmaAddrRDRAM, copySize); ## Args: $t0, $t1, $s0, $s4, $t2
  addu $s0, $a1, $s6                                 ## L:281  |    *10 | addrOut = addrIn + segment;
  ori $s6, $zero, %lo(MATRIX_NORMAL)                 ## L:271  |     11 | u16 address = MATRIX_NORMAL;
  ldv $v20, 0, 0, $s6                                ## L:272  |     12 | mat0 = load(address, 0x00).xyzwxyzw;
  ldv $v19, 0, 8, $s6                                ## L:273  |     13 | mat1 = load(address, 0x08).xyzwxyzw;
  ldv $v19, 8, 8, $s6                                ## L:273  |     14 | mat1 = load(address, 0x08).xyzwxyzw;
  ldv $v18, 0, 16, $s6                               ## L:274  |     15 | mat2 = load(address, 0x10).xyzwxyzw;
  ldv $v18, 8, 16, $s6                               ## L:274  |     16 | mat2 = load(address, 0x10).xyzwxyzw;
  jal loadCurrentMVPMat                              ## L:346  |     17 | loadCurrentMVPMat(mat0, mat1, mat2, mat3); ## Args: $v27, $v25, $v23, $v21
  ldv $v20, 8, 0, $s6                                ## L:272  |    *19 | mat0 = load(address, 0x00).xyzwxyzw;
  ori $at, $zero, %lo(NORMAL_MASK_SHIFT)             ## L:348  |     20 | vec16 normMask = load(NORMAL_MASK_SHIFT, 0x00).xyzwxyzw;
  ldv $v17, 0, 0, $at                                ## L:348  |     21 | vec16 normMask = load(NORMAL_MASK_SHIFT, 0x00).xyzwxyzw;
  ldv $v17, 8, 0, $at                                ## L:348  |     22 | vec16 normMask = load(NORMAL_MASK_SHIFT, 0x00).xyzwxyzw;
  ldv $v16, 0, 8, $at                                ## L:349  |     23 | vec16 normShift = load(NORMAL_MASK_SHIFT, 0x08).xyzwxyzw;
  ldv $v16, 8, 8, $at                                ## L:349  |     24 | vec16 normShift = load(NORMAL_MASK_SHIFT, 0x08).xyzwxyzw;
  ori $at, $zero, %lo(CLIPPING_PLANES)               ## L:351  |     25 | vec16 guardBandScale = load(CLIPPING_PLANES).xy;
  llv $v15, 0, 0, $at                                ## L:351  |     26 | vec16 guardBandScale = load(CLIPPING_PLANES).xy;
  vor $v15, $v00, $v15.e1                            ## L:352  |  ***30 | guardBandScale = guardBandScale.y;
  ori $at, $zero, %lo(SCREEN_SCALE_OFFSET)           ## L:354  |      ^ | vec32 screenSize:sint = load(SCREEN_SCALE_OFFSET).xyzwxyzw;
  ldv $v13, 0, 0, $at                                ## L:354  |     31 | vec32 screenSize:sint = load(SCREEN_SCALE_OFFSET).xyzwxyzw;
  ldv $v13, 8, 0, $at                                ## L:354  |     32 | vec32 screenSize:sint = load(SCREEN_SCALE_OFFSET).xyzwxyzw;
  vmudl $v14, $v00, $v31.e7                          ## L:356  |      ^ | screenSize >>= 8;
  vmadm $v13, $v13, $v31.e7                          ## L:356  |  ***36 | screenSize >>= 8;
  vmadn $v14, $v00, $v00                             ## L:356  |     37 | screenSize >>= 8;
  ldv $v12, 0, 8, $at                                ## L:358  |      ^ | vec16 screenOffset = load(SCREEN_SCALE_OFFSET, 0x08).xyzwxyzw;
  ldv $v12, 8, 8, $at                                ## L:358  |     38 | vec16 screenOffset = load(SCREEN_SCALE_OFFSET, 0x08).xyzwxyzw;
  ori $at, $zero, %lo(FOG_SCALE_OFFSET)              ## L:359  |     39 | vec16 fogScaleOffset = load(FOG_SCALE_OFFSET).xyzw;
  ldv $v11, 0, 0, $at                                ## L:359  |     40 | vec16 fogScaleOffset = load(FOG_SCALE_OFFSET).xyzw;
  ori $at, $zero, %lo(NORM_SCALE_W)                  ## L:362  |     41 | vec16 normScaleW = load(NORM_SCALE_W).xyzwxyzw;
  ldv $v10, 0, 0, $at                                ## L:362  |     42 | vec16 normScaleW = load(NORM_SCALE_W).xyzwxyzw;
  ldv $v10, 8, 0, $at                                ## L:362  |     43 | vec16 normScaleW = load(NORM_SCALE_W).xyzwxyzw;
  andi $s6, $a2, 65535                               ## L:364  |     44 | u32 ptrBuffA = addressInOut & 0xFFFF;
  addiu $s5, $s6, 65500                              ## L:365  |     45 | u32 ptrBuffB = ptrBuffA - 36;
  lbu $s2, %lo(ACTIVE_LIGHT_SIZE + 0)                ## L:369  |     46 | u8 ptrLightEnd = load(ACTIVE_LIGHT_SIZE);
  addiu $s2, $s2, %lo(LIGHT_DIR_COLOR)               ## L:370  |   **49 | ptrLightEnd += LIGHT_DIR_COLOR;
  ori $at, $zero, %lo(UV_GEN_PARAMS)                 ## L:374  |     50 | uvGenArgs.XYZW = load(UV_GEN_PARAMS).xyzw;
  ldv $v09, 8, 0, $at                                ## L:374  |     51 | uvGenArgs.XYZW = load(UV_GEN_PARAMS).xyzw;
  ori $at, $zero, %lo(SCREEN_UVGEN_SCALE)            ## L:375  |     52 | uvGenArgs.W = load(SCREEN_UVGEN_SCALE).x;
  lsv $v09, 14, 0, $at                               ## L:375  |     53 | uvGenArgs.W = load(SCREEN_UVGEN_SCALE).x;
  ori $at, $zero, %lo(UV_GEN_PARAMS)                 ## L:376  |     54 | uvGenArgs.xy = load(UV_GEN_PARAMS).xy;
  llv $v09, 0, 0, $at                                ## L:376  |     55 | uvGenArgs.xy = load(UV_GEN_PARAMS).xy;
  llv $v09, 4, 0, $at                                ## L:377  |     56 | uvGenArgs.zw = load(UV_GEN_PARAMS).xy;
  lbu $s1, %lo(FOG_STORE_OFFSET + 0)                 ## L:379  |     57 | u8 fogStoreOffset = load(FOG_STORE_OFFSET);
  addiu $s6, $s6, 65464                              ## L:380  |     58 | ptrBuffA -= 72;
  lbu $k0, %lo(TRI_COMMAND + 0)                      ## L:385  |     59 | u8 isUnlit = load(TRI_COMMAND);
  lbu $s0, %lo(CLIP_CODE_ORG_ADDR + 0)               ## L:389  |     60 | u8<$s0> baseTileMask = load(CLIP_CODE_ORG_ADDR);
  jal DMAWaitIdle                                    ## L:393  |     61 | dma_await();
  vmov $v11.e6, $v11.e2                              ## L:360  |    *63 | fogScaleOffset.Z = fogScaleOffset.z;
  lqv $v08, 0, 0, $s4                                ## L:396  |     64 | vec16 pos = load(prt3d, 0x00);
  vand $v07, $v17, $v08.h3                           ## L:397  |  ***68 | vec16 norm = normMask & pos.wwwwWWWW;
  vmudn $v07, $v07, $v16.v                           ## L:398  |  ***72 | norm *= normShift;
  LOOP_START:
  vor $v03, $v00, $v11.e2                            ## L:411  |     73 | vec16 lightColor = fogScaleOffset.z;
  lbu $a2, 19($s4)                                   ## L:414  |      ^ | baseTile = load(prt3d, 0x13);
  or $t9, $zero, $zero                               ## L:417  |     74 | u8 mvMatrixLoaded = 0;
  addiu $s6, $s6, 72                                 ## L:419  |     75 | ptrBuffA += 72;
  addiu $s5, $s5, 72                                 ## L:420  |     76 | ptrBuffB += 72;
  lhu $k1, %lo(VERTEX_FX_FUNC + 0)                   ## L:421  |     77 | vertexFxFunc = load(VERTEX_FX_FUNC);
  vmulf $v29, $v20, $v07.h0                          ## L:425  |      ^ | VTEMP:sfract = matN0:sfract  * norm.xxxxXXXX;
  vmacf $v29, $v19, $v07.h1                          ## L:426  |     78 | VTEMP:sfract = matN1:sfract +* norm.yyyyYYYY;
  vmacf $v07, $v18, $v07.h2                          ## L:427  |     79 | norm         = matN2:sfract +* norm.zzzzZZZZ;
  vmudn $v06, $v28, $v08.h0                          ## L:212  |     80 | out = mat0  * vec.xxxxXXXX;
  vmadh $v05, $v27, $v08.h0                          ## L:212  |     81 | out = mat0  * vec.xxxxXXXX;
  vmadn $v06, $v26, $v08.h1                          ## L:213  |     82 | out = mat1 +* vec.yyyyYYYY;
  vmadh $v05, $v25, $v08.h1                          ## L:213  |     83 | out = mat1 +* vec.yyyyYYYY;
  vmadn $v06, $v24, $v08.h2                          ## L:214  |     84 | out = mat2 +* vec.zzzzZZZZ;
  vmadh $v05, $v23, $v08.h2                          ## L:214  |     85 | out = mat2 +* vec.zzzzZZZZ;
  vmadn $v06, $v22, $v30.e7                          ## L:215  |     86 | out = mat3 +* 1;
  vmadh $v05, $v21, $v30.e7                          ## L:215  |     87 | out = mat3 +* 1;
  ori $s3, $zero, %lo(LIGHT_DIR_COLOR)               ## L:432  |      ^ | ptrLight = LIGHT_DIR_COLOR;
  lpv $v02, 0, 8, $s3                                ## L:435  |     88 | vec16 lightDirVec  = load_vec_s8(ptrLight, 8);
  lbu $t8, 15($s3)                                   ## L:436  |     89 | u8 isPointLight = load(ptrLight, 15);
  bne $k0, $zero, LIGHT_LOOP_END                     ## L:439  |     90 | if(isUnlit)goto LIGHT_LOOP_END;
  and $a2, $a2, $s0                                  ## L:415  |    *92 | baseTile &= baseTileMask;
  beq $s3, $s2, LIGHT_LOOP_END                       ## L:442  |     93 | if(ptrLight != ptrLightEnd)
  luv $v03, 0, -8, $s3                               ## L:440  |    *95 | lightColor = load_vec_u8(ptrLight, -8);
  LABEL_T3DCmd_VertLoad_0003:
  luv $v04, 0, 0, $s3                                ## L:447  |     96 | color = load_vec_u8(ptrLight);
  ori $ra, $zero, LABEL_T3DCmd_VertLoad_0005         ## L:970  |     97 | 
  bne $t8, $zero, pointLight                         ## L:449  |     98 | if(isPointLight) {
  vmulf $v01, $v07, $v02.v                           ## L:446  |   *100 | vec16 lightDirScale = norm:sfract * lightDirVec:sfract;
  LABEL_T3DCmd_VertLoad_0005:
  vmulu $v29, $v04, $v01.h0                          ## L:457  | ***104 | VTEMP = color:ufract  * lightDirScale:ufract.xxxxXXXX;
  vmacu $v29, $v04, $v01.h1                          ## L:458  |    105 | VTEMP = color:ufract +* lightDirScale:ufract.yyyyYYYY;
  vmacu $v29, $v04, $v01.h2                          ## L:459  |    106 | VTEMP = color:ufract +* lightDirScale:ufract.zzzzZZZZ;
  addiu $s3, $s3, 16                                 ## L:463  |      ^ | ptrLight += 16;
  lpv $v02, 0, 8, $s3                                ## L:464  |    107 | lightDirVec = load_vec_s8(ptrLight, 8);
  lbu $t8, 15($s3)                                   ## L:465  |    108 | isPointLight =  load(ptrLight, 15);
  bne $s3, $s2, LABEL_T3DCmd_VertLoad_0003           ## L:465  |    109 | isPointLight =  load(ptrLight, 15);
  vadd $v03, $v03, $v29.v                            ## L:461  |   *111 | lightColor:sint += VTEMP;
  LIGHT_LOOP_END:
  vmudm $v29, $v15, $v06.h3                          ## L:471  |    112 | vec32 clipPlaneW = guardBandScale * posClip.wwwwWWWW;
  vmadh $v01, $v15, $v05.h3                          ## L:471  |    113 | vec32 clipPlaneW = guardBandScale * posClip.wwwwWWWW;
  luv $v04, 0, 16, $s4                               ## L:473  |      ^ | color = load_vec_u8(prt3d, 0x10);
  addu $fp, $s6, $s1                                 ## L:474  |    114 | fogA = ptrBuffA + fogStoreOffset;
  ori $ra, $zero, LABEL_T3DCmd_VertLoad_0006         ## L:970  |    115 | 
  bne $t9, $zero, loadCurrentMVPMat                  ## L:476  |    116 | if(mvMatrixLoaded) {
  vmadn $v02, $v00, $v00                             ## L:471  |   *118 | vec32 clipPlaneW = guardBandScale * posClip.wwwwWWWW;
  LABEL_T3DCmd_VertLoad_0006:
  vch $v29, $v05, $v05.h3                            ## L:484  |    119 | u32 rejCodes = clip(posClip, posClip.wwwwWWWW);
  vcl $v29, $v06, $v06.h3                            ## L:484  |    120 | u32 rejCodes = clip(posClip, posClip.wwwwWWWW);
  cfc2 $t4, $vcc                                     ## L:484  |    121 | u32 rejCodes = clip(posClip, posClip.wwwwWWWW);
  vch $v29, $v05, $v01                               ## L:485  |      ^ | u32 clipCodes = clip(posClip, clipPlaneW);
  vcl $v29, $v06, $v02                               ## L:485  |    122 | u32 clipCodes = clip(posClip, clipPlaneW);
  cfc2 $t3, $vcc                                     ## L:485  |   *124 | u32 clipCodes = clip(posClip, clipPlaneW);
  srl $t7, $t3, 4                                    ## L:487  |  **127 | clipCodeB = clipCodes >> 4;
  andi $t8, $t3, 775                                 ## L:246  |    128 | res = clipCode & 0b0000'0011'0000'0111;
  srl $t2, $t8, 5                                    ## L:247  |    129 | u32 tmp = res >> 5;
  or $t8, $t8, $t2                                   ## L:248  |    130 | res |= tmp;
  andi $t7, $t7, 775                                 ## L:246  |    131 | res = clipCode & 0b0000'0011'0000'0111;
  srl $t2, $t7, 5                                    ## L:247  |    132 | u32 tmp = res >> 5;
  or $t7, $t7, $t2                                   ## L:248  |    133 | res |= tmp;
  or $t8, $t8, $a2                                   ## L:491  |    134 | clipCodeA |= baseTile;
  or $t7, $t7, $a2                                   ## L:492  |    135 | clipCodeB |= baseTile;
  srl $t5, $t4, 4                                    ## L:494  |    136 | rejCodesB = rejCodes >> 4;
  andi $t6, $t4, 1799                                ## L:254  |    137 | res = clipCode & 0b0000'0111'0000'0111;
  srl $t2, $t6, 5                                    ## L:255  |    138 | u32 tmp = res >> 5;
  nor $t6, $t6, $t2                                  ## L:256  |    139 | res = res ~| tmp;
  andi $t5, $t5, 1799                                ## L:254  |    140 | res = clipCode & 0b0000'0111'0000'0111;
  srl $t2, $t5, 5                                    ## L:255  |    141 | u32 tmp = res >> 5;
  nor $t5, $t5, $t2                                  ## L:256  |    142 | res = res ~| tmp;
  TAG_LightMul: vmulf $v04, $v04, $v03.v                           ## L:501  |      ^ | color:sfract *= lightColor:ufract;
  vmudl $v29, $v06, $v10.v                           ## L:503  |    143 | posClip *= normScaleW:ufract;
  vmadm $v05, $v05, $v10.v                           ## L:503  |    144 | posClip *= normScaleW:ufract;
  vmadn $v06, $v00, $v00                             ## L:503  |    145 | posClip *= normScaleW:ufract;
  vmudm $v04, $v04, $v31.e6                          ## L:505  |    146 | color >>= 7;
  vmudh $v04, $v04, $v09.e6                          ## L:506  | ***150 | color:sint *= uvGenArgs:sint.Z;
  suv $v04, 0, 8, $s6                                ## L:508  | ***154 | @Barrier("color-fog") store_vec_u8(color.x, ptrBuffA, 0x08); ## Barrier: 0x1
  suv $v04, 4, 8, $s5                                ## L:509  |    155 | @Barrier("color-fog") store_vec_u8(color.X, ptrBuffB, 0x08); ## Barrier: 0x1
  sdv $v05, 0, 16, $s6                               ## L:513  |    156 | store(posClip.xyzw, ptrBuffA, 0x10);
  sdv $v06, 0, 24, $s6                               ## L:513  |    157 | store(posClip.xyzw, ptrBuffA, 0x10);
  sdv $v05, 8, 16, $s5                               ## L:514  |    158 | store(posClip.XYZW, ptrBuffB, 0x10);
  sdv $v06, 8, 24, $s5                               ## L:514  |    159 | store(posClip.XYZW, ptrBuffB, 0x10);
  vrcph $v13.e3, $v05.e3                             ## L:519  |      ^ | screenSize.w = invert_half(posClip).w;
  vrcpl $v14.e3, $v06.e3                             ## L:519  |    160 | screenSize.w = invert_half(posClip).w;
  vrcph $v13.e3, $v05.e7                             ## L:520  |    161 | screenSize.W = invert_half(posClip).W;
  vrcpl $v14.e7, $v06.e7                             ## L:520  |    162 | screenSize.W = invert_half(posClip).W;
  vrcph $v13.e7, $v00.e7                             ## L:520  | ***166 | screenSize.W = invert_half(posClip).W;
  ssv $v14, 6, 34, $s6                               ## L:524  |      ^ | store(screenSize.w, ptrBuffA, 0x20);
  ssv $v13, 14, 32, $s5                              ## L:525  | ***170 | store(screenSize.W, ptrBuffB, 0x20);
  ssv $v14, 14, 34, $s5                              ## L:525  |    171 | store(screenSize.W, ptrBuffB, 0x20);
  vaddc $v03, $v06, $v11.e1                          ## L:531  |      ^ | fog:ufract = posClip + fogScaleOffset:ufract.y;
  vadd $v02, $v05, $v11.e0                           ## L:532  |    172 | fog:sint   = posClip + fogScaleOffset.x;
  vmudl $v29, $v06, $v14.h3                          ## L:534  |    173 | posClip *= screenSize.wwwwWWWW;
  vmadm $v29, $v05, $v14.h3                          ## L:534  |    174 | posClip *= screenSize.wwwwWWWW;
  vmadn $v06, $v06, $v13.h3                          ## L:534  |    175 | posClip *= screenSize.wwwwWWWW;
  vmadh $v05, $v05, $v13.h3                          ## L:534  |    176 | posClip *= screenSize.wwwwWWWW;
  vmudh $v02, $v02, $v11.e3                          ## L:536  |    177 | fog:sint *= fogScaleOffset:sint.w;
  vmudl $v29, $v06, $v14.v                           ## L:538  |   *179 | posClip *= screenSize;
  vmadm $v29, $v05, $v14.v                           ## L:538  |    180 | posClip *= screenSize;
  vmadn $v06, $v06, $v13.v                           ## L:538  |    181 | posClip *= screenSize;
  vmadh $v05, $v05, $v13.v                           ## L:538  |    182 | posClip *= screenSize;
  vsub $v29, $v11, $v02.h2                           ## L:540  |    183 | VTEMP:sint = fogScaleOffset - fog:sint.zzzzZZZZ;
  addu $sp, $s5, $s1                                 ## L:543  |      ^ | fogB = ptrBuffB + fogStoreOffset;
  vmadh $v05, $v12, $v30.e7                          ## L:547  |    184 | posClip:sint = screenOffset:sint +* 1;
  vmudn $v29, $v29, $v30.e6                          ## L:549  |  **187 | VTEMP <<= 1;
  ssv $v29, 4, 0, $fp                                ## L:550  | ***191 | @Barrier("color-fog") store(VTEMP:sint.z, fogA); ## Barrier: 0x1
  ssv $v29, 12, 0, $sp                               ## L:551  |    192 | @Barrier("color-fog") store(VTEMP:sint.Z, fogB); ## Barrier: 0x1
  ldv $v03, 0, 24, $s4                               ## L:553  |    193 | vec16 uv = load(prt3d, 0x18).xyzw;
  sdv $v05, 0, 0, $s6                                ## L:555  |    194 | @Barrier("pos-cc") store(posClip:sint.xyzw, ptrBuffA, 0x00); ## Barrier: 0x2
  sdv $v05, 8, 0, $s5                                ## L:556  |   *196 | @Barrier("pos-cc") store(posClip:sint.XYZW, ptrBuffB, 0x00); ## Barrier: 0x2
  sb $t8, 6($s6)                                     ## L:559  |    197 | @Barrier("pos-cc") store(clipCodeA:u8, ptrBuffA, 0x06); ## Barrier: 0x2
  sb $t6, 7($s6)                                     ## L:560  |    198 | @Barrier("pos-cc") store(rejCodesA:u8, ptrBuffA, 0x07); ## Barrier: 0x2
  sb $t7, 6($s5)                                     ## L:562  |    199 | @Barrier("pos-cc") store(clipCodeB:u8, ptrBuffB, 0x06); ## Barrier: 0x2
  sb $t5, 7($s5)                                     ## L:563  |    200 | @Barrier("pos-cc") store(rejCodesB:u8, ptrBuffB, 0x07); ## Barrier: 0x2
  vaddc $v03, $v03, $v09.v                           ## L:565  |      ^ | uv += uvGenArgs;
  lqv $v08, 0, 32, $s4                               ## L:567  |    201 | pos = load(prt3d, 0x20);
  vor $v02, $v00, $v07                               ## L:568  |      ^ | vec16 oldNorm = norm;
  vand $v07, $v17, $v08.h3                           ## L:569  | ***205 | norm = normMask & pos.wwwwWWWW;
  vmudn $v07, $v07, $v16.v                           ## L:570  | ***209 | norm *= normShift;
  addiu $s4, $s4, 32                                 ## L:572  |      ^ | prt3d += 0x20;
  jal $k1                                            ## L:574  |    210 | vertexFxFunc();
  ssv $v13, 6, 32, $s6                               ## L:524  |   *212 | store(screenSize.w, ptrBuffA, 0x20);
VertexFX_None:
  slv $v03, 4, 12, $s5                               ## L:592  |      ^ | store(uv.zw, ptrBuffB, 0x0C);
  bne $s4, $s7, LOOP_START                           ## L:583  |      2 | if(prt3d == ptr3dEnd)goto RSPQ_Loop;
  slv $v03, 0, 12, $s6                               ## L:591  |     *4 | store(uv.xy, ptrBuffA, 0x0C);
  j RSPQ_Loop                                        ## L:583  |      5 | if(prt3d == ptr3dEnd)goto RSPQ_Loop;
  nop                                                ## L:583  |     *7 | if(prt3d == ptr3dEnd)goto RSPQ_Loop;
VertexFX_Spherical:
  vge $v05, $v05, $v00.e0                            ## L:605  |      ^ | posClipInt = posClipInt >= 0;
  vsubc $v04, $v05, $v12.v                           ## L:606  |   ***5 | uvOffsetFactor = posClipInt - screenOffset;
  vlt $v04, $v04, $v12                               ## L:607  |   ***9 | uvOffsetFactor = uvOffsetFactor < screenOffset;
  vmulf $v03, $v02, $v09.v                           ## L:608  |     10 | uv:sfract = normal * uvGenArgs;
  vmudn $v04, $v04, $v09.e7                          ## L:610  |   **13 | uvOffsetFactor *= uvGenArgs.W;
  vmulf $v04, $v04, $v02.h2                          ## L:611  |  ***17 | uvOffsetFactor:sfract *= normal.zzzzZZZZ;
  vmulf $v04, $v04, $v09.e0                          ## L:612  |  ***21 | uvOffsetFactor:sfract *= uvGenArgs.x;
  vaddc $v03, $v03, $v09.e0                          ## L:614  |     22 | uv += uvGenArgs.x;
  vaddc $v03, $v03, $v04.v                           ## L:615  |  ***26 | uv += uvOffsetFactor;
  slv $v03, 8, 12, $s5                               ## L:618  |  ***30 | store(uv.XY, ptrBuffB, 0x0C);
  bne $s4, $s7, LOOP_START                           ## L:583  |     31 | if(prt3d == ptr3dEnd)goto RSPQ_Loop;
  slv $v03, 0, 12, $s6                               ## L:617  |    *33 | store(uv.xy, ptrBuffA, 0x0C);
  j RSPQ_Loop                                        ## L:583  |     34 | if(prt3d == ptr3dEnd)goto RSPQ_Loop;
  nop                                                ## L:583  |    *36 | if(prt3d == ptr3dEnd)goto RSPQ_Loop;
VertexFX_CelShadeColor:
  vge $v04, $v04, $v04.h1                            ## L:629  |      ^ | color = max(color, color.yyyyYYYY);
  vge $v04, $v04, $v04.h2                            ## L:630  |   ***5 | color = max(color, color.zzzzZZZZ);
  vmov $v03.e0, $v04.e0                              ## L:631  |   ***9 | uv.x = color.x;
  vmov $v03.e2, $v04.e4                              ## L:632  |     10 | uv.z = color.X;
  slv $v03, 4, 12, $s5                               ## L:635  |  ***14 | store(uv.zw, ptrBuffB, 0x0C);
  bne $s4, $s7, LOOP_START                           ## L:583  |     15 | if(prt3d == ptr3dEnd)goto RSPQ_Loop;
  slv $v03, 0, 12, $s6                               ## L:634  |    *17 | store(uv.xy, ptrBuffA, 0x0C);
  j RSPQ_Loop                                        ## L:583  |     18 | if(prt3d == ptr3dEnd)goto RSPQ_Loop;
  nop                                                ## L:583  |    *20 | if(prt3d == ptr3dEnd)goto RSPQ_Loop;
VertexFX_CelShadeAlpha:
  vmov $v03.e0, $v04.e3                              ## L:646  |      ^ | uv.x = color.w;
  vmov $v03.e2, $v04.e7                              ## L:647  |      2 | uv.z = color.W;
  slv $v03, 4, 12, $s5                               ## L:650  |   ***6 | store(uv.zw, ptrBuffB, 0x0C);
  bne $s4, $s7, LOOP_START                           ## L:583  |      7 | if(prt3d == ptr3dEnd)goto RSPQ_Loop;
  slv $v03, 0, 12, $s6                               ## L:649  |     *9 | store(uv.xy, ptrBuffA, 0x0C);
  j RSPQ_Loop                                        ## L:583  |     10 | if(prt3d == ptr3dEnd)goto RSPQ_Loop;
  nop                                                ## L:583  |    *12 | if(prt3d == ptr3dEnd)goto RSPQ_Loop;
VertexFX_Outline:
  vmulf $v29, $v02, $v09.v                           ## L:662  |      ^ | VTEMP = normal:sfract * uvGenArgs;
  vmadh $v05, $v05, $v30.e7                          ## L:663  |      2 | posClipInt:sint = posClipInt:sint +* 1;
  slv $v05, 8, 0, $s5                                ## L:669  |   ***6 | store(posClipInt.XY, ptrBuffB, 0x00);
  bne $s4, $s7, LOOP_START                           ## L:583  |      7 | if(prt3d == ptr3dEnd)goto RSPQ_Loop;
  slv $v05, 0, 0, $s6                                ## L:668  |     *9 | store(posClipInt.xy, ptrBuffA, 0x00);
  j RSPQ_Loop                                        ## L:583  |     10 | if(prt3d == ptr3dEnd)goto RSPQ_Loop;
  nop                                                ## L:583  |    *12 | if(prt3d == ptr3dEnd)goto RSPQ_Loop;
pointLight:
  bne $t9, $zero, LABEL_pointLight_000C              ## L:685  |      ^ | if(!mvMatrixLoaded) {
  lhu $t7, 14($s3)                                   ## L:684  |     *3 | lightSize:u16 = load(ptrLight, 14);
  ori $at, $zero, %lo(MATRIX_MV)                     ## L:261  |      4 | mat0 = load(MATRIX_MV, 0x00).xyzwxyzw;
  ldv $v27, 0, 0, $at                                ## L:261  |      5 | mat0 = load(MATRIX_MV, 0x00).xyzwxyzw;
  ldv $v27, 8, 0, $at                                ## L:261  |      6 | mat0 = load(MATRIX_MV, 0x00).xyzwxyzw;
  ldv $v28, 0, 8, $at                                ## L:261  |      7 | mat0 = load(MATRIX_MV, 0x00).xyzwxyzw;
  ldv $v28, 8, 8, $at                                ## L:261  |      8 | mat0 = load(MATRIX_MV, 0x00).xyzwxyzw;
  ldv $v25, 0, 16, $at                               ## L:262  |      9 | mat1 = load(MATRIX_MV, 0x10).xyzwxyzw;
  ldv $v25, 8, 16, $at                               ## L:262  |     10 | mat1 = load(MATRIX_MV, 0x10).xyzwxyzw;
  ldv $v26, 0, 24, $at                               ## L:262  |     11 | mat1 = load(MATRIX_MV, 0x10).xyzwxyzw;
  ldv $v26, 8, 24, $at                               ## L:262  |     12 | mat1 = load(MATRIX_MV, 0x10).xyzwxyzw;
  ldv $v23, 0, 32, $at                               ## L:263  |     13 | mat2 = load(MATRIX_MV, 0x20).xyzwxyzw;
  ldv $v23, 8, 32, $at                               ## L:263  |     14 | mat2 = load(MATRIX_MV, 0x20).xyzwxyzw;
  ldv $v24, 0, 40, $at                               ## L:263  |     15 | mat2 = load(MATRIX_MV, 0x20).xyzwxyzw;
  ldv $v24, 8, 40, $at                               ## L:263  |     16 | mat2 = load(MATRIX_MV, 0x20).xyzwxyzw;
  ldv $v21, 0, 48, $at                               ## L:264  |     17 | mat3 = load(MATRIX_MV, 0x30).xyzwxyzw;
  ldv $v21, 8, 48, $at                               ## L:264  |     18 | mat3 = load(MATRIX_MV, 0x30).xyzwxyzw;
  ldv $v22, 0, 56, $at                               ## L:264  |     19 | mat3 = load(MATRIX_MV, 0x30).xyzwxyzw;
  ldv $v22, 8, 56, $at                               ## L:264  |     20 | mat3 = load(MATRIX_MV, 0x30).xyzwxyzw;
  vmudn $v28, $v28, $v08.h0                          ## L:212  |      ^ | out = mat0  * vec.xxxxXXXX;
  vmadh $v27, $v27, $v08.h0                          ## L:212  |     21 | out = mat0  * vec.xxxxXXXX;
  vmadn $v28, $v26, $v08.h1                          ## L:213  |     22 | out = mat1 +* vec.yyyyYYYY;
  vmadh $v27, $v25, $v08.h1                          ## L:213  |     23 | out = mat1 +* vec.yyyyYYYY;
  vmadn $v28, $v24, $v08.h2                          ## L:214  |     24 | out = mat2 +* vec.zzzzZZZZ;
  vmadh $v27, $v23, $v08.h2                          ## L:214  |     25 | out = mat2 +* vec.zzzzZZZZ;
  vmadn $v28, $v22, $v30.e7                          ## L:215  |     26 | out = mat3 +* 1;
  vmadh $v27, $v21, $v30.e7                          ## L:215  |     27 | out = mat3 +* 1;
  vmudl $v28, $v28, $v31.e3                          ## L:691  |   **30 | vertPos >>= 4;
  vmadm $v27, $v27, $v31.e3                          ## L:691  |     31 | vertPos >>= 4;
  vmadn $v28, $v00, $v00                             ## L:691  |     32 | vertPos >>= 4;
  addiu $t9, $zero, 1                                ## L:692  |      ^ | mvMatrixLoaded = 1;
  LABEL_pointLight_000C:
  andi $t7, $t7, 32767                               ## L:695  |     33 | lightSize &= 0x7FFF;
  sll $t7, $t7, 16                                   ## L:696  |     34 | lightSize <<= 16;
  ldv $v23, 0, 8, $s3                                ## L:699  |     35 | ptPos:sint.xyzw = load(ptrLight, 8).xyzw;
  ldv $v23, 8, 8, $s3                                ## L:700  |     36 | ptPos:sint.XYZW = load(ptrLight, 8).xyzw;
  vmudm $v23, $v23, $v31.e5                          ## L:702  |  ***40 | ptPos = ptPos:sint >> 6;
  vmadn $v24, $v00, $v00                             ## L:702  |     41 | ptPos = ptPos:sint >> 6;
  vsubc $v24, $v24, $v28.v                           ## L:703  |  ***45 | ptPos = ptPos - vertPos;
  vsub $v23, $v23, $v27.v                            ## L:703  |     46 | ptPos = ptPos - vertPos;
  vmudl $v29, $v24, $v24.v                           ## L:706  |   **49 | lightVertDot = ptPos * ptPos;
  vmadm $v29, $v23, $v24.v                           ## L:706  |     50 | lightVertDot = ptPos * ptPos;
  vmadn $v22, $v24, $v23.v                           ## L:706  |     51 | lightVertDot = ptPos * ptPos;
  vmadh $v21, $v23, $v23.v                           ## L:706  |     52 | lightVertDot = ptPos * ptPos;
  vaddc $v26, $v22, $v22.h1                          ## L:707  |   **55 | mat1         = lightVertDot + lightVertDot.yyyyYYYY;
  vadd $v25, $v21, $v21.h1                           ## L:707  |     56 | mat1         = lightVertDot + lightVertDot.yyyyYYYY;
  vaddc $v22, $v22, $v26.h2                          ## L:708  |   **59 | lightVertDot = lightVertDot + mat1.zzzzZZZZ;
  vadd $v21, $v21, $v25.h2                           ## L:708  |     60 | lightVertDot = lightVertDot + mat1.zzzzZZZZ;
  mtc2 $t7, $v26.e3                                  ## L:710  |      ^ | mat1.w = lightSize;
  srl $at, $t7, 16                                   ## L:710  |     61 | mat1.w = lightSize;
  mtc2 $at, $v25.e3                                  ## L:710  |    *63 | mat1.w = lightSize;
  lbu $t7, 14($s3)                                   ## L:712  |     64 | lightSize:u8 = load(ptrLight, 14);
  sra $t7, $t7, 7                                    ## L:713  |   **67 | lightSize >>= 7;
  vrsqh $v21.e0, $v21.e0                             ## L:716  |      ^ | lightVertDot.x = invert_half_sqrt(lightVertDot).x;
  vrsql $v22.e0, $v22.e0                             ## L:716  |     68 | lightVertDot.x = invert_half_sqrt(lightVertDot).x;
  vrsqh $v21.e0, $v21.e4                             ## L:717  |   **71 | lightVertDot.X = invert_half_sqrt(lightVertDot).X;
  vrsql $v22.e4, $v22.e4                             ## L:717  |     72 | lightVertDot.X = invert_half_sqrt(lightVertDot).X;
  vrsqh $v21.e4, $v00.e0                             ## L:717  |     73 | lightVertDot.X = invert_half_sqrt(lightVertDot).X;
  vmudl $v29, $v22, $v26.e3                          ## L:718  |   **76 | lightRes = lightVertDot * mat1.w;
  vmadm $v29, $v21, $v26.e3                          ## L:718  |     77 | lightRes = lightVertDot * mat1.w;
  vmadn $v29, $v22, $v25.e3                          ## L:718  |     78 | lightRes = lightVertDot * mat1.w;
  bne $t7, $zero, LABEL_pointLight_000D              ## L:720  |      ^ | if(!lightSize) {
  vmadh $v01, $v21, $v25.e3                          ## L:718  |    *80 | lightRes = lightVertDot * mat1.w;
  vmudn $v22, $v22, $v31.e7                          ## L:723  |     81 | lightVertDot *= 256;
  vmadh $v21, $v21, $v31.e7                          ## L:723  |     82 | lightVertDot *= 256;
  vmulf $v01, $v01, $v01.v                           ## L:725  |    *84 | lightRes:sfract *= lightRes:sfract;
  vmudl $v29, $v24, $v22.h0                          ## L:727  |     85 | pos:sint = ptPos * lightVertDot.xxxxXXXX;
  vmadm $v29, $v23, $v22.h0                          ## L:727  |     86 | pos:sint = ptPos * lightVertDot.xxxxXXXX;
  vmadn $v29, $v24, $v21.h0                          ## L:727  |     87 | pos:sint = ptPos * lightVertDot.xxxxXXXX;
  vmadh $v08, $v23, $v21.h0                          ## L:727  |     88 | pos:sint = ptPos * lightVertDot.xxxxXXXX;
  vmulf $v08, $v08, $v01.h0                          ## L:728  |  ***92 | pos:sfract *= lightRes:sfract.xxxxXXXX;
  beq $zero, $zero, LABEL_pointLight_000E            ## L:731  |      ^ | lightRes = pos:sfract * norm:sfract;
  vmulf $v01, $v08, $v07.v                           ## L:731  | ****97 | lightRes = pos:sfract * norm:sfract;
  LABEL_pointLight_000D:
  vmulf $v01, $v01, $v01.v                           ## L:734  | ***101 | lightRes:sfract *= lightRes:sfract;
  vmov $v01.e1, $v00.e0                              ## L:735  |    102 | lightRes.y = 0; lightRes.z = 0;
  vmov $v01.e2, $v00.e0                              ## L:735  |    103 | lightRes.y = 0; lightRes.z = 0;
  vmov $v01.e5, $v00.e0                              ## L:736  |    104 | lightRes.Y = 0; lightRes.Z = 0;
  vmov $v01.e6, $v00.e0                              ## L:736  |    105 | lightRes.Y = 0; lightRes.Z = 0;
  LABEL_pointLight_000E:
  jr $ra                                             ## L:737  |      ^ | }
  nop                                                ## L:737  |   *107 | }
loadCurrentMVPMat:
  ori $at, $zero, %lo(MATRIX_MVP)                    ## L:744  |      ^ | mat0 = load(MATRIX_MVP, 0x00).xyzwxyzw;
  ldv $v27, 8, 0, $at                                ## L:744  |      2 | mat0 = load(MATRIX_MVP, 0x00).xyzwxyzw;
  ldv $v28, 0, 8, $at                                ## L:744  |      3 | mat0 = load(MATRIX_MVP, 0x00).xyzwxyzw;
  ldv $v28, 8, 8, $at                                ## L:744  |      4 | mat0 = load(MATRIX_MVP, 0x00).xyzwxyzw;
  ldv $v25, 0, 16, $at                               ## L:745  |      5 | mat1 = load(MATRIX_MVP, 0x10).xyzwxyzw;
  ldv $v25, 8, 16, $at                               ## L:745  |      6 | mat1 = load(MATRIX_MVP, 0x10).xyzwxyzw;
  ldv $v26, 0, 24, $at                               ## L:745  |      7 | mat1 = load(MATRIX_MVP, 0x10).xyzwxyzw;
  ldv $v26, 8, 24, $at                               ## L:745  |      8 | mat1 = load(MATRIX_MVP, 0x10).xyzwxyzw;
  ldv $v23, 0, 32, $at                               ## L:746  |      9 | mat2 = load(MATRIX_MVP, 0x20).xyzwxyzw;
  ldv $v23, 8, 32, $at                               ## L:746  |     10 | mat2 = load(MATRIX_MVP, 0x20).xyzwxyzw;
  ldv $v24, 0, 40, $at                               ## L:746  |     11 | mat2 = load(MATRIX_MVP, 0x20).xyzwxyzw;
  ldv $v24, 8, 40, $at                               ## L:746  |     12 | mat2 = load(MATRIX_MVP, 0x20).xyzwxyzw;
  ldv $v21, 0, 48, $at                               ## L:747  |     13 | mat3 = load(MATRIX_MVP, 0x30).xyzwxyzw;
  ldv $v21, 8, 48, $at                               ## L:747  |     14 | mat3 = load(MATRIX_MVP, 0x30).xyzwxyzw;
  ldv $v22, 0, 56, $at                               ## L:747  |     15 | mat3 = load(MATRIX_MVP, 0x30).xyzwxyzw;
  ldv $v22, 8, 56, $at                               ## L:747  |     16 | mat3 = load(MATRIX_MVP, 0x30).xyzwxyzw;
  jr $ra                                             ## L:748  |     17 | }
  ldv $v27, 0, 0, $at                                ## L:744  |    *19 | mat0 = load(MATRIX_MVP, 0x00).xyzwxyzw;
T3DCmd_SetScreenSize:
  sw $a2, %lo(0 + SCREEN_SCALE_OFFSET)($zero)        ## L:759  |      ^ | store(screenScaleXY, SCREEN_SCALE_OFFSET, 0x00);
  sw $a1, %lo(8 + SCREEN_SCALE_OFFSET)($zero)        ## L:760  |      2 | store(screenOffsetXY, SCREEN_SCALE_OFFSET, 0x08);
  sh $a3, %lo(6 + NORM_SCALE_W)($zero)               ## L:762  |      3 | store(depthAndWScale:s16, NORM_SCALE_W, 6);
  srl $s7, $a0, 8                                    ## L:764  |      4 | u16 uvgenScaleFactor = guardBandFactor >> 8;
  sh $s7, %lo(SCREEN_UVGEN_SCALE)($zero)             ## L:765  |      5 | store(uvgenScaleFactor, SCREEN_UVGEN_SCALE);
  andi $a0, $a0, 15                                  ## L:768  |      6 | guardBandFactor &= 0xF;
  subu $s6, $zero, $a0                               ## L:769  |      7 | s8 guardBandFactorNeg = ZERO - guardBandFactor;
  sb $a0, %lo(3 + CLIPPING_PLANES)($zero)            ## L:771  |      8 | store(guardBandFactor,    CLIPPING_PLANES, 3);
  sb $a0, %lo(7 + CLIPPING_PLANES)($zero)            ## L:772  |      9 | store(guardBandFactor,    CLIPPING_PLANES, 7);
  sb $s6, %lo(15 + CLIPPING_PLANES)($zero)           ## L:774  |     10 | store(guardBandFactorNeg, CLIPPING_PLANES, 15);
  sb $s6, %lo(19 + CLIPPING_PLANES)($zero)           ## L:775  |     11 | store(guardBandFactorNeg, CLIPPING_PLANES, 19);
  j RSPQ_Loop                                        ## L:776  |     12 | }
  sw $a3, %lo(4 + SCREEN_SCALE_OFFSET)($zero)        ## L:758  |    *14 | store(depthAndWScale:u32, SCREEN_SCALE_OFFSET, 0x04);
T3DCmd_SetFogState:
  j RSPQ_Loop                                        ## L:785  |      ^ | }
  sb $a0, %lo(FOG_STORE_OFFSET)($zero)               ## L:784  |     *3 | store(enabled, FOG_STORE_OFFSET);
T3DCmd_LightSet:
  beq $a2, $zero, LABEL_T3DCmd_LightSet_000F         ## L:798  |      ^ | if(dirXYZ0) {
  sw $a1, 0($a0)                                     ## L:796  |     *3 | store(rgba8, addr, 0);
  sw $a2, 8($a0)                                     ## L:799  |      4 | store(dirXYZ0, addr, 8);
  sw $a3, 12($a0)                                    ## L:800  |      5 | store(dirXYZ1, addr, 12);
  LABEL_T3DCmd_LightSet_000F:
  j RSPQ_Loop                                        ## L:804  |      6 | }
  sw $a1, 4($a0)                                     ## L:803  |     *8 | store(rgba8, addr, 4);
T3DCmd_RenderMode:
  lbu $s7, %lo(RDPQ_OTHER_MODES + 0)                 ## L:815  |      ^ | u8 mipmap = load(RDPQ_OTHER_MODES);
  andi $s7, $s7, 56                                  ## L:816  |    **4 | mipmap &= 0x38;
  or $a1, $a1, $s7                                   ## L:817  |      5 | triCommand |= mipmap;
  sw $a1, %lo(TRI_COMMAND)($zero)                    ## L:819  |      6 | store(triCommand, TRI_COMMAND);
  j RSPQ_Loop                                        ## L:820  |      7 | }
  sb $a0, %lo(FACE_CULLING)($zero)                   ## L:812  |     *9 | store(culling, FACE_CULLING);
mulMat4Mat4:
  ldv $v01, 0, 0, $s3                                ## L:842  |      ^ | vec32 matL0 = load(addrMatL, 0x00).xyzwxyzw;
  ldv $v01, 8, 0, $s3                                ## L:842  |      2 | vec32 matL0 = load(addrMatL, 0x00).xyzwxyzw;
  ldv $v02, 0, 8, $s3                                ## L:842  |      3 | vec32 matL0 = load(addrMatL, 0x00).xyzwxyzw;
  ldv $v02, 8, 8, $s3                                ## L:842  |      4 | vec32 matL0 = load(addrMatL, 0x00).xyzwxyzw;
  ldv $v03, 0, 16, $s3                               ## L:843  |      5 | vec32 matL1 = load(addrMatL, 0x10).xyzwxyzw;
  ldv $v03, 8, 16, $s3                               ## L:843  |      6 | vec32 matL1 = load(addrMatL, 0x10).xyzwxyzw;
  ldv $v04, 0, 24, $s3                               ## L:843  |      7 | vec32 matL1 = load(addrMatL, 0x10).xyzwxyzw;
  ldv $v04, 8, 24, $s3                               ## L:843  |      8 | vec32 matL1 = load(addrMatL, 0x10).xyzwxyzw;
  ldv $v05, 0, 32, $s3                               ## L:844  |      9 | vec32 matL2 = load(addrMatL, 0x20).xyzwxyzw;
  ldv $v05, 8, 32, $s3                               ## L:844  |     10 | vec32 matL2 = load(addrMatL, 0x20).xyzwxyzw;
  ldv $v06, 0, 40, $s3                               ## L:844  |     11 | vec32 matL2 = load(addrMatL, 0x20).xyzwxyzw;
  ldv $v06, 8, 40, $s3                               ## L:844  |     12 | vec32 matL2 = load(addrMatL, 0x20).xyzwxyzw;
  ldv $v07, 0, 48, $s3                               ## L:845  |     13 | vec32 matL3 = load(addrMatL, 0x30).xyzwxyzw;
  ldv $v07, 8, 48, $s3                               ## L:845  |     14 | vec32 matL3 = load(addrMatL, 0x30).xyzwxyzw;
  ldv $v08, 0, 56, $s3                               ## L:845  |     15 | vec32 matL3 = load(addrMatL, 0x30).xyzwxyzw;
  ldv $v08, 8, 56, $s3                               ## L:845  |     16 | vec32 matL3 = load(addrMatL, 0x30).xyzwxyzw;
  ldv $v09, 0, 0, $s4                                ## L:849  |     17 | matR01.xyzw = load(addrMatR, 0x00).xyzw;
  ldv $v10, 0, 8, $s4                                ## L:849  |     18 | matR01.xyzw = load(addrMatR, 0x00).xyzw;
  ldv $v09, 8, 16, $s4                               ## L:850  |     19 | matR01.XYZW = load(addrMatR, 0x10).xyzw;
  ldv $v10, 8, 24, $s4                               ## L:850  |     20 | matR01.XYZW = load(addrMatR, 0x10).xyzw;
  ldv $v11, 0, 32, $s4                               ## L:851  |     21 | matR23.xyzw = load(addrMatR, 0x20).xyzw;
  ldv $v12, 0, 40, $s4                               ## L:851  |     22 | matR23.xyzw = load(addrMatR, 0x20).xyzw;
  ldv $v11, 8, 48, $s4                               ## L:852  |     23 | matR23.XYZW = load(addrMatR, 0x30).xyzw;
  ldv $v12, 8, 56, $s4                               ## L:852  |     24 | matR23.XYZW = load(addrMatR, 0x30).xyzw;
  vmudl $v29, $v02, $v10.h0                          ## L:855  |      ^ | tmp    = matL0  * matR01.xxxxXXXX;
  vmadm $v29, $v01, $v10.h0                          ## L:855  |     25 | tmp    = matL0  * matR01.xxxxXXXX;
  vmadn $v14, $v02, $v09.h0                          ## L:855  |     26 | tmp    = matL0  * matR01.xxxxXXXX;
  vmadh $v13, $v01, $v09.h0                          ## L:855  |     27 | tmp    = matL0  * matR01.xxxxXXXX;
  vmadl $v29, $v04, $v10.h1                          ## L:856  |     28 | tmp    = matL1 +* matR01.yyyyYYYY;
  vmadm $v29, $v03, $v10.h1                          ## L:856  |     29 | tmp    = matL1 +* matR01.yyyyYYYY;
  vmadn $v14, $v04, $v09.h1                          ## L:856  |     30 | tmp    = matL1 +* matR01.yyyyYYYY;
  vmadh $v13, $v03, $v09.h1                          ## L:856  |     31 | tmp    = matL1 +* matR01.yyyyYYYY;
  vmadl $v29, $v06, $v10.h2                          ## L:857  |     32 | tmp    = matL2 +* matR01.zzzzZZZZ;
  vmadm $v29, $v05, $v10.h2                          ## L:857  |     33 | tmp    = matL2 +* matR01.zzzzZZZZ;
  vmadn $v14, $v06, $v09.h2                          ## L:857  |     34 | tmp    = matL2 +* matR01.zzzzZZZZ;
  vmadh $v13, $v05, $v09.h2                          ## L:857  |     35 | tmp    = matL2 +* matR01.zzzzZZZZ;
  vmadl $v29, $v08, $v10.h3                          ## L:858  |     36 | matR01 = matL3 +* matR01.wwwwWWWW;
  vmadm $v29, $v07, $v10.h3                          ## L:858  |     37 | matR01 = matL3 +* matR01.wwwwWWWW;
  vmadn $v10, $v08, $v09.h3                          ## L:858  |     38 | matR01 = matL3 +* matR01.wwwwWWWW;
  vmadh $v09, $v07, $v09.h3                          ## L:858  |     39 | matR01 = matL3 +* matR01.wwwwWWWW;
  vmudl $v29, $v02, $v12.h0                          ## L:860  |     40 | tmp    = matL0  * matR23.xxxxXXXX;
  vmadm $v29, $v01, $v12.h0                          ## L:860  |     41 | tmp    = matL0  * matR23.xxxxXXXX;
  vmadn $v14, $v02, $v11.h0                          ## L:860  |     42 | tmp    = matL0  * matR23.xxxxXXXX;
  vmadh $v13, $v01, $v11.h0                          ## L:860  |     43 | tmp    = matL0  * matR23.xxxxXXXX;
  vmadl $v29, $v04, $v12.h1                          ## L:861  |     44 | tmp    = matL1 +* matR23.yyyyYYYY;
  vmadm $v29, $v03, $v12.h1                          ## L:861  |     45 | tmp    = matL1 +* matR23.yyyyYYYY;
  vmadn $v14, $v04, $v11.h1                          ## L:861  |     46 | tmp    = matL1 +* matR23.yyyyYYYY;
  vmadh $v13, $v03, $v11.h1                          ## L:861  |     47 | tmp    = matL1 +* matR23.yyyyYYYY;
  vmadl $v29, $v06, $v12.h2                          ## L:862  |     48 | tmp    = matL2 +* matR23.zzzzZZZZ;
  vmadm $v29, $v05, $v12.h2                          ## L:862  |     49 | tmp    = matL2 +* matR23.zzzzZZZZ;
  vmadn $v14, $v06, $v11.h2                          ## L:862  |     50 | tmp    = matL2 +* matR23.zzzzZZZZ;
  vmadh $v13, $v05, $v11.h2                          ## L:862  |     51 | tmp    = matL2 +* matR23.zzzzZZZZ;
  vmadl $v29, $v08, $v12.h3                          ## L:863  |     52 | matR23 = matL3 +* matR23.wwwwWWWW;
  vmadm $v29, $v07, $v12.h3                          ## L:863  |     53 | matR23 = matL3 +* matR23.wwwwWWWW;
  vmadn $v12, $v08, $v11.h3                          ## L:863  |     54 | matR23 = matL3 +* matR23.wwwwWWWW;
  vmadh $v11, $v07, $v11.h3                          ## L:863  |     55 | matR23 = matL3 +* matR23.wwwwWWWW;
  sdv $v10, 0, 8, $s2                                ## L:865  |      ^ | store(matR01.xyzw, addrOut, 0x00);
  sdv $v09, 8, 16, $s2                               ## L:866  |     56 | store(matR01.XYZW, addrOut, 0x10);
  sdv $v10, 8, 24, $s2                               ## L:866  |     57 | store(matR01.XYZW, addrOut, 0x10);
  sdv $v11, 0, 32, $s2                               ## L:867  |    *59 | store(matR23.xyzw, addrOut, 0x20);
  sdv $v12, 0, 40, $s2                               ## L:867  |     60 | store(matR23.xyzw, addrOut, 0x20);
  sdv $v11, 8, 48, $s2                               ## L:868  |     61 | store(matR23.XYZW, addrOut, 0x30);
  sdv $v12, 8, 56, $s2                               ## L:868  |     62 | store(matR23.XYZW, addrOut, 0x30);
  jr $ra                                             ## L:869  |     63 | }
  sdv $v09, 0, 0, $s2                                ## L:865  |    *65 | store(matR01.xyzw, addrOut, 0x00);
normalizeMatrixVector:
  ldv $v01, 0, 0, $s4                                ## L:881  |      ^ | vec32 v = load(addrIn, 0x00).xyzw;
  ldv $v02, 0, 8, $s4                                ## L:881  |      2 | vec32 v = load(addrIn, 0x00).xyzw;
  vmudl $v29, $v02, $v02.v                           ## L:884  |   ***6 | vec32 vLenInv = v * v;
  vmadm $v29, $v01, $v02.v                           ## L:884  |      7 | vec32 vLenInv = v * v;
  vmadn $v04, $v02, $v01.v                           ## L:884  |      8 | vec32 vLenInv = v * v;
  vmadh $v03, $v01, $v01.v                           ## L:884  |      9 | vec32 vLenInv = v * v;
  vaddc $v06, $v04, $v04.h1                          ## L:885  |   **12 | vec32 tmp = vLenInv + vLenInv.yyyyYYYY;
  vadd $v05, $v03, $v03.h1                           ## L:885  |     13 | vec32 tmp = vLenInv + vLenInv.yyyyYYYY;
  vaddc $v04, $v04, $v06.h2                          ## L:886  |   **16 | vLenInv   = vLenInv + tmp.zzzzZZZZ;
  vadd $v03, $v03, $v05.h2                           ## L:886  |     17 | vLenInv   = vLenInv + tmp.zzzzZZZZ;
  vrsqh $v03.e3, $v03.e0                             ## L:889  |  ***21 | vLenInv.w = invert_half_sqrt(vLenInv).x;
  vrsql $v04.e3, $v04.e0                             ## L:889  |     22 | vLenInv.w = invert_half_sqrt(vLenInv).x;
  vrsqh $v03.e3, $v00.e0                             ## L:889  |     23 | vLenInv.w = invert_half_sqrt(vLenInv).x;
  vmudl $v04, $v04, $v30.e0                          ## L:890  |   **26 | vLenInv >>= 9;
  vmadm $v03, $v03, $v30.e0                          ## L:890  |     27 | vLenInv >>= 9;
  vmadn $v04, $v00, $v00                             ## L:890  |     28 | vLenInv >>= 9;
  vmudl $v29, $v02, $v04.h3                          ## L:892  |  ***32 | v *= vLenInv.wwwwWWWW;
  vmadm $v29, $v01, $v04.h3                          ## L:892  |     33 | v *= vLenInv.wwwwWWWW;
  vmadn $v02, $v02, $v03.h3                          ## L:892  |     34 | v *= vLenInv.wwwwWWWW;
  vmadh $v01, $v01, $v03.h3                          ## L:892  |     35 | v *= vLenInv.wwwwWWWW;
  vadd $v02, $v02, $v02.v                            ## L:893  |   **38 | v:sfract += v:sfract;
  sdv $v02, 0, 0, $t1                                ## L:895  |  ***42 | store(v:sfract.xyzw, addrOut, 0x00);
  addiu $t1, $t1, 8                                  ## L:898  |     43 | addrOut += 0x8;
  jr $ra                                             ## L:899  |     44 | }
  addiu $s4, $s4, 16                                 ## L:897  |    *46 | addrIn += 0x10;
T3DCmd_MatrixStack:
  andi $s7, $a0, 1                                   ## L:915  |      ^ | u16 doMultiply = stackAdvance & 0b01;
  andi $s6, $a0, 2                                   ## L:916  |      2 | u16 onlyStackMove = stackAdvance & 0b10;
  lh $a0, %lo(RSPQ_DMEM_BUFFER  -7)($gp)             ## L:918  |      3 | stackAdvance:s16 = load_arg(1);
  lw $s5, %lo(MATRIX_STACK_PTR + 0)                  ## L:920  |      4 | s32 stackPtr = load(MATRIX_STACK_PTR);
  addu $s5, $s5, $a0                                 ## L:921  |    **7 | stackPtr += stackAdvance;
  bne $s6, $zero, RSPQ_Loop                          ## L:925  |      8 | if(onlyStackMove)exit;
  sw $s5, %lo(MATRIX_STACK_PTR)($zero)               ## L:922  |    *10 | store(stackPtr, MATRIX_STACK_PTR);
  srl $s3, $a1, 24                                   ## L:279  |     11 | u32 segment = addrIn >> 24;
  lw $s3, %lo(SEGMENT_TABLE)($s3)                    ## L:280  |     12 | segment = load(segment, SEGMENT_TABLE);
  addu $a1, $a1, $s3                                 ## L:281  |   **15 | addrOut = addrIn + segment;
  bgez $a0, LABEL_T3DCmd_MatrixStack_0011            ## L:930  |     16 | if(stackAdvance < 0)addressMat = stackPtr;
  ori $s4, $zero, %lo(MATRIX_MV)                     ## L:926  |    *18 | dmaDest = MATRIX_MV;
  or $a1, $zero, $s5                                 ## L:930  |     19 | if(stackAdvance < 0)addressMat = stackPtr;
  LABEL_T3DCmd_MatrixStack_0011:
  lui $at, 0xFF                                      ## L:931  |     20 | u32<$s0> addrRDRAM = addressMat & 0xFFFFFF;
  ori $at, $at, 0xFFFF                               ## L:931  |     21 | u32<$s0> addrRDRAM = addressMat & 0xFFFFFF;
  ori $t0, $zero, 63                                 ## L:933  |     22 | dma_in(dmaDest, addrRDRAM, 0x40);
  addiu $t2, $zero, 12                               ## L:933  |     23 | dma_in(dmaDest, addrRDRAM, 0x40);
  jal DMAExec                                        ## L:933  |     24 | dma_in(dmaDest, addrRDRAM, 0x40); ## Args: $t0, $t1, $s0, $s4, $t2
  and $s0, $a1, $at                                  ## L:931  |    *26 | u32<$s0> addrRDRAM = addressMat & 0xFFFFFF;
  beq $s7, $zero, LABEL_T3DCmd_MatrixStack_0012      ## L:940  |     27 | if(doMultiply) {
  ori $t1, $zero, %lo(MATRIX_NORMAL)                 ## L:936  |    *29 | u32<$t1> normOut = MATRIX_NORMAL;
  addiu $s0, $s5, 65472                              ## L:943  |     30 | addrRDRAM = stackPtr -  0x40;
  ori $t0, $zero, 63                                 ## L:944  |     31 | dma_in(dmaDest, addrRDRAM, 0x40);
  addiu $t2, $zero, 12                               ## L:944  |     32 | dma_in(dmaDest, addrRDRAM, 0x40);
  jal DMAExec                                        ## L:944  |     33 | dma_in(dmaDest, addrRDRAM, 0x40); ## Args: $t0, $t1, $s0, $s4, $t2
  ori $s4, $zero, %lo(CLIP_BUFFER_RESULT)            ## L:942  |    *35 | dmaDest = CLIP_BUFFER_RESULT;
  ori $s3, $zero, %lo(CLIP_BUFFER_RESULT)            ## L:948  |     36 | u32<$s3> mulLeft = CLIP_BUFFER_RESULT;
  ori $s4, $zero, %lo(MATRIX_MV)                     ## L:949  |     37 | dmaDest = MATRIX_MV;
  jal mulMat4Mat4                                    ## L:950  |     38 | mulMat4Mat4(mulDest, mulLeft, dmaDest); ## Args: $s2, $s3, $s4
  ori $s2, $zero, %lo(MATRIX_MV)                     ## L:947  |    *40 | u32<$s2> mulDest = MATRIX_MV;
  LABEL_T3DCmd_MatrixStack_0012:
  ori $t0, $zero, 63                                 ## L:955  |     41 | dma_out(dmaDest, addrRDRAM, 0x40);
  addiu $t2, $zero, -32756                           ## L:955  |     42 | dma_out(dmaDest, addrRDRAM, 0x40);
  jal DMAExec                                        ## L:955  |     43 | dma_out(dmaDest, addrRDRAM, 0x40); ## Args: $t0, $t1, $s0, $s4, $t2
  or $s0, $zero, $s5                                 ## L:954  |    *45 | addrRDRAM = stackPtr;
  jal normalizeMatrixVector                          ## L:961  |     46 | normalizeMatrixVector(dmaDest, normOut); ## Args: $s4, $t1
  ori $s2, $zero, %lo(MATRIX_MVP)                    ## L:959  |    *48 | u32<$s2> mulDest = MATRIX_MVP;
  jal normalizeMatrixVector                          ## L:965  |     49 | normalizeMatrixVector(dmaDest, normOut); ## Args: $s4, $t1
  ori $s3, $zero, %lo(MATRIX_PROJ)                   ## L:963  |    *51 | u32<$s3> mulLeft = MATRIX_PROJ;
  jal normalizeMatrixVector                          ## L:966  |     52 | normalizeMatrixVector(dmaDest, normOut); ## Args: $s4, $t1
  nop                                                ## L:966  |    *54 | normalizeMatrixVector(dmaDest, normOut);
  jal mulMat4Mat4                                    ## L:973  |     55 | mulMat4Mat4(mulDest, mulLeft, mulRight); ## Args: $s2, $s3, $s4
  ori $s4, $zero, %lo(MATRIX_MV)                     ## L:972  |    *57 | u32<$s4> mulRight = MATRIX_MV;
  j RSPQ_Loop                                        ## L:974  |     58 | }
  nop                                                ## L:974  |    *60 | }
T3DCmd_MatProjectionSet:
  ori $t0, $zero, 63                                 ## L:983  |      ^ | dma_in(MATRIX_PROJ, addrRDRAM, 0x40);
  ori $s4, $zero, %lo(MATRIX_PROJ)                   ## L:983  |      2 | dma_in(MATRIX_PROJ, addrRDRAM, 0x40);
  addiu $t2, $zero, 12                               ## L:983  |      3 | dma_in(MATRIX_PROJ, addrRDRAM, 0x40);
  j DMAExec                                          ## L:983  |      4 | dma_in(MATRIX_PROJ, addrRDRAM, 0x40); ## Args: $t0, $t1, $s0, $s4, $t2
  or $s0, $zero, $a0                                 ## L:982  |     *6 | u32<$s0> addrRDRAM = addressMat;
T3DCmd_SetWord:
  andi $s7, $a0, 32768                               ## L:993  |      ^ | u32 is16bit = addrDMEM & 0x8000;
  bne $s7, $zero, RSPQ_Loop                          ## L:996  |      2 | if(is16bit)goto RSPQ_Loop;
  sh $a1, ($a0)                                      ## L:994  |     *4 | store(value:u16, addrDMEM);
  j RSPQ_Loop                                        ## L:999  |      5 | }
  sw $a1, ($a0)                                      ## L:998  |     *7 | store(value:u32, addrDMEM);
T3DCmd_TriDraw_Cmd:
  j T3DCmd_TriDraw                                   ## L:1010 |      ^ | goto T3DCmd_TriDraw;
  nop                                                ## L:1010 |     *3 | goto T3DCmd_TriDraw;
  RDPQ_Triangle_Clip:
  sh $ra, %lo(CLIP_RA)($zero)                        ## L:1035 |      4 | store(RA:u16, CLIP_RA);
  jal RDPQ_Triangle_Send_End                         ## L:1036 |      5 | RDPQ_Triangle_Send_End();
  sh $s4, %lo(18 + RSPQ_SCRATCH_MEM)($zero)          ## L:1034 |     *7 | store(targetImem:u16, RSPQ_SCRATCH_MEM, 18);
  j CLIPPING_CODE_TARGET                             ## L:1042 |      8 | goto CLIPPING_CODE_TARGET;
  ori $s4, $zero, %lo(CLIPPING_CODE_TARGET)          ## L:1041 |    *10 | targetImem = CLIPPING_CODE_TARGET;
T3DCmd_TriDraw_Strip:
  LABEL_T3DCmd_TriDraw_Strip_0014:
  mfc0 $ra, COP0_DMA_BUSY                            ## L:1057 |      1 | RA = get_dma_busy();
  andi $t0, $a1, 65535                               ## L:1058 |      2 | copySize = countDmem & 0xFFFF;
  sra $s4, $a1, 16                                   ## L:1059 |      3 | prtDmem = countDmem >> 16;
  bne $ra, $zero, LABEL_T3DCmd_TriDraw_Strip_0014    ## L:1060 |      4 | prtDmemEnd = prtDmem + copySize;
  addu $s2, $s4, $t0                                 ## L:1060 |     *6 | prtDmemEnd = prtDmem + copySize;
  LABEL_T3DCmd_TriDraw_Strip_0015:
  mtc0 $s4, COP0_DMA_SPADDR                          ## L:236  |      7 | @Barrier("DMA") set_dma_addr_rsp(addrDMEM); ## Barrier: 0x4
  mtc0 $a0, COP0_DMA_RAMADDR                         ## L:237  |      8 | @Barrier("DMA") set_dma_addr_rdram(addrRDRAM); ## Barrier: 0x4
  mtc0 $t0, COP0_DMA_READ                            ## L:238  |   **11 | @Barrier("DMA") set_dma_read(size); ## Barrier: 0x4
  T3DCmd_TriDraw_Strip_PostDMA:
  addiu $s2, $s2, 65533                              ## L:1075 |     12 | prtDmemEnd -= 3;
  addiu $s4, $s4, 65534                              ## L:1081 |     13 | prtDmem -= 2;
  LABEL_T3DCmd_TriDraw_Strip_0016:
  mfc0 $v1, COP0_DMA_BUSY                            ## L:1084 |     14 | vertAddr = get_dma_busy();
  bne $v1, $zero, LABEL_T3DCmd_TriDraw_Strip_0016    ## L:1084 |   **17 | vertAddr = get_dma_busy();
  nop                                                ## L:1084 |    *19 | vertAddr = get_dma_busy();
  STRIP_LOOP_NEW:
  lbu $v0, %lo(FACE_CULLING + 0)                     ## L:1088 |     20 | faceCull = load(FACE_CULLING);
  addiu $s4, $s4, 2                                  ## L:1089 |     21 | prtDmem += 2;
  STRIP_LOOP_NEXT:
  beq $s4, $s2, STRIP_LOOP_END                       ## L:1092 |     22 | if(prtDmem == prtDmemEnd)goto STRIP_LOOP_END;
  xori $v0, $v0, 1                                   ## L:1091 |    *24 | faceCull ^= 1;
  lhu $a0, 2($s4)                                    ## L:1094 |     25 | idx0     = load(prtDmem, 2);
  lh $a1, 4($s4)                                     ## L:1095 |     26 | idx1:s16 = load(prtDmem, 4);
  lhu $a2, 0($s4)                                    ## L:1096 |     27 | idx2     = load(prtDmem, 0);
  bltz $a1, STRIP_LOOP_NEW                           ## L:1099 |    *29 | if(idx1:s16 < 0)goto STRIP_LOOP_NEW;
  addiu $s4, $s4, 2                                  ## L:1098 |    *31 | prtDmem += 2;
  j RDPQ_Triangle_Send_Async                         ## L:1103 |     32 | goto RDPQ_Triangle_Send_Async;
  ori $ra, $zero, %lo(STRIP_LOOP_NEXT)               ## L:1102 |    *34 | RA = STRIP_LOOP_NEXT;
  STRIP_LOOP_END:
  bgez $s2, RSPQ_Loop                                ## L:1111 |     35 | if(prtDmemEnd >= 0)goto RSPQ_Loop;
  ori $ra, $zero, %lo(RSPQ_Loop)                     ## L:1108 |    *37 | RA = RSPQ_Loop;
T3DCmd_TriSync:
  lbu $s7, %lo(CLIP_CODE_ADDR + 0)                   ## L:1121 |      ^ | u8 clipLoaded = load(CLIP_CODE_ADDR);
  beq $s7, $zero, RDPQ_Triangle_Send_End             ## L:1125 |    **4 | if(!clipLoaded)goto RDPQ_Triangle_Send_End;
  ori $s4, $zero, %lo(CLIPPING_CODE_TARGET)          ## L:1122 |     *6 | u32<$s4> targetImem = CLIPPING_CODE_TARGET;
  lhu $t0, %lo(CLIP_CODE_SIZE + 0)                   ## L:1128 |      7 | u16<$t0> codeSize = load(CLIP_CODE_SIZE);
  addiu $t0, $t0, -1                                 ## L:1131 |   **10 | dma_in_async(targetImem, codeAddr, codeSize);
  or $t2, $zero, $zero                               ## L:1131 |     11 | dma_in_async(targetImem, codeAddr, codeSize);
  jal DMAExec                                        ## L:1131 |     12 | dma_in_async(targetImem, codeAddr, codeSize); ## Args: $t0, $t1, $s0, $s4, $t2
  lw $s0, %lo(CLIP_CODE_ORG_ADDR + 0)                ## L:1127 |    *14 | u32<$s0> codeAddr = load(CLIP_CODE_ORG_ADDR);
  sb $zero, %lo(CLIP_CODE_ADDR)($zero)               ## L:1134 |    *16 | store(ZERO:u8, CLIP_CODE_ADDR);
  j RDPQ_Triangle_Send_End                           ## L:1135 |     17 | goto RDPQ_Triangle_Send_End;
  ori $ra, $zero, %lo(RSPQ_Loop)                     ## L:1133 |    *19 | RA = RSPQ_Loop;
T3DCmd_TriDraw_Seq:
  andi $s4, $a1, 65535                               ## L:1143 |      ^ | u8<$s4> baseVtxEnd = endIndex & 0xFFFF;
  andi $s2, $a0, 65535                               ## L:1144 |      2 | u16<$s2> baseVtx = sizeAndIndex & 0xFFFF;
  lbu $v0, %lo(FACE_CULLING + 0)                     ## L:1146 |      3 | u8<$v0> faceCull = load(FACE_CULLING);
  srl $s1, $a1, 16                                   ## L:1147 |      4 | u16<$s1> isQuad = endIndex >> 16;
  addiu $v1, $s1, 108                                ## L:1148 |      5 | u16<$v1> vtxIncr = isQuad + (36 * 3);
  SEQ_LOOP_NEXT:
  beq $s2, $s4, SEQ_LOOP_END                         ## L:1156 |      6 | if(baseVtx == baseVtxEnd)goto SEQ_LOOP_END;
  addiu $a0, $s2, 72                                 ## L:1155 |     *8 | idx0 = baseVtx + (36 * 2);
  addiu $a1, $s2, 36                                 ## L:1157 |      9 | idx1 = baseVtx + (36 * 1);
  or $a2, $zero, $s2                                 ## L:1158 |     10 | idx2 = baseVtx;
  ori $ra, $zero, %lo(SEQ_LOOP_NEXT)                 ## L:1161 |     11 | RA = SEQ_LOOP_NEXT;
  j RDPQ_Triangle_Send_Async                         ## L:1162 |     12 | goto RDPQ_Triangle_Send_Async;
  addu $s2, $s2, $v1                                 ## L:1159 |    *14 | baseVtx += vtxIncr;
  SEQ_LOOP_END:
  beq $s1, $zero, T3DCmd_TriSync                     ## L:1167 |     15 | if(isQuad) {
  ori $ra, $zero, %lo(RSPQ_Loop)                     ## L:1166 |    *17 | RA = RSPQ_Loop;
  xori $v0, $v0, 1                                   ## L:1169 |     18 | faceCull ^= 1;
  lhu $s2, %lo(RSPQ_DMEM_BUFFER  -6)($gp)            ## L:1170 |     19 | baseVtx:u16 = load_arg(2);
  addiu $s2, $s2, 36                                 ## L:1171 |   **22 | baseVtx += 36;
  addiu $s4, $s4, 36                                 ## L:1172 |     23 | baseVtxEnd += 36;
  j SEQ_LOOP_NEXT                                    ## L:1173 |     24 | goto SEQ_LOOP_NEXT;
  or $s1, $zero, $zero                               ## L:1168 |    *26 | isQuad = 0;
T3DCmd_Patch:
  or $s0, $zero, $a0                                 ## L:1181 |      ^ | u32<$s0> rdramAddr = rdramAddr_;
  ori $s4, $zero, %lo(RSPQ_SCRATCH_MEM)              ## L:1182 |      2 | s16<$s4> dmemAddr = RSPQ_SCRATCH_MEM;
  sw $a1, 0($s4)                                     ## L:1184 |      3 | store(opcode0, dmemAddr, 0);
  sw $a2, 4($s4)                                     ## L:1185 |      4 | store(opcode1, dmemAddr, 4);
  j DMAOut                                           ## L:1187 |      5 | DMAOut(size, rdramAddr, dmemAddr); ## Args: $t0, $s0, $s4
  addiu $t0, $zero, 7                                ## L:1180 |     *7 | u16<$t0> size = 8 - 1;

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