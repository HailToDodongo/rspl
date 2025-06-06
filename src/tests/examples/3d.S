## Auto-generated file, transpiled with RSPL
#define TRI_BUFFER_COUNT 70
#define LIGHT_COUNT 8
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
#define MATRIX_SIZE 0x40
#define DRAW_TRIS 1
#define DP_STATUS_END_VALID 512
#define RDPQ_DYNAMIC_BUFFER_SIZE 65536
#include <rsp_queue.inc>
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
    RSPQ_DefineCommand T3DCmd_TriDraw, 8
    RSPQ_DefineCommand T3DCmd_SetScreenSize, 16
    RSPQ_DefineCommand T3DCmd_MatrixStack, 8
    RSPQ_DefineCommand RSPQ_Loop, 4
    RSPQ_DefineCommand T3DCmd_VertLoad, 12
    RSPQ_DefineCommand T3DCmd_LightSet, 12
    RSPQ_DefineCommand T3DCmd_RenderMode, 8
    RSPQ_DefineCommand RSPQ_Loop, 4
    RSPQ_DefineCommand T3DCmd_MatProjectionSet, 4
    RSPQ_DefineCommand T3DCmd_LightCount, 4
    RSPQ_DefineCommand T3DCmd_SetFogRange, 8
    RSPQ_DefineCommand T3DCmd_SetFogState, 4
    RSPQ_DefineCommand RDPQ_Triangle_Send_End, 4
  RSPQ_EndOverlayHeader

  RSPQ_BeginSavedState
    STATE_MEM_START:
    .align 4
    MATRIX_PROJ: .ds.b 64
    .align 4
    MATRIX_MVP: .ds.b 64
    .align 4
    MATRIX_NORMAL: .ds.b 32
    TRI_BUFFER: .ds.b 2520
    .align 4
    CLIP_BUFFER_TMP: .ds.b 252
    .align 4
    CLIP_BUFFER_RESULT: .ds.b 288
    .align 4
    SCREEN_SCALE_OFFSET: .half 0, 0, 0, 0, 0, 0, 16383, 255
    .align 4
    NORMAL_MASK_SHIFT: .half 63488, 2016, 31, 0, 1, 32, 2048, 0
    .align 4
    CLIPPING_PLANES: .byte 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, -1, 0, 1, 0, -1
    .align 4
    NORM_SCALE_W: .half 65535, 65535, 65535, 0, 0, 0, 0, 65535
    .align 2
    COLOR_AMBIENT: .ds.b 8
    .align 2
    LIGHT_DIR_COLOR: .ds.b 128
    .align 2
    TRI_COMMAND: .word 0
    .align 2
    MATRIX_STACK_PTR: .word 0
    .align 1
    FOG_SCALE_OFFSET: .half 0, 0, 0, 32767
    .align 1
    CURRENT_MAT_ADDR: .half 0
    .align 1
    RDPQ_TRI_BUFF_OFFSET: .half 0
    USE_REJECT: .byte 0
    FACE_CULLING: .byte 0
    USE_FOG: .byte 0
    ACTIVE_LIGHT_SIZE: .byte 0
    STATE_MEM_END:
  RSPQ_EndSavedState

.text
OVERLAY_CODE_START:

T3DCmd_VertLoad:
  srl $s4, $a2, 16
  andi $t0, $a0, 65535
  addu $s7, $s4, $t0
  addiu $t0, $t0, -1
  or $t2, $zero, $zero
  jal DMAExec
  or $s0, $zero, $a1
  ori $s6, $zero, %lo(MATRIX_MVP)
  ldv $v27, 0, 0, $s6
  ldv $v27, 8, 0, $s6
  ldv $v28, 0, 8, $s6
  ldv $v28, 8, 8, $s6
  ldv $v25, 0, 16, $s6
  ldv $v25, 8, 16, $s6
  ldv $v26, 0, 24, $s6
  ldv $v26, 8, 24, $s6
  ldv $v23, 0, 32, $s6
  ldv $v23, 8, 32, $s6
  ldv $v24, 0, 40, $s6
  ldv $v24, 8, 40, $s6
  ldv $v21, 0, 48, $s6
  ldv $v21, 8, 48, $s6
  ldv $v22, 0, 56, $s6
  ldv $v22, 8, 56, $s6
  ori $s6, $zero, %lo(MATRIX_NORMAL)
  ldv $v20, 0, 0, $s6
  ldv $v20, 8, 0, $s6
  ldv $v19, 0, 8, $s6
  ldv $v19, 8, 8, $s6
  ldv $v18, 0, 16, $s6
  ldv $v18, 8, 16, $s6
  ori $at, $zero, %lo(NORMAL_MASK_SHIFT)
  ldv $v17, 0, 0, $at
  ldv $v17, 8, 0, $at
  ldv $v16, 0, 8, $at
  ldv $v16, 8, 8, $at
  ori $at, $zero, %lo(CLIPPING_PLANES)
  llv $v15, 0, 0, $at
  vor $v15, $v00, $v15.e1
  ori $at, $zero, %lo(SCREEN_SCALE_OFFSET)
  ldv $v13, 0, 0, $at
  ldv $v13, 8, 0, $at
  vmudl $v14, $v00, $v31.e3
  vmadm $v13, $v13, $v31.e3
  ldv $v12, 0, 8, $at
  ldv $v12, 8, 8, $at
  ori $at, $zero, %lo(FOG_SCALE_OFFSET)
  ldv $v11, 0, 0, $at
  ori $at, $zero, %lo(NORM_SCALE_W)
  ldv $v10, 0, 0, $at
  ldv $v10, 8, 0, $at
  andi $s6, $a2, 65535
  addiu $s5, $s6, 36
  ori $s3, $zero, %lo(LIGHT_DIR_COLOR)
  lbu $s2, %lo(ACTIVE_LIGHT_SIZE + 0)
  addu $s2, $s2, $s3
  jal DMAWaitIdle
  vmadn $v14, $v00, $v00
  LABEL_T3DCmd_VertLoad_0001:
  lqv $v06, 0, 0, $s4
  vand $v05, $v17, $v06.h3
  vmudn $v05, $v05, $v16.v
  vmulf $v09, $v20, $v05.h0
  vmacf $v09, $v19, $v05.h1
  vmacf $v05, $v18, $v05.h2
  ori $at, $zero, %lo(CLIPPING_PLANES)
  lsv $v06, 6, 4, $at
  lsv $v06, 14, 4, $at
  vmudn $v09, $v28, $v06.h0
  vmadh $v08, $v27, $v06.h0
  vmadn $v09, $v26, $v06.h1
  vmadh $v08, $v25, $v06.h1
  vmadn $v09, $v24, $v06.h2
  vmadh $v08, $v23, $v06.h2
  vmadn $v09, $v22, $v06.h3
  vmadh $v08, $v21, $v06.h3
  luv $v07, 0, 16, $s4
  ori $at, $zero, %lo(COLOR_AMBIENT)
  luv $v06, 0, 0, $at
  LABEL_T3DCmd_VertLoad_0003:
  beq $s3, $s2, LABEL_T3DCmd_VertLoad_0004
  nop
  lpv $v04, 0, 8, $s3
  vmulf $v03, $v05, $v04.v
  vadd $v02, $v03, $v03.h1
  vadd $v03, $v03, $v02.h2
  luv $v02, 0, 0, $s3
  vmulu $v03, $v02, $v03.h0
  addiu $s3, $s3, 16
  j LABEL_T3DCmd_VertLoad_0003
  vadd $v06, $v06, $v03.v
  LABEL_T3DCmd_VertLoad_0004:
  vmulf $v07, $v07, $v06.v
  vmudm $v03, $v15, $v09.h3
  vmadh $v02, $v15, $v08.h3
  vmadn $v03, $v00, $v00
  vch $v29, $v08, $v02
  vcl $v29, $v09, $v03
  cfc2 $k1, $vcc
  srl $s0, $k1, 4
  andi $s1, $k1, 1799
  srl $k0, $s1, 5
  or $s1, $s1, $k0
  andi $s0, $s0, 1799
  srl $k0, $s0, 5
  or $s0, $s0, $k0
  vch $v29, $v08, $v08.h3
  vcl $v29, $v09, $v09.h3
  cfc2 $k0, $vcc
  srl $sp, $k0, 4
  andi $fp, $k0, 1799
  srl $t9, $fp, 5
  nor $fp, $fp, $t9
  andi $sp, $sp, 1799
  srl $t9, $sp, 5
  nor $sp, $sp, $t9
  vmudl $v09, $v09, $v10.v
  vmadm $v08, $v08, $v10.v
  vmadn $v09, $v00, $v00
  sdv $v08, 0, 16, $s6
  sdv $v09, 0, 24, $s6
  sdv $v08, 8, 16, $s5
  sdv $v09, 8, 24, $s5
  vrcph $v08.e3, $v08.e3
  vrcpl $v09.e3, $v09.e3
  vrcph $v08.e3, $v08.e7
  vrcpl $v09.e7, $v09.e7
  vrcph $v08.e7, $v00.e7
  lbu $k1, %lo(USE_FOG + 0)
  vaddc $v06, $v09, $v11.e2
  vadd $v05, $v08, $v11.e1
  vmudn $v06, $v06, $v11.e0
  vmadh $v05, $v05, $v11.e0
  vor $v04, $v00, $v11.e3
  vsub $v05, $v04, $v05.v
  beq $k1, $zero, LABEL_T3DCmd_VertLoad_0005
  ori $s3, $zero, %lo(LIGHT_DIR_COLOR)
  vmov $v07.e3, $v05.e2
  vmov $v07.e7, $v05.e6
  LABEL_T3DCmd_VertLoad_0005:
  ssv $v08, 6, 32, $s6
  ssv $v09, 6, 34, $s6
  ssv $v08, 14, 32, $s5
  ssv $v09, 14, 34, $s5
  vmudl $v29, $v09, $v09.h3
  vmadm $v29, $v08, $v09.h3
  vmadn $v09, $v09, $v08.h3
  vmadh $v08, $v08, $v08.h3
  vmudl $v29, $v09, $v14.v
  vmadm $v29, $v08, $v14.v
  vmadn $v09, $v09, $v13.v
  vmadh $v08, $v08, $v13.v
  vadd $v08, $v08, $v12.v
  sdv $v08, 0, 0, $s6
  sdv $v08, 8, 0, $s5
  suv $v07, 0, 8, $s6
  suv $v07, 4, 8, $s5
  ldv $v09, 0, 24, $s4
  slv $v09, 0, 12, $s6
  slv $v09, 4, 12, $s5
  addiu $s4, $s4, 32
  sb $s1, 6($s6)
  sb $s0, 6($s5)
  sb $fp, 7($s6)
  sb $sp, 7($s5)
  addiu $s5, $s5, 72
  bne $s4, $s7, LABEL_T3DCmd_VertLoad_0001
  addiu $s6, $s6, 72
  LABEL_T3DCmd_VertLoad_0002:
  j RSPQ_Loop
  nop
copyClippingVertex:
  ldv $v01, 0, 0, $s0
  ldv $v02, 0, 8, $s0
  ldv $v03, 0, 16, $s0
  ldv $v04, 0, 24, $s0
  sdv $v01, 0, 0, $s5
  sdv $v02, 0, 8, $s5
  sdv $v03, 0, 16, $s5
  sdv $v04, 0, 24, $s5
  jr $ra
  addiu $s5, $s5, 36
emitClippedTri:
  suv $v28, 0, 8, $s5
  slv $v26, 8, 12, $s5
  sb $s1, 6($s5)
  sdv $v26, 0, 16, $s5
  sdv $v27, 0, 24, $s5
  jr $ra
  addiu $s5, $s5, 36
triToScreenSpace:
  ori $at, $zero, %lo(SCREEN_SCALE_OFFSET)
  ldv $v01, 0, 0, $at
  ldv $v02, 0, 8, $at
  vmudl $v02, $v00, $v31.e3
  vmadm $v01, $v01, $v31.e3
  vmadn $v02, $v00, $v00
  ldv $v03, 0, 8, $at
  ldv $v04, 0, 16, $s6
  ldv $v05, 0, 24, $s6
  vrcph $v04.e3, $v04.e3
  vrcpl $v05.e3, $v05.e3
  vrcph $v04.e3, $v00.e3
  ssv $v04, 6, 32, $s6
  ssv $v05, 6, 34, $s6
  vmudl $v29, $v05, $v05.e3
  vmadm $v29, $v04, $v05.e3
  vmadn $v05, $v05, $v04.e3
  vmadh $v04, $v04, $v04.e3
  vmudl $v29, $v05, $v02.v
  vmadm $v29, $v04, $v02.v
  vmadn $v05, $v05, $v01.v
  vmadh $v04, $v04, $v01.v
  vadd $v04, $v04, $v03.v
  sdv $v04, 0, 0, $s6
  ssv $v03, 6, 6, $s6
  jr $ra
  addiu $s6, $s6, 65500
T3DCmd_TriDraw:
  addiu $v1, $gp, %lo(RSPQ_DMEM_BUFFER)  -6
  ori $sp, $zero, %lo(RSPQ_Loop)
  jal RDPQ_Triangle_Send_Async
  srl $a2, $a1, 16
  bne $v1, $zero, LABEL_T3DCmd_TriDraw_0006
  lhu $s7, %lo(RDPQ_TRI_BUFF_OFFSET + 0)
  xori $s7, $s7, 176
  j RSPQ_Loop
  sh $s7, %lo(RDPQ_TRI_BUFF_OFFSET)($zero)
  LABEL_T3DCmd_TriDraw_0006:
  lhu $a1, %lo(RSPQ_DMEM_BUFFER  -4)($gp)
  jal RDPQ_Triangle_Send_End
  lhu $a2, %lo(RSPQ_DMEM_BUFFER  -2)($gp)
  lhu $a0, %lo(RSPQ_DMEM_BUFFER  -6)($gp)
  ori $s7, $zero, %lo(CLIP_BUFFER_RESULT)
  ori $s6, $zero, %lo(CLIP_BUFFER_TMP)
  ori $s5, $zero, %lo(CLIP_BUFFER_TMP)
  or $s0, $zero, $a0
  jal copyClippingVertex
  addiu $t0, $zero, 108
  jal copyClippingVertex
  or $s0, $zero, $a1
  jal copyClippingVertex
  or $s0, $zero, $a2
  ori $s4, $zero, %lo(CLIPPING_PLANES)
  addiu $s3, $zero, 1
  LABEL_T3DCmd_TriDraw_0007:
  addiu $s2, $s5, 65500
  luv $v28, 0, 8, $s2
  ldv $v26, 0, 16, $s2
  ldv $v27, 0, 24, $s2
  llv $v26, 8, 12, $s2
  lbu $s1, 6($s2)
  xor $s7, $s7, $s6
  xor $s6, $s7, $s6
  xor $s7, $s7, $s6
  or $s5, $zero, $s6
  or $s2, $zero, $s7
  addu $s0, $s7, $t0
  LABEL_T3DCmd_TriDraw_0009:
  ldv $v24, 0, 16, $s2
  llv $v24, 8, 12, $s2
  luv $v23, 0, 8, $s2
  lbu $fp, 6($s2)
  xor $sp, $fp, $s1
  nor $k1, $zero, $fp
  and $sp, $sp, $s3
  and $k1, $k1, $s3
  beq $sp, $zero, LABEL_T3DCmd_TriDraw_000B
  ldv $v25, 0, 24, $s2
  ori $at, $zero, %lo(NORM_SCALE_W)
  ldv $v22, 0, 8, $at
  ldv $v22, 8, 8, $at
  vsubc $v21, $v23, $v28.v
  vsubc $v20, $v25, $v27.v
  vsub $v19, $v24, $v26.v
  lpv $v17, 0, 0, $s4
  vmudl $v18, $v00, $v31.e7
  vmadm $v17, $v17, $v31.e7
  vmadn $v18, $v00, $v00
  ori $at, $zero, %lo(RSPQ_SCRATCH_MEM)
  sdv $v17, 0, 0, $at
  ldv $v17, 8, 0, $at
  vmudl $v18, $v18, $v22.v
  vmadm $v17, $v17, $v22.v
  vmadn $v18, $v00, $v00
  vor $v15, $v00, $v26
  vor $v16, $v00, $v27
  sdv $v24, 0, 0, $at
  sdv $v25, 0, 8, $at
  ldv $v15, 8, 0, $at
  ldv $v16, 8, 8, $at
  vmudl $v29, $v16, $v18.v
  vmadm $v29, $v15, $v18.v
  vmadn $v14, $v16, $v17.v
  vmadh $v13, $v15, $v17.v
  vaddc $v14, $v14, $v14.q1
  vadd $v13, $v13, $v13.q1
  vaddc $v14, $v14, $v14.h2
  vadd $v13, $v13, $v13.h2
  vsubc $v12, $v14, $v14.e4
  vsub $v11, $v13, $v13.e4
  vrcph $v11.e0, $v11.e0
  vrcpl $v12.e0, $v12.e0
  vrcph $v11.e0, $v00.e0
  vaddc $v12, $v12, $v12.v
  vadd $v11, $v11, $v11.v
  vmudl $v29, $v12, $v14.e0
  vmadm $v29, $v11, $v14.e0
  vmadn $v12, $v12, $v13.e0
  vmadh $v11, $v11, $v13.e0
  vlt $v29, $v11, $v30.e7
  vmrg $v12, $v12, $v22.e3
  vge $v29, $v11, $v00.e0
  vmrg $v12, $v12, $v00
  vmudl $v14, $v20, $v12.e0
  vmadm $v13, $v19, $v12.e0
  vmadn $v14, $v00, $v00
  vmadn $v27, $v27, $v30.e7
  vmadh $v26, $v26, $v30.e7
  vmudm $v21, $v21, $v12.e0
  vaddc $v28, $v28, $v21.v
  vmudl $v21, $v27, $v22.v
  vmadm $v20, $v26, $v22.v
  vmadn $v21, $v00, $v00
  vch $v29, $v20, $v26.e3
  vcl $v29, $v21, $v27.e3
  cfc2 $s1, $vcc
  andi $s1, $s1, 1799
  srl $k0, $s1, 5
  jal emitClippedTri
  or $s1, $s1, $k0
  LABEL_T3DCmd_TriDraw_000B:
  vor $v27, $v00, $v25
  or $s1, $zero, $fp
  vor $v28, $v00, $v23
  ori $ra, $zero, LABEL_T3DCmd_TriDraw_000C
  bne $k1, $zero, emitClippedTri
  vor $v26, $v00, $v24
  LABEL_T3DCmd_TriDraw_000C:
  addiu $s2, $s2, 36
  bne $s2, $s0, LABEL_T3DCmd_TriDraw_0009
  nop
  LABEL_T3DCmd_TriDraw_000A:
  subu $t0, $s5, $s6
  beq $t0, $zero, RSPQ_Loop
  sll $s3, $s3, 1
  addiu $at, $zero, 32
  bne $s3, $at, LABEL_T3DCmd_TriDraw_0007
  addiu $s4, $s4, 4
  CLIP_END:
  ori $s6, $zero, %lo(CLIP_BUFFER_RESULT)
  addu $s5, $s6, $t0
  sh $s6, %lo(0 + RSPQ_SCRATCH_MEM)($zero)
  jal triToScreenSpace
  addiu $s5, $s5, 65464
  jal triToScreenSpace
  addiu $s6, $s6, 72
  LABEL_T3DCmd_TriDraw_000E:
  jal triToScreenSpace
  addiu $s6, $s6, 72
  or $a2, $zero, $s6
  addiu $a1, $s6, 36
  ori $v1, $zero, %lo(RSPQ_SCRATCH_MEM)
  sh $a2, %lo(2 + RSPQ_SCRATCH_MEM)($zero)
  sh $a1, %lo(4 + RSPQ_SCRATCH_MEM)($zero)
  ori $sp, $zero, %lo(CLIP_AFTER_EMIT)
  jal RDPQ_Triangle_Send_Async
  ori $a0, $zero, %lo(CLIP_BUFFER_RESULT)
  jal RDPQ_Triangle_Send_End
  nop
  CLIP_AFTER_EMIT:
  bne $s6, $s5, LABEL_T3DCmd_TriDraw_000E
  nop
  LABEL_T3DCmd_TriDraw_000F:
  j RSPQ_Loop
  nop
T3DCmd_SetScreenSize:
  sw $a2, %lo(0 + SCREEN_SCALE_OFFSET)($zero)
  sw $a1, %lo(8 + SCREEN_SCALE_OFFSET)($zero)
  sh $a3, %lo(6 + NORM_SCALE_W)($zero)
  sh $a3, %lo(8 + NORM_SCALE_W)($zero)
  sh $a3, %lo(10 + NORM_SCALE_W)($zero)
  sh $a3, %lo(12 + NORM_SCALE_W)($zero)
  andi $a0, $a0, 15
  subu $s7, $zero, $a0
  sb $a0, %lo(3 + CLIPPING_PLANES)($zero)
  sb $a0, %lo(7 + CLIPPING_PLANES)($zero)
  sb $s7, %lo(15 + CLIPPING_PLANES)($zero)
  sb $s7, %lo(19 + CLIPPING_PLANES)($zero)
  j RSPQ_Loop
  sw $a3, %lo(4 + SCREEN_SCALE_OFFSET)($zero)
T3DCmd_SetFogRange:
  sw $a1, %lo(2 + FOG_SCALE_OFFSET)($zero)
  j RSPQ_Loop
  sh $a0, %lo(0 + FOG_SCALE_OFFSET)($zero)
T3DCmd_SetFogState:
  j RSPQ_Loop
  sb $a0, %lo(USE_FOG)($zero)
T3DCmd_LightSet:
  beq $a2, $zero, LABEL_T3DCmd_LightSet_0010
  sw $a1, 0($a0)
  sw $a2, 8($a0)
  sw $a2, 12($a0)
  LABEL_T3DCmd_LightSet_0010:
  j RSPQ_Loop
  sw $a1, 4($a0)
T3DCmd_LightCount:
  j RSPQ_Loop
  sb $a0, %lo(ACTIVE_LIGHT_SIZE)($zero)
T3DCmd_RenderMode:
  lbu $s7, %lo(RDPQ_OTHER_MODES + 0)
  andi $s7, $s7, 56
  or $a1, $a1, $s7
  sw $a1, %lo(TRI_COMMAND)($zero)
  j RSPQ_Loop
  sb $a0, %lo(FACE_CULLING)($zero)
mulMat4Mat4:
  ldv $v01, 0, 0, $s3
  ldv $v01, 8, 0, $s3
  ldv $v02, 0, 8, $s3
  ldv $v02, 8, 8, $s3
  ldv $v03, 0, 16, $s3
  ldv $v03, 8, 16, $s3
  ldv $v04, 0, 24, $s3
  ldv $v04, 8, 24, $s3
  ldv $v05, 0, 32, $s3
  ldv $v05, 8, 32, $s3
  ldv $v06, 0, 40, $s3
  ldv $v06, 8, 40, $s3
  ldv $v07, 0, 48, $s3
  ldv $v07, 8, 48, $s3
  ldv $v08, 0, 56, $s3
  ldv $v08, 8, 56, $s3
  ldv $v09, 0, 0, $s4
  ldv $v10, 0, 8, $s4
  ldv $v09, 8, 16, $s4
  ldv $v10, 8, 24, $s4
  ldv $v11, 0, 32, $s4
  ldv $v12, 0, 40, $s4
  ldv $v11, 8, 48, $s4
  ldv $v12, 8, 56, $s4
  vmudl $v29, $v02, $v10.h0
  vmadm $v29, $v01, $v10.h0
  vmadn $v14, $v02, $v09.h0
  vmadh $v13, $v01, $v09.h0
  vmadl $v29, $v04, $v10.h1
  vmadm $v29, $v03, $v10.h1
  vmadn $v14, $v04, $v09.h1
  vmadh $v13, $v03, $v09.h1
  vmadl $v29, $v06, $v10.h2
  vmadm $v29, $v05, $v10.h2
  vmadn $v14, $v06, $v09.h2
  vmadh $v13, $v05, $v09.h2
  vmadl $v29, $v08, $v10.h3
  vmadm $v29, $v07, $v10.h3
  vmadn $v10, $v08, $v09.h3
  vmadh $v09, $v07, $v09.h3
  vmudl $v29, $v02, $v12.h0
  vmadm $v29, $v01, $v12.h0
  vmadn $v14, $v02, $v11.h0
  vmadh $v13, $v01, $v11.h0
  vmadl $v29, $v04, $v12.h1
  vmadm $v29, $v03, $v12.h1
  vmadn $v14, $v04, $v11.h1
  vmadh $v13, $v03, $v11.h1
  vmadl $v29, $v06, $v12.h2
  vmadm $v29, $v05, $v12.h2
  vmadn $v14, $v06, $v11.h2
  vmadh $v13, $v05, $v11.h2
  vmadl $v29, $v08, $v12.h3
  vmadm $v29, $v07, $v12.h3
  vmadn $v12, $v08, $v11.h3
  vmadh $v11, $v07, $v11.h3
  sdv $v10, 0, 8, $s2
  sdv $v09, 8, 16, $s2
  sdv $v10, 8, 24, $s2
  sdv $v11, 0, 32, $s2
  sdv $v12, 0, 40, $s2
  sdv $v11, 8, 48, $s2
  sdv $v12, 8, 56, $s2
  jr $ra
  sdv $v09, 0, 0, $s2
normalizeMatrixVector:
  ldv $v01, 0, 0, $s4
  ldv $v02, 0, 8, $s4
  vmudl $v29, $v02, $v02.v
  vmadm $v29, $v01, $v02.v
  vmadn $v04, $v02, $v01.v
  vmadh $v03, $v01, $v01.v
  vaddc $v06, $v04, $v04.h1
  vadd $v05, $v03, $v03.h1
  vaddc $v04, $v04, $v06.h2
  vadd $v03, $v03, $v05.h2
  vrsqh $v03.e3, $v03.e0
  vrsql $v04.e3, $v04.e0
  vrsqh $v03.e3, $v00.e0
  vmudl $v04, $v04, $v30.e0
  vmadm $v03, $v03, $v30.e0
  vmadn $v04, $v00, $v00
  vmudl $v29, $v02, $v04.h3
  vmadm $v29, $v01, $v04.h3
  vmadn $v02, $v02, $v03.h3
  vmadh $v01, $v01, $v03.h3
  vadd $v02, $v02, $v02.v
  sdv $v02, 0, 0, $t1
  addiu $t1, $t1, 8
  jr $ra
  addiu $s4, $s4, 16
T3DCmd_MatrixStack:
  andi $s7, $a1, 1
  andi $s6, $a1, 2
  sra $a1, $a1, 16
  lw $s5, %lo(MATRIX_STACK_PTR + 0)
  addu $s5, $s5, $a1
  bne $s6, $zero, RSPQ_Loop
  sw $s5, %lo(MATRIX_STACK_PTR)($zero)
  bgez $a1, LABEL_T3DCmd_MatrixStack_0012
  ori $s4, $zero, %lo(CLIP_BUFFER_TMP)
  or $a0, $zero, $s5
  LABEL_T3DCmd_MatrixStack_0012:
  lui $at, 0xFF
  ori $at, $at, 0xFFFF
  ori $t0, $zero, 63
  addiu $t2, $zero, 12
  jal DMAExec
  and $s0, $a0, $at
  beq $s7, $zero, LABEL_T3DCmd_MatrixStack_0013
  ori $t1, $zero, %lo(MATRIX_NORMAL)
  addiu $s0, $s5, 65472
  ori $t0, $zero, 63
  addiu $t2, $zero, 12
  jal DMAExec
  ori $s4, $zero, %lo(CLIP_BUFFER_RESULT)
  ori $s3, $zero, %lo(CLIP_BUFFER_RESULT)
  ori $s4, $zero, %lo(CLIP_BUFFER_TMP)
  jal mulMat4Mat4
  ori $s2, $zero, %lo(CLIP_BUFFER_TMP)
  LABEL_T3DCmd_MatrixStack_0013:
  ori $t0, $zero, 63
  addiu $t2, $zero, -32756
  jal DMAExec
  or $s0, $zero, $s5
  jal normalizeMatrixVector
  ori $s2, $zero, %lo(MATRIX_MVP)
  jal normalizeMatrixVector
  ori $s3, $zero, %lo(MATRIX_PROJ)
  jal normalizeMatrixVector
  nop
  jal mulMat4Mat4
  ori $s4, $zero, %lo(CLIP_BUFFER_TMP)
  j RSPQ_Loop
  nop
T3DCmd_MatProjectionSet:
  lui $at, 0xFF
  ori $at, $at, 0xFFFF
  ori $t0, $zero, 63
  ori $s4, $zero, %lo(MATRIX_PROJ)
  addiu $t2, $zero, 12
  j DMAExec
  and $s0, $a0, $at

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
#include "./rspq_triangle.inc"