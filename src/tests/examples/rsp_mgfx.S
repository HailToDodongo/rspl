## Auto-generated file, transpiled with RSPL
#define MG_VTX_XYZ 0
#define MG_VTX_CLIP_CODE 6
#define MG_VTX_TR_CODE 7
#define MG_VTX_RGBA 8
#define MG_VTX_ST 12
#define MG_VTX_Wi 16
#define MG_VTX_Wf 18
#define MG_VTX_INVWi 20
#define MG_VTX_INVWf 22
#define MG_VTX_CS_POSi 24
#define MG_VTX_CS_POSf 32
#define MG_VTX_SIZE 40
#define MG_VTX_SIZE2 80
#define MGFX_ATTRIBUTE_POSITION 0
#define MGFX_ATTRIBUTE_NORMAL 1
#define MGFX_ATTRIBUTE_COLOR 2
#define MGFX_ATTRIBUTE_TEXCOORD 3
#define MGFX_BINDING_MATRICES 0
#define MGFX_BINDING_TEXTURING 1
#define MGFX_BINDING_LIGHTING 2
#define MGFX_BINDING_FOG 3
#define MGFX_LIGHT_COUNT_MAX 8
#define MGFX_LIGHT_POSITION 0
#define MGFX_LIGHT_POSITIONW 6
#define MGFX_LIGHT_COLOR 8
#define MGFX_LIGHT_INTENSITY 14
#define MGFX_LIGHT_SIZE 16
#define MGFX_MATRIX_SIZE 64
#define MGFX_VTX_TEX_SHIFT 5
#define SCRATCH_TEXSCALE 0x80
#include <rsp_magma.inc>

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

MgBeginShaderUniforms
  MgBeginUniform MATRICES, 0
    .align 4
    MATRICES_MVP: .ds.b 64
    .align 4
    MATRICES_MV: .ds.b 64
    .align 4
    MATRICES_NORMAL: .ds.b 32
  MgEndUniform

  MgBeginUniform TEXTURING, 1
    .align 1
    TEXTURING_SCALE: .ds.b 4
    .align 1
    TEXTURING_OFFSET: .ds.b 4
  MgEndUniform

  MgBeginUniform LIGHTING, 2
    LIGHTING_LIGHTS: .ds.b 128
    .align 1
    LIGHTING_AMBIENT: .ds.b 6
    LIGHTING_HAS_POINTS: .ds.b 1
    LIGHTING_COUNT: .ds.b 1
  MgEndUniform

  MgBeginUniform FOG, 3
    .align 1
    FOG_PARAMS: .ds.b 6
    FOG_MASK: .ds.b 1
  MgEndUniform

MgEndShaderUniforms

MgBeginVertexInput
  MgBeginVertexAttribute 0, 0
    MgVertexAttributeLoaders LOAD_POSITION243, LOAD_POSITION244, LOAD_POSITION505, LOAD_POSITION506
  MgEndVertexAttribute

  MgBeginVertexAttribute 1, 1
    MgVertexAttributeLoaders LOAD_NORMAL241, LOAD_NORMAL242, LOAD_NORMAL503, LOAD_NORMAL504
  MgEndVertexAttribute

  MgBeginVertexAttribute 2, 1
    MgVertexAttributeLoaders LOAD_COLOR245, LOAD_COLOR246, LOAD_COLOR507, LOAD_COLOR508
    MgBeginVertexAttributePatch PATCH_COLOR432
      vnop
    MgEndVertexAttributePatch
  MgEndVertexAttribute

  MgBeginVertexAttribute 3, 1
    MgVertexAttributeLoaders LOAD_TEXCOORD270, LOAD_TEXCOORD273
  MgEndVertexAttribute

MgEndVertexInput

MgBeginShader
  addiu $t0, $gp, %lo(RSPQ_DMEM_BUFFER)  -6          ## L:146  |      ^ | u32 inputs_ptr = get_cmd_address(0x2);
  ori $at, $zero, %lo(MG_VERTEX_SIZE)                ## L:147  |      2 | vec16 vvtx_size = load(MG_VERTEX_SIZE).xyzw;
  ldv $v01, 0, 0, $at                                ## L:147  |      3 | vec16 vvtx_size = load(MG_VERTEX_SIZE).xyzw;
  ldv $v02, 0, 0, $t0                                ## L:148  |      4 | vec16 vinputs = load(inputs_ptr).xyzw;
  vmudh $v04, $v02, $v01.v                           ## L:150  |   ***8 | vec32 vproducts = vinputs * vvtx_size;
  vsar $v03, COP2_ACC_HI                             ## L:150  |      9 | vec32 vproducts = vinputs * vvtx_size;
  vsar $v04, COP2_ACC_MD                             ## L:150  |     10 | vec32 vproducts = vinputs * vvtx_size;
  ori $at, $zero, %lo(RSPQ_SCRATCH_MEM)              ## L:151  |      ^ | @Barrier("inputs0") store(vproducts:ufract.xyzw, RSPQ_SCRATCH_MEM); ## Barrier: 0x1
  sdv $v04, 0, 0, $at                                ## L:151  |  ***14 | @Barrier("inputs0") store(vproducts:ufract.xyzw, RSPQ_SCRATCH_MEM); ## Barrier: 0x1
  ssv $v03, 0, 8, $at                                ## L:152  |     15 | @Barrier("inputs1") store(vproducts.x, RSPQ_SCRATCH_MEM, 0x8); ## Barrier: 0x2
  ssv $v04, 0, 10, $at                               ## L:152  |     16 | @Barrier("inputs1") store(vproducts.x, RSPQ_SCRATCH_MEM, 0x8); ## Barrier: 0x2
  lhu $s7, %lo(RSPQ_SCRATCH_MEM + 2)                 ## L:158  |     17 | @Barrier("inputs0") vtx_out0_ptr = load(RSPQ_SCRATCH_MEM, 0x2); ## Barrier: 0x1
  lhu $t1, %lo(RSPQ_SCRATCH_MEM + 4)                 ## L:160  |     18 | @Barrier("inputs0") vertices_size = load(RSPQ_SCRATCH_MEM, 0x4); ## Barrier: 0x1
  lw $t2, %lo(RSPQ_SCRATCH_MEM + 8)                  ## L:162  |     19 | @Barrier("inputs1") buffer_offset = load(RSPQ_SCRATCH_MEM, 0x8); ## Barrier: 0x2
  lw $s0, %lo(MG_VERTEX_BUFFER + 0)                  ## L:164  |     20 | u32<$s0> vtx_buffer = load(MG_VERTEX_BUFFER);
  addu $s0, $s0, $t2                                 ## L:165  |   **23 | vtx_buffer += buffer_offset;
  andi $t3, $t2, 7                                   ## L:167  |     24 | u32 misalignment = buffer_offset & 0x7;
  addu $t0, $t1, $t3                                 ## L:168  |     25 | dma_size = vertices_size + misalignment;
  addiu $t0, $t0, 7                                  ## L:169  |     26 | dma_size += 0x7;
  andi $t0, $t0, 4088                                ## L:170  |     27 | dma_size &= 0xFF8;
  ori $s4, $zero, %lo(MG_VERTEX_CACHE_END)           ## L:172  |     28 | vtx_in0_ptr = MG_VERTEX_CACHE_END;
  subu $s4, $s4, $t0                                 ## L:173  |     29 | vtx_in0_ptr -= dma_size;
  addiu $t2, $zero, 12                               ## L:175  |     30 | dma_in(vtx_in0_ptr, vtx_buffer, dma_size);
  jal DMAExec                                        ## L:175  |     31 | dma_in(vtx_in0_ptr, vtx_buffer, dma_size); ## Args: $t0, $t1, $s0, $s4, $t2
  addiu $t0, $t0, -1                                 ## L:175  |    *33 | dma_in(vtx_in0_ptr, vtx_buffer, dma_size);
  lhu $t8, %lo(MG_VERTEX_SIZE + 0)                   ## L:177  |     34 | vtx_size = load(MG_VERTEX_SIZE);
  sll $t9, $t8, 1                                    ## L:178  |   **37 | vtx_size2 = vtx_size * 2;
  addu $s6, $s4, $t1                                 ## L:180  |     38 | vtx_in_end = vtx_in0_ptr + vertices_size;
  addiu $s7, $s7, %lo(MG_VERTEX_CACHE)               ## L:181  |     39 | vtx_out0_ptr += MG_VERTEX_CACHE;
  ori $t0, $zero, %lo(MATRICES_MVP)                  ## L:189  |     40 | u16 mvpaddr = MATRICES_MVP;
  ldv $v01, 0, 0, $t0                                ## L:127  |     41 | m0:sint   = load(address, 0x00).xyzwxyzw;
  ldv $v01, 8, 0, $t0                                ## L:127  |     42 | m0:sint   = load(address, 0x00).xyzwxyzw;
  ldv $v02, 0, 32, $t0                               ## L:128  |     43 | m0:ufract = load(address, 0x20).xyzwxyzw;
  ldv $v02, 8, 32, $t0                               ## L:128  |     44 | m0:ufract = load(address, 0x20).xyzwxyzw;
  ldv $v03, 0, 8, $t0                                ## L:129  |     45 | m1:sint   = load(address, 0x08).xyzwxyzw;
  ldv $v03, 8, 8, $t0                                ## L:129  |     46 | m1:sint   = load(address, 0x08).xyzwxyzw;
  ldv $v04, 0, 40, $t0                               ## L:130  |     47 | m1:ufract = load(address, 0x28).xyzwxyzw;
  ldv $v04, 8, 40, $t0                               ## L:130  |     48 | m1:ufract = load(address, 0x28).xyzwxyzw;
  ldv $v05, 0, 16, $t0                               ## L:131  |     49 | m2:sint   = load(address, 0x10).xyzwxyzw;
  ldv $v05, 8, 16, $t0                               ## L:131  |     50 | m2:sint   = load(address, 0x10).xyzwxyzw;
  ldv $v06, 0, 48, $t0                               ## L:132  |     51 | m2:ufract = load(address, 0x30).xyzwxyzw;
  ldv $v06, 8, 48, $t0                               ## L:132  |     52 | m2:ufract = load(address, 0x30).xyzwxyzw;
  ldv $v07, 0, 24, $t0                               ## L:133  |     53 | m3:sint   = load(address, 0x18).xyzwxyzw;
  ldv $v07, 8, 24, $t0                               ## L:133  |     54 | m3:sint   = load(address, 0x18).xyzwxyzw;
  ldv $v08, 0, 56, $t0                               ## L:134  |     55 | m3:ufract = load(address, 0x38).xyzwxyzw;
  ldv $v08, 8, 56, $t0                               ## L:134  |     56 | m3:ufract = load(address, 0x38).xyzwxyzw;
  ori $at, $zero, %lo(MATRICES_NORMAL)               ## L:194  |     57 | vmn0 = load(MATRICES_NORMAL, 0x00).xyzwxyzw;
  ldv $v09, 0, 0, $at                                ## L:194  |     58 | vmn0 = load(MATRICES_NORMAL, 0x00).xyzwxyzw;
  ldv $v09, 8, 0, $at                                ## L:194  |     59 | vmn0 = load(MATRICES_NORMAL, 0x00).xyzwxyzw;
  ldv $v10, 0, 8, $at                                ## L:195  |     60 | vmn1 = load(MATRICES_NORMAL, 0x08).xyzwxyzw;
  ldv $v10, 8, 8, $at                                ## L:195  |     61 | vmn1 = load(MATRICES_NORMAL, 0x08).xyzwxyzw;
  ldv $v11, 0, 16, $at                               ## L:196  |     62 | vmn2 = load(MATRICES_NORMAL, 0x10).xyzwxyzw;
  ldv $v11, 8, 16, $at                               ## L:196  |     63 | vmn2 = load(MATRICES_NORMAL, 0x10).xyzwxyzw;
  ori $at, $zero, %lo(MG_NORMAL_MASK)                ## L:198  |     64 | vec16 vnormmask = load(MG_NORMAL_MASK,0x0).xyzwxyzw;
  ldv $v12, 0, 0, $at                                ## L:198  |     65 | vec16 vnormmask = load(MG_NORMAL_MASK,0x0).xyzwxyzw;
  ldv $v12, 8, 0, $at                                ## L:198  |     66 | vec16 vnormmask = load(MG_NORMAL_MASK,0x0).xyzwxyzw;
  ldv $v13, 0, 8, $at                                ## L:199  |     67 | vec16 vnormfactor = load(MG_NORMAL_MASK,0x8).xyzwxyzw;
  ldv $v13, 8, 8, $at                                ## L:199  |     68 | vec16 vnormfactor = load(MG_NORMAL_MASK,0x8).xyzwxyzw;
  ori $at, $zero, %lo(TEXTURING_SCALE)               ## L:202  |     69 | vtexscale.xy = load(TEXTURING_SCALE).xy;
  llv $v14, 0, 0, $at                                ## L:202  |     70 | vtexscale.xy = load(TEXTURING_SCALE).xy;
  llv $v14, 8, 0, $at                                ## L:203  |     71 | vtexscale.XY = load(TEXTURING_SCALE).xy;
  ori $at, $zero, %lo(MG_VIEWPORT)                   ## L:212  |     72 | vec16 vviewscale = load(MG_VIEWPORT, 0).xyzwxyzw;
  ldv $v15, 0, 0, $at                                ## L:212  |     73 | vec16 vviewscale = load(MG_VIEWPORT, 0).xyzwxyzw;
  ldv $v15, 8, 0, $at                                ## L:212  |     74 | vec16 vviewscale = load(MG_VIEWPORT, 0).xyzwxyzw;
  ldv $v16, 0, 8, $at                                ## L:213  |     75 | vec16 vviewoff = load(MG_VIEWPORT, 8).xyzwxyzw;
  ldv $v16, 8, 8, $at                                ## L:213  |     76 | vec16 vviewoff = load(MG_VIEWPORT, 8).xyzwxyzw;
  ori $at, $zero, %lo(TEXTURING_OFFSET)              ## L:216  |     77 | vtexoffset.xy = load(TEXTURING_OFFSET).xy;
  llv $v17, 0, 0, $at                                ## L:216  |     78 | vtexoffset.xy = load(TEXTURING_OFFSET).xy;
  llv $v17, 8, 0, $at                                ## L:217  |     79 | vtexoffset.XY = load(TEXTURING_OFFSET).xy;
  lbu $t0, %lo(LIGHTING_HAS_POINTS + 0)              ## L:219  |     80 | u8 has_points = load(LIGHTING_HAS_POINTS);
  ori $at, $zero, %lo(FOG_PARAMS)                    ## L:223  |     81 | vfog_parms = load(FOG_PARAMS).xyzwxyzw;
  ldv $v18, 0, 0, $at                                ## L:223  |     82 | vfog_parms = load(FOG_PARAMS).xyzwxyzw;
  ldv $v18, 8, 0, $at                                ## L:223  |     83 | vfog_parms = load(FOG_PARAMS).xyzwxyzw;
  lbu $t1, %lo(FOG_MASK + 0)                         ## L:224  |     84 | u8 fog_mask = load(FOG_MASK);
  addu $t2, $s4, $t8                                 ## L:226  |     85 | u16 vtx_in1_ptr = vtx_in0_ptr + vtx_size;
  addiu $t3, $s7, 40                                 ## L:227  |     86 | u16 vtx_out1_ptr = vtx_out0_ptr + 40;
  ori $t4, $zero, %lo(MG_SCRATCH_MEM)                ## L:229  |     87 | u16 prev_out0 = MG_SCRATCH_MEM;
  ori $t5, $zero, %lo(MG_SCRATCH_MEM)                ## L:230  |     88 | u16 prev_out1 = MG_SCRATCH_MEM;
  LOAD_NORMAL241: lsv $v20, 0, 0, $s4                ## L:241  |     89 | @AttrLoader("NORMAL") vnorm.x = load(vtx_in0_ptr).x;
  LOAD_NORMAL242: lsv $v20, 8, 0, $t2                ## L:242  |     90 | @AttrLoader("NORMAL") vnorm.X = load(vtx_in1_ptr).x;
  LOAD_POSITION243: ldv $v19, 0, 0, $s4              ## L:243  |     91 | @AttrLoader("POSITION") vpos.xyzw = load(vtx_in0_ptr).xyzw;
  LOAD_POSITION244: ldv $v19, 8, 0, $t2              ## L:244  |     92 | @AttrLoader("POSITION") vpos.XYZW = load(vtx_in1_ptr).xyzw;
  LOAD_COLOR245: llv $v23, 0, 0, $s4                 ## L:245  |     93 | @AttrLoader("COLOR") vrgba_in.xy = load(vtx_in0_ptr).xy;
  LOAD_COLOR246: llv $v23, 4, 0, $t2                 ## L:246  |     94 | @AttrLoader("COLOR") vrgba_in.zw = load(vtx_in1_ptr).xy;
  VERTEX_LOOP:
  sdv $v24, 0, 0, $t4                                ## L:251  |   **97 | @Barrier("st_c0") @Barrier("st_tr0") store(vpos_clip:sint.xyzw, prev_out0, 0); ## Barrier: 0xC
  sdv $v24, 8, 0, $t5                                ## L:252  |     98 | @Barrier("st_c1") @Barrier("st_tr1") store(vpos_clip:sint.XYZW, prev_out1, 0); ## Barrier: 0x30
  sb $t6, 6($t4)                                     ## L:254  |     99 | @Barrier("st_c0") store(c0, prev_out0, 6); ## Barrier: 0x4
  sb $t7, 6($t5)                                     ## L:255  |    100 | @Barrier("st_c1") store(c1, prev_out1, 6); ## Barrier: 0x10
  sb $k0, 7($t4)                                     ## L:256  |    101 | @Barrier("st_tr0") store(tr0, prev_out0, 7); ## Barrier: 0x8
  sb $k1, 7($t5)                                     ## L:257  |    102 | @Barrier("st_tr1") store(tr1, prev_out1, 7); ## Barrier: 0x20
  suv $v22, 0, 8, $t4                                ## L:259  |    103 | @Barrier("st_tex0") store_vec_u8(vrgba.x, prev_out0, 8); ## Barrier: 0x40
  suv $v22, 4, 8, $t5                                ## L:260  |    104 | @Barrier("st_tex1") store_vec_u8(vrgba.X, prev_out1, 8); ## Barrier: 0x80
  slv $v21, 0, 12, $t4                               ## L:262  |    105 | @Barrier("st_tex0") store(vtex.xy, prev_out0, 12); ## Barrier: 0x40
  slv $v21, 8, 12, $t5                               ## L:263  |    106 | @Barrier("st_tex1") store(vtex.XY, prev_out1, 12); ## Barrier: 0x80
  ori $at, $zero, %lo(RSPQ_SCRATCH_MEM)              ## L:265  |    107 | @Barrier("unpack_col") store(vrgba_in.xyzw, RSPQ_SCRATCH_MEM); ## Barrier: 0x100
  sdv $v23, 0, 0, $at                                ## L:265  |    108 | @Barrier("unpack_col") store(vrgba_in.xyzw, RSPQ_SCRATCH_MEM); ## Barrier: 0x100
  luv $v23, 0, 0, $at                                ## L:266  |    109 | @Barrier("unpack_col") vrgba_in = load_vec_u8(RSPQ_SCRATCH_MEM); ## Barrier: 0x100
  LOAD_TEXCOORD270: llv $v21, 0, 0, $s4              ## L:270  |    110 | @AttrLoader("TEXCOORD") vtex.xy = load(vtx_in0_ptr).xy; ## Barrier: 0x7E00
  LOAD_TEXCOORD273: llv $v21, 8, 0, $t2              ## L:273  |    111 | @AttrLoader("TEXCOORD") vtex.XY = load(vtx_in1_ptr).xy; ## Barrier: 0x7E00
  vand $v20, $v12, $v20.h0                           ## L:277  |      ^ | vnorm = vnormmask & vnorm.xxxxXXXX;
  vmudn $v20, $v20, $v13.v                           ## L:278  | ***115 | vnorm *= vnormfactor;
  vmudn $v29, $v02, $v19.h0                          ## L:281  |    116 | VTEMP = vmvp0  * vpos.xxxxXXXX;
  vmadh $v29, $v01, $v19.h0                          ## L:281  |    117 | VTEMP = vmvp0  * vpos.xxxxXXXX;
  vmadn $v29, $v06, $v19.h2                          ## L:282  |    118 | VTEMP = vmvp2 +* vpos.zzzzZZZZ;
  vmadh $v29, $v05, $v19.h2                          ## L:282  |    119 | VTEMP = vmvp2 +* vpos.zzzzZZZZ;
  vmadn $v29, $v04, $v19.h1                          ## L:283  |    120 | VTEMP = vmvp1 +* vpos.yyyyYYYY;
  vmadh $v29, $v03, $v19.h1                          ## L:283  |    121 | VTEMP = vmvp1 +* vpos.yyyyYYYY;
  vmadn $v25, $v08, $v30.e7                          ## L:284  |    122 | vpos_clip = vmvp3 +* 1;
  vmadh $v24, $v07, $v30.e7                          ## L:284  |    123 | vpos_clip = vmvp3 +* 1;
  vmulf $v29, $v09, $v20.h0                          ## L:286  |    124 | VTEMP = vmn0:sfract  * vnorm:sfract.xxxxXXXX;
  vmacf $v29, $v10, $v20.h1                          ## L:287  |    125 | VTEMP = vmn1:sfract +* vnorm:sfract.yyyyYYYY;
  vmacf $v20, $v11, $v20.h2                          ## L:288  |    126 | vnorm = vmn2:sfract +* vnorm:sfract.zzzzZZZZ;
  ori $at, $zero, %lo(LIGHTING_AMBIENT)              ## L:290  |      ^ | vrgba = load(LIGHTING_AMBIENT).xyzwxyzw;
  ldv $v22, 0, 0, $at                                ## L:290  |    127 | vrgba = load(LIGHTING_AMBIENT).xyzwxyzw;
  ldv $v22, 8, 0, $at                                ## L:290  |    128 | vrgba = load(LIGHTING_AMBIENT).xyzwxyzw;
  ori $at, $zero, %lo(MG_CLIP_FACTORS)               ## L:309  |    129 | vec16 vclip_factors = load(MG_CLIP_FACTORS).xyzwxyzw;
  ldv $v26, 0, 0, $at                                ## L:309  |    130 | vec16 vclip_factors = load(MG_CLIP_FACTORS).xyzwxyzw;
  ldv $v26, 8, 0, $at                                ## L:309  |    131 | vec16 vclip_factors = load(MG_CLIP_FACTORS).xyzwxyzw;
  vmudn $v28, $v25, $v26.v                           ## L:310  | ***135 | vec32 vguard = vpos_clip * vclip_factors;
  vmadh $v27, $v24, $v26.v                           ## L:310  |    136 | vec32 vguard = vpos_clip * vclip_factors;
  vch $v29, $v27, $v27.h3                            ## L:311  | ***140 | u16 clip_codes = clip(vguard, vguard.wwwwWWWW);
  vcl $v29, $v28, $v28.h3                            ## L:311  |    141 | u16 clip_codes = clip(vguard, vguard.wwwwWWWW);
  cfc2 $fp, $vcc                                     ## L:311  |    142 | u16 clip_codes = clip(vguard, vguard.wwwwWWWW);
  srl $s0, $fp, 4                                    ## L:113  |  **145 | u16 codes2 = codes >> 4;
  andi $s1, $fp, 1799                                ## L:93   |    146 | t0 = codes & 0x707;
  srl $s2, $s1, 5                                    ## L:94   |    147 | t1 = t0 >> 5;
  or $t6, $s1, $s2                                   ## L:101  |    148 | result = t0 | t1;
  andi $s1, $s0, 1799                                ## L:93   |    149 | t0 = codes & 0x707;
  srl $s2, $s1, 5                                    ## L:94   |    150 | t1 = t0 >> 5;
  or $t7, $s1, $s2                                   ## L:101  |    151 | result = t0 | t1;
  vch $v29, $v24, $v24.h3                            ## L:315  |      ^ | u16 tr_codes = clip(vpos_clip, vpos_clip.wwwwWWWW);
  vcl $v29, $v25, $v25.h3                            ## L:315  |    152 | u16 tr_codes = clip(vpos_clip, vpos_clip.wwwwWWWW);
  cfc2 $s0, $vcc                                     ## L:315  |    153 | u16 tr_codes = clip(vpos_clip, vpos_clip.wwwwWWWW);
  srl $s1, $s0, 4                                    ## L:120  |  **156 | u16 codes2 = codes >> 4;
  andi $s2, $s0, 1799                                ## L:93   |    157 | t0 = codes & 0x707;
  srl $s3, $s2, 5                                    ## L:94   |    158 | t1 = t0 >> 5;
  nor $k0, $s2, $s3                                  ## L:108  |    159 | result = t0 ~| t1;
  andi $s2, $s1, 1799                                ## L:93   |    160 | t0 = codes & 0x707;
  srl $s3, $s2, 5                                    ## L:94   |    161 | t1 = t0 >> 5;
  nor $k1, $s2, $s3                                  ## L:108  |    162 | result = t0 ~| t1;
  sdv $v24, 0, 24, $s7                               ## L:319  |    163 | @Barrier("st_cp0") store(vpos_clip.xyzw, vtx_out0_ptr, 24); ## Barrier: 0x200
  sdv $v25, 0, 32, $s7                               ## L:319  |    164 | @Barrier("st_cp0") store(vpos_clip.xyzw, vtx_out0_ptr, 24); ## Barrier: 0x200
  sdv $v24, 8, 24, $t3                               ## L:320  |    165 | @Barrier("st_cp1") store(vpos_clip.XYZW, vtx_out1_ptr, 24); ## Barrier: 0x400
  sdv $v25, 8, 32, $t3                               ## L:320  |    166 | @Barrier("st_cp1") store(vpos_clip.XYZW, vtx_out1_ptr, 24); ## Barrier: 0x400
  vaddc $v27, $v25, $v18.h1                          ## L:324  |      ^ | vec16 vtmp:ufract = vpos_clip:ufract + vfog_parms:ufract.yyyyYYYY;
  vadd $v26, $v24, $v18.h0                           ## L:325  |    167 | vfog:sint = vpos_clip:sint + vfog_parms:sint.xxxxXXXX;
  vmudn $v29, $v27, $v18.h2                          ## L:327  |  **170 | VTEMP = vtmp:ufract * vfog_parms:sint.zzzzZZZZ;
  vmadh $v26, $v26, $v18.h2                          ## L:328  |    171 | vfog = vfog:sint +* vfog_parms:sint.zzzzZZZZ;
  vge $v26, $v00, $v26.h2                            ## L:329  | ***175 | vfog = max(VZERO, vfog.zzzzZZZZ);
  vmudl $v25, $v25, $v15.h3                          ## L:337  |    176 | vpos_clip *= vviewscale:sfract.wwwwWWWW;
  vmadm $v24, $v24, $v15.h3                          ## L:337  |    177 | vpos_clip *= vviewscale:sfract.wwwwWWWW;
  vmadn $v25, $v00, $v00                             ## L:337  | ***181 | vpos_clip *= vviewscale:sfract.wwwwWWWW;
  ssv $v24, 6, 16, $s7                               ## L:340  |      ^ | @Barrier("st_w0") store(vpos_clip.w, vtx_out0_ptr, 16); ## Barrier: 0x800
  ssv $v25, 6, 18, $s7                               ## L:340  | ***185 | @Barrier("st_w0") store(vpos_clip.w, vtx_out0_ptr, 16); ## Barrier: 0x800
  ssv $v24, 14, 16, $t3                              ## L:341  |    186 | @Barrier("st_w1") store(vpos_clip.W, vtx_out1_ptr, 16); ## Barrier: 0x1000
  ssv $v25, 14, 18, $t3                              ## L:341  |    187 | @Barrier("st_w1") store(vpos_clip.W, vtx_out1_ptr, 16); ## Barrier: 0x1000
  vrcph $v27.e3, $v24.e3                             ## L:344  |      ^ | vinvw.w = invert_half(vpos_clip).w;
  vrcpl $v28.e3, $v25.e3                             ## L:344  |    188 | vinvw.w = invert_half(vpos_clip).w;
  vrcph $v27.e3, $v24.e7                             ## L:345  |    189 | vinvw.W = invert_half(vpos_clip).W;
  vrcpl $v28.e7, $v25.e7                             ## L:345  |    190 | vinvw.W = invert_half(vpos_clip).W;
  vrcph $v27.e7, $v00.e7                             ## L:345  |    191 | vinvw.W = invert_half(vpos_clip).W;
  ssv $v27, 6, 20, $s7                               ## L:347  | ***195 | @Barrier("st_iw0") store(vinvw.w, vtx_out0_ptr, 20); ## Barrier: 0x2000
  ssv $v28, 6, 22, $s7                               ## L:347  |    196 | @Barrier("st_iw0") store(vinvw.w, vtx_out0_ptr, 20); ## Barrier: 0x2000
  ssv $v27, 14, 20, $t3                              ## L:348  |    197 | @Barrier("st_iw1") store(vinvw.W, vtx_out1_ptr, 20); ## Barrier: 0x4000
  ssv $v28, 14, 22, $t3                              ## L:348  |    198 | @Barrier("st_iw1") store(vinvw.W, vtx_out1_ptr, 20); ## Barrier: 0x4000
  vmudl $v29, $v25, $v28.h3                          ## L:350  |      ^ | vpos_clip *= vinvw.wwwwWWWW;
  vmadm $v29, $v24, $v28.h3                          ## L:350  |    199 | vpos_clip *= vinvw.wwwwWWWW;
  vmadn $v25, $v25, $v27.h3                          ## L:350  |    200 | vpos_clip *= vinvw.wwwwWWWW;
  vmadh $v24, $v24, $v27.h3                          ## L:350  |    201 | vpos_clip *= vinvw.wwwwWWWW;
  vmudh $v29, $v16, $v30.e7                          ## L:354  |    202 | VTEMP = vviewoff:sint * 1;
  vmadn $v25, $v25, $v15.v                           ## L:355  |   *204 | vpos_clip = vpos_clip +* vviewscale;
  vmadh $v24, $v24, $v15.v                           ## L:355  |    205 | vpos_clip = vpos_clip +* vviewscale;
  beq $t0, $zero, LABEL_mgfx_0001                    ## L:363  |      ^ | if (has_points != 0)
  lbu $sp, %lo(LIGHTING_COUNT + 0)                   ## L:291  |   *207 | u8 light_count = load(LIGHTING_COUNT);
  ori $fp, $zero, %lo(MATRICES_MV)                   ## L:366  |    208 | u16 addr = MATRICES_MV;
  ldv $v01, 0, 0, $fp                                ## L:127  |    209 | m0:sint   = load(address, 0x00).xyzwxyzw;
  ldv $v01, 8, 0, $fp                                ## L:127  |    210 | m0:sint   = load(address, 0x00).xyzwxyzw;
  ldv $v02, 0, 32, $fp                               ## L:128  |    211 | m0:ufract = load(address, 0x20).xyzwxyzw;
  ldv $v02, 8, 32, $fp                               ## L:128  |    212 | m0:ufract = load(address, 0x20).xyzwxyzw;
  ldv $v03, 0, 8, $fp                                ## L:129  |    213 | m1:sint   = load(address, 0x08).xyzwxyzw;
  ldv $v03, 8, 8, $fp                                ## L:129  |    214 | m1:sint   = load(address, 0x08).xyzwxyzw;
  ldv $v04, 0, 40, $fp                               ## L:130  |    215 | m1:ufract = load(address, 0x28).xyzwxyzw;
  ldv $v04, 8, 40, $fp                               ## L:130  |    216 | m1:ufract = load(address, 0x28).xyzwxyzw;
  ldv $v05, 0, 16, $fp                               ## L:131  |    217 | m2:sint   = load(address, 0x10).xyzwxyzw;
  ldv $v05, 8, 16, $fp                               ## L:131  |    218 | m2:sint   = load(address, 0x10).xyzwxyzw;
  ldv $v06, 0, 48, $fp                               ## L:132  |    219 | m2:ufract = load(address, 0x30).xyzwxyzw;
  ldv $v06, 8, 48, $fp                               ## L:132  |    220 | m2:ufract = load(address, 0x30).xyzwxyzw;
  ldv $v07, 0, 24, $fp                               ## L:133  |    221 | m3:sint   = load(address, 0x18).xyzwxyzw;
  ldv $v07, 8, 24, $fp                               ## L:133  |    222 | m3:sint   = load(address, 0x18).xyzwxyzw;
  ldv $v08, 0, 56, $fp                               ## L:134  |    223 | m3:ufract = load(address, 0x38).xyzwxyzw;
  ldv $v08, 8, 56, $fp                               ## L:134  |    224 | m3:ufract = load(address, 0x38).xyzwxyzw;
  vmudn $v29, $v02, $v19.h0                          ## L:369  |      ^ | VTEMP = vmvp0  * vpos.xxxxXXXX;
  vmadh $v29, $v01, $v19.h0                          ## L:369  |    225 | VTEMP = vmvp0  * vpos.xxxxXXXX;
  vmadn $v29, $v04, $v19.h1                          ## L:370  |    226 | VTEMP = vmvp1 +* vpos.yyyyYYYY;
  vmadh $v29, $v03, $v19.h1                          ## L:370  |    227 | VTEMP = vmvp1 +* vpos.yyyyYYYY;
  vmadn $v29, $v06, $v19.h2                          ## L:371  |    228 | VTEMP = vmvp2 +* vpos.zzzzZZZZ;
  vmadh $v29, $v05, $v19.h2                          ## L:371  |    229 | VTEMP = vmvp2 +* vpos.zzzzZZZZ;
  vmadn $v29, $v08, $v30.e7                          ## L:372  |    230 | vpos = vmvp3 +* 1;
  vmadh $v19, $v07, $v30.e7                          ## L:372  |    231 | vpos = vmvp3 +* 1;
  LABEL_mgfx_0001:
  beq $sp, $zero, LIGHT_END                          ## L:376  |      ^ | if (light_count == 0) goto LIGHT_END;
  ori $fp, $zero, %lo(LIGHTING_LIGHTS)               ## L:375  |   *233 | u16 light_ptr = LIGHTING_LIGHTS;
  LIGHT_LOOP:
  ldv $v27, 0, 8, $fp                                ## L:379  |    234 | vec16 vlcol = load(light_ptr, 8).xyzwxyzw;
  ldv $v28, 0, 0, $fp                                ## L:380  |    235 | vec16 vlpos = load(light_ptr, 0).xyzwxyzw;
  ldv $v28, 8, 0, $fp                                ## L:380  |    236 | vec16 vlpos = load(light_ptr, 0).xyzwxyzw;
  lh $s0, 6($fp)                                     ## L:382  |    237 | s16 light_w = load(light_ptr, 6);
  beq $s0, $zero, LABEL_mgfx_0003                    ## L:383  |  **240 | if (light_w != 0)
  ldv $v27, 8, 8, $fp                                ## L:379  |   *242 | vec16 vlcol = load(light_ptr, 8).xyzwxyzw;
  vsubc $v28, $v28, $v19.v                           ## L:391  |    243 | vlpos -= vpos;
  vmudh $v02, $v28, $v28.v                           ## L:392  | ***247 | vec32 vsqdist = vlpos * vlpos;
  vsar $v01, COP2_ACC_HI                             ## L:392  |    248 | vec32 vsqdist = vlpos * vlpos;
  vsar $v02, COP2_ACC_MD                             ## L:392  |    249 | vec32 vsqdist = vlpos * vlpos;
  vaddc $v04, $v02, $v02.h1                          ## L:393  | ***253 | vec32 vtmp = vsqdist + vsqdist.yyyyYYYY;
  vadd $v03, $v01, $v01.h1                           ## L:393  |    254 | vec32 vtmp = vsqdist + vsqdist.yyyyYYYY;
  vaddc $v02, $v04, $v02.h2                          ## L:394  |  **257 | vsqdist = vtmp + vsqdist.zzzzZZZZ;
  vadd $v01, $v03, $v01.h2                           ## L:394  |    258 | vsqdist = vtmp + vsqdist.zzzzZZZZ;
  vrsqh $v03.e0, $v01.e0                             ## L:396  | ***262 | vtmp.x = invert_half_sqrt(vsqdist).x;
  vrsql $v04.e0, $v02.e0                             ## L:396  |    263 | vtmp.x = invert_half_sqrt(vsqdist).x;
  vrsqh $v03.e0, $v01.e4                             ## L:397  |    264 | vtmp.X = invert_half_sqrt(vsqdist).X;
  vrsql $v04.e4, $v02.e4                             ## L:397  |    265 | vtmp.X = invert_half_sqrt(vsqdist).X;
  vrsqh $v03.e4, $v00.e0                             ## L:397  |    266 | vtmp.X = invert_half_sqrt(vsqdist).X;
  vmudm $v29, $v28, $v04.h0                          ## L:399  |  **269 | vlpos *= vtmp.xxxxXXXX;
  vmadh $v28, $v28, $v03.h0                          ## L:399  |    270 | vlpos *= vtmp.xxxxXXXX;
  vmudn $v29, $v04, $v27.h3                          ## L:400  |    271 | vec16 vatten = vtmp * vlcol.wwwwWWWW;
  vmadh $v05, $v03, $v27.h3                          ## L:400  |    272 | vec16 vatten = vtmp * vlcol.wwwwWWWW;
  vmulf $v05, $v05, $v05.v                           ## L:401  | ***276 | vatten:sfract *= vatten:sfract;
  vmulf $v27, $v27, $v05.h0                          ## L:402  | ***280 | vlcol:sfract *= vatten:sfract.xxxxXXXX;
  ori $s1, $zero, %lo(MATRICES_MVP)                  ## L:411  |      ^ | u16 addr = MATRICES_MVP;
  ldv $v01, 0, 0, $s1                                ## L:127  |    281 | m0:sint   = load(address, 0x00).xyzwxyzw;
  ldv $v01, 8, 0, $s1                                ## L:127  |    282 | m0:sint   = load(address, 0x00).xyzwxyzw;
  ldv $v02, 0, 32, $s1                               ## L:128  |    283 | m0:ufract = load(address, 0x20).xyzwxyzw;
  ldv $v02, 8, 32, $s1                               ## L:128  |    284 | m0:ufract = load(address, 0x20).xyzwxyzw;
  ldv $v03, 0, 8, $s1                                ## L:129  |    285 | m1:sint   = load(address, 0x08).xyzwxyzw;
  ldv $v03, 8, 8, $s1                                ## L:129  |    286 | m1:sint   = load(address, 0x08).xyzwxyzw;
  ldv $v04, 0, 40, $s1                               ## L:130  |    287 | m1:ufract = load(address, 0x28).xyzwxyzw;
  ldv $v04, 8, 40, $s1                               ## L:130  |    288 | m1:ufract = load(address, 0x28).xyzwxyzw;
  ldv $v05, 0, 16, $s1                               ## L:131  |    289 | m2:sint   = load(address, 0x10).xyzwxyzw;
  ldv $v05, 8, 16, $s1                               ## L:131  |    290 | m2:sint   = load(address, 0x10).xyzwxyzw;
  ldv $v06, 0, 48, $s1                               ## L:132  |    291 | m2:ufract = load(address, 0x30).xyzwxyzw;
  ldv $v06, 8, 48, $s1                               ## L:132  |    292 | m2:ufract = load(address, 0x30).xyzwxyzw;
  ldv $v07, 0, 24, $s1                               ## L:133  |    293 | m3:sint   = load(address, 0x18).xyzwxyzw;
  ldv $v07, 8, 24, $s1                               ## L:133  |    294 | m3:sint   = load(address, 0x18).xyzwxyzw;
  ldv $v08, 0, 56, $s1                               ## L:134  |    295 | m3:ufract = load(address, 0x38).xyzwxyzw;
  ldv $v08, 8, 56, $s1                               ## L:134  |    296 | m3:ufract = load(address, 0x38).xyzwxyzw;
  LABEL_mgfx_0003:
  vmulf $v28, $v28, $v20.v                           ## L:416  |      ^ | vlpos *= vnorm:sfract;
  vmulf $v29, $v27, $v28.h0                          ## L:417  | ***300 | VTEMP = vlcol:sfract  * vlpos:sfract.xxxxXXXX;
  vmacf $v29, $v27, $v28.h1                          ## L:418  |    301 | VTEMP = vlcol:sfract +* vlpos:sfract.yyyyYYYY;
  vmacf $v27, $v27, $v28.h2                          ## L:419  |    302 | vlcol = vlcol:sfract +* vlpos:sfract.zzzzZZZZ;
  vge $v27, $v27, $v00                               ## L:420  | ***306 | vlcol = max(vlcol, VZERO);
  addiu $fp, $fp, 16                                 ## L:424  |      ^ | light_ptr += 16;
  addiu $sp, $sp, 65535                              ## L:425  |    307 | light_count -= 1;
  bne $sp, $zero, LIGHT_LOOP                         ## L:426  |    308 | if (light_count != 0) goto LIGHT_LOOP;
  vadd $v22, $v22, $v27.v                            ## L:422  |  **311 | vrgba:sint += vlcol;
  LIGHT_END:
  vmov $v22.e3, $v12.e3                              ## L:430  |    312 | vrgba.w = vnormmask.w;
  vmov $v22.e7, $v12.e3                              ## L:431  |    313 | vrgba.W = vnormmask.w;
  PATCH_COLOR432: vmulf $v22, $v22, $v23.v           ## L:432  | ***317 | @AttrPatch("COLOR:vnop") vrgba:sfract *= vrgba_in:sfract;
  ctc2 $t1, $vcc                                     ## L:434  |      ^ | set_vcc(fog_mask);
  vmrg $v22, $v22, $v26.h2                           ## L:435  | ***321 | vrgba = select(vrgba, vfog.zzzzZZZZ);
  vmudh $v29, $v17, $v30.e7                          ## L:493  |    322 | VTEMP = vtexoffset:sint * 1;
  or $t4, $zero, $s7                                 ## L:496  |      ^ | prev_out0 = vtx_out0_ptr;
  or $t5, $zero, $t3                                 ## L:497  |    323 | prev_out1 = vtx_out1_ptr;
  addu $s4, $s4, $t9                                 ## L:498  |    324 | vtx_in0_ptr += vtx_size2;
  addu $t2, $t2, $t9                                 ## L:499  |    325 | vtx_in1_ptr += vtx_size2;
  addiu $s7, $s7, 80                                 ## L:500  |    326 | vtx_out0_ptr += 80;
  addiu $t3, $t3, 80                                 ## L:501  |    327 | vtx_out1_ptr += 80;
  LOAD_NORMAL503: lsv $v20, 0, 0, $s4                ## L:503  |    328 | @AttrLoader("NORMAL") vnorm.x = load(vtx_in0_ptr).x;
  LOAD_NORMAL504: lsv $v20, 8, 0, $t2                ## L:504  |    329 | @AttrLoader("NORMAL") vnorm.X = load(vtx_in1_ptr).x;
  LOAD_POSITION505: ldv $v19, 0, 0, $s4              ## L:505  |    330 | @AttrLoader("POSITION") vpos.xyzw = load(vtx_in0_ptr).xyzw;
  LOAD_POSITION506: ldv $v19, 8, 0, $t2              ## L:506  |    331 | @AttrLoader("POSITION") vpos.XYZW = load(vtx_in1_ptr).xyzw;
  LOAD_COLOR507: llv $v23, 0, 0, $s4                 ## L:507  |    332 | @AttrLoader("COLOR") vrgba_in.xy = load(vtx_in0_ptr).xy;
  LOAD_COLOR508: llv $v23, 4, 0, $t2                 ## L:508  |    333 | @AttrLoader("COLOR") vrgba_in.zw = load(vtx_in1_ptr).xy;
  sltu $at, $s4, $s6                                 ## L:510  |    334 | if (vtx_in0_ptr < vtx_in_end) goto VERTEX_LOOP;
  bne $at, $zero, VERTEX_LOOP                        ## L:510  |    335 | if (vtx_in0_ptr < vtx_in_end) goto VERTEX_LOOP;
  vmadh $v21, $v21, $v14.v                           ## L:494  |   *337 | vtex:sint = vtex:sint +* vtexscale;
  sdv $v24, 0, 0, $t4                                ## L:512  |    338 | @Barrier("st_c0") @Barrier("st_tr0") store(vpos_clip:sint.xyzw, prev_out0, 0); ## Barrier: 0xC
  sdv $v24, 8, 0, $t5                                ## L:513  |    339 | @Barrier("st_c1") @Barrier("st_tr1") store(vpos_clip:sint.XYZW, prev_out1, 0); ## Barrier: 0x30
  sb $t7, 6($t5)                                     ## L:516  |    340 | @Barrier("st_c1") store(c1, prev_out1, 6); ## Barrier: 0x10
  sb $k0, 7($t4)                                     ## L:517  |    341 | @Barrier("st_tr0") store(tr0, prev_out0, 7); ## Barrier: 0x8
  sb $k1, 7($t5)                                     ## L:518  |    342 | @Barrier("st_tr1") store(tr1, prev_out1, 7); ## Barrier: 0x20
  suv $v22, 0, 8, $t4                                ## L:520  |    343 | @Barrier("st_tex0") store_vec_u8(vrgba.x, prev_out0, 8); ## Barrier: 0x40
  suv $v22, 4, 8, $t5                                ## L:521  |    344 | @Barrier("st_tex1") store_vec_u8(vrgba.X, prev_out1, 8); ## Barrier: 0x80
  slv $v21, 0, 12, $t4                               ## L:523  |    345 | @Barrier("st_tex0") store(vtex.xy, prev_out0, 12); ## Barrier: 0x40
  slv $v21, 8, 12, $t5                               ## L:524  |    346 | @Barrier("st_tex1") store(vtex.XY, prev_out1, 12); ## Barrier: 0x80
  j RSPQ_Loop                                        ## L:525  |    347 | }
  sb $t6, 6($t4)                                     ## L:515  |   *349 | @Barrier("st_c0") store(c0, prev_out0, 6); ## Barrier: 0x4

MgEndShader

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