import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: true};

describe('Examples', () =>
{
  test('Matrix x Vector', () => {
    const asm = transpileSource(`include "rsp_queue.inc"
state { 
  vec32 VEC_SLOTS[20];
}

command<0> VecCmd_Transform(u32 vec_out, u32 mat_in)
{
  u32<$t0> trans_mtx = mat_in >> 16;
  trans_mtx &= 0xFF0;

  u32<$t1> trans_vec = mat_in & 0xFF0;
  u32<$t2> trans_out = vec_out & 0xFF0;
  
  trans_mtx += VEC_SLOTS;
  trans_vec += VEC_SLOTS;
  trans_out += VEC_SLOTS;
  
  vec32<$v01> mat0 = load(trans_mtx, 0x00).xyzwxyzw;
  vec32<$v03> mat1 = load(trans_mtx, 0x08).xyzwxyzw;
  vec32<$v05> mat2 = load(trans_mtx, 0x20).xyzwxyzw;
  vec32<$v07> mat3 = load(trans_mtx, 0x28).xyzwxyzw;
  
  vec32<$v09> vecIn = load(trans_vec);
  vec32<$v13> res;
  
  res = mat0  * vecIn.xxxxXXXX;
  res = mat1 +* vecIn.yyyyYYYY;
  res = mat2 +* vecIn.zzzzZZZZ;
  res = mat3 +* vecIn.wwwwWWWW;

  trans_out = store(res);
}`, CONF).trim();

    expect(asm).toBe(`## Auto-generated file, transpiled with RSPL
#include <rsp_queue.inc>

.set noreorder
.set at

.data
  RSPQ_BeginOverlayHeader
    RSPQ_DefineCommand VecCmd_Transform, 8
  RSPQ_EndOverlayHeader

  RSPQ_BeginSavedState
    .align 8
    VEC_SLOTS: .ds.b 640
  RSPQ_EndSavedState

.text

VecCmd_Transform:
  srl t0, a1, 16
  andi t0, t0, 0x0FF0
  andi t1, a1, 0x0FF0
  andi t2, a0, 0x0FF0
  addiu t0, t0, %lo(VEC_SLOTS)
  addiu t1, t1, %lo(VEC_SLOTS)
  addiu t2, t2, %lo(VEC_SLOTS)
  ldv $v01, 0x00, 0, t0
  ldv $v01, 0x08, 0, t0
  ldv $v02, 0x00, 0 + 0x10, t0
  ldv $v02, 0x08, 0 + 0x10, t0
  ldv $v03, 0x00, 8, t0
  ldv $v03, 0x08, 8, t0
  ldv $v04, 0x00, 8 + 0x10, t0
  ldv $v04, 0x08, 8 + 0x10, t0
  ldv $v05, 0x00, 32, t0
  ldv $v05, 0x08, 32, t0
  ldv $v06, 0x00, 32 + 0x10, t0
  ldv $v06, 0x08, 32 + 0x10, t0
  ldv $v07, 0x00, 40, t0
  ldv $v07, 0x08, 40, t0
  ldv $v08, 0x00, 40 + 0x10, t0
  ldv $v08, 0x08, 40 + 0x10, t0
  lqv $v09, 0x00, 0, t1
  lqv $v10, 0x00, 0 + 0x10, t1
  vmudl $v14, $v02, $v10.h0
  vmadm $v14, $v01, $v10.h0
  vmadn $v14, $v02, $v09.h0
  vmadh $v13, $v01, $v09.h0
  
  vmadl $v14, $v04, $v10.h1
  vmadm $v14, $v03, $v10.h1
  vmadn $v14, $v04, $v09.h1
  vmadh $v13, $v03, $v09.h1
  
  vmadl $v14, $v06, $v10.h2
  vmadm $v14, $v05, $v10.h2
  vmadn $v14, $v06, $v09.h2
  vmadh $v13, $v05, $v09.h2
  
  vmadl $v14, $v08, $v10.h3
  vmadm $v14, $v07, $v10.h3
  vmadn $v14, $v08, $v09.h3
  vmadh $v13, $v07, $v09.h3
  
  sqv $v13, 0x0, 0x00, t2
  sqv $v14, 0x0, 0x10, t2
  jr ra`);
  });
});