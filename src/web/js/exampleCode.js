/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

export const EXAMPLE_CODE = `include "rsp_queue.inc"

state
{
  extern u32 RDPQ_CMD_STAGING;
  
  vec32 VEC_SLOTS[20];
  u32 TEST_CONST;
}

function RDPQ_Send(u32<$s4> buffStart, u32<$s3> buffEnd);

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

  store(res, trans_out);
}

function vec_add(vec32<$v13> test)
{
  vec32<$v01> testAdd;
  testAdd.x = 12.7;
  
  test += testAdd.x;
}

function func_extern()
{
  u32<$s4> buffer = RDPQ_CMD_STAGING;
  u32<$s3> buffEnd = buffer;
  
  RDPQ_Send(buffer, buffEnd);
}

function branching() 
{
  u32<$t0> a, b;
  if(a > 0x112233) {
    b = 11;
  } else {
    if(b != 42) {
      b += 10;
    }
    b = 22;
  }
}

function scope() 
{
  u32<$t0> a;
  {
     u32<$t1> b;
     b += 2;
  } // 'b' is no longer defined now
  a += 2;
}

function labels()
{
  u32<$t0> a;
  
  label_a:
    a += 2;
    goto label_b;
    a += 0xFF;
    
  label_b:
    goto label_a;
}

include "rsp_rdpq.inc"
`;