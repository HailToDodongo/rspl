## Auto-generated file, transpiled with RSPL

.set noreorder
.set at

.data
  RSPQ_BeginOverlayHeader
    RSPQ_DefineCommand VecCmd_Transform, 8
  RSPQ_EndOverlayHeader

  RSPQ_BeginSavedState
    .align 8
    VEC_SLOTS: .ds.b 640 
    .align 4
    TEST_CONST: .ds.b 4 
  RSPQ_EndSavedState

.text

test_vec32_ops:
  ## Add 
  ##c = a + b; 
  jr ra

test_vector_load:
  ## Whole Vector 
  lqv $v01, 0x00, 0, t0
  lqv $v02, 0x00, 0 + 0x10, t0
  lqv $v01, 0x00, 16, t0
  lqv $v02, 0x00, 16 + 0x10, t0
  lqv $v01, 0x02, 0, t0
  lqv $v02, 0x02, 0 + 0x10, t0
  lqv $v01, 0x04, 16, t0
  lqv $v02, 0x04, 16 + 0x10, t0
  ##dst = load(src, TEST_CONST); Invalid 
  ##dst = load(TEST_CONST); Invalid 
  ##dst = load(TEST_CONST, 0x10); Invalid 
  ## Swizzle 
  ldv $v01, 0x00, 0, t0
  ldv $v01, 0x08, 0, t0
  ldv $v02, 0x00, 0 + 0x10, t0
  ldv $v02, 0x08, 0 + 0x10, t0
  ldv $v01, 0x00, 16, t0
  ldv $v01, 0x08, 16, t0
  ldv $v02, 0x00, 16 + 0x10, t0
  ldv $v02, 0x08, 16 + 0x10, t0
  ldv $v01, 0x02, 0, t0
  ldv $v01, 0x0A, 0, t0
  ldv $v02, 0x02, 0 + 0x10, t0
  ldv $v02, 0x0A, 0 + 0x10, t0
  ldv $v01, 0x04, 16, t0
  ldv $v01, 0x0C, 16, t0
  ldv $v02, 0x04, 16 + 0x10, t0
  ldv $v02, 0x0C, 16 + 0x10, t0
  ##dst = load(src, TEST_CONST).xyzwxyzw; Invalid 
  ##dst = load(TEST_CONST).xyzwxyzw; Invalid 
  ##dst = load(TEST_CONST, 0x10).xyzwxyzw; Invalid 
  jr ra

test_label:
  label_a: 
  addiu t0, t0, 2
  b label_b
  nop 
  addiu t0, t0, 255
  label_b: 
  b label_a
  nop 
  jr ra

test_scalar_load:
  ## destination = load(source, offset) 
  lw t1, 0(t0)
  lw t1, 16(t0)
  lw t1, %lo(TEST_CONST)(t0)
  lw t1, %lo(TEST_CONST + 0)
  lw t1, %lo(TEST_CONST + 16)
  ## dst = load(TEST_CONST, TEST_CONST); Invalid 
  jr ra

test_scalar_ops:
  ## Add 
  addu t2, t0, t1
  add t5, t3, t4
  addiu t2, t0, 1
  addi t5, t3, 1
  addiu t2, t0, %lo(TEST_CONST)
  addi t5, t3, %lo(TEST_CONST)
  ## Sub 
  subu t2, t0, t1
  sub t5, t3, t4
  addiu t2, t0, -1
  addi t5, t3, -1
  ##c = a - TEST_CONST; sc = sa - TEST_CONST; 
  ## Mul/Div not possible 
  ## And 
  and t2, t0, t1
  andi t2, t0, 0x0001
  andi t2, t0, %lo(TEST_CONST)
  ## Or 
  or t2, t0, t1
  ori t2, t0, 0x0002
  ori t2, t0, %lo(TEST_CONST)
  ## XOR 
  xor t2, t0, t1
  xori t2, t0, 0x0002
  xori t2, t0, %lo(TEST_CONST)
  ## Not 
  not t2, t1
  ## Shift-Left 
  sllv t2, t0, t1
  sll t2, t0, 2
  ##c = a << TEST_CONST; 
  ## Shift-Right 
  srlv t2, t0, t1
  srl t2, t0, 2
  ##c = a >> TEST_CONST; 
  jr ra

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
  jr ra

