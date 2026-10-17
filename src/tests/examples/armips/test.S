// Auto-generated file, transpiled with RSPL
SOME_DEFINE equ 1
COLOR_BUFF equ 12340

.rsp
.create "dmem", 0x0000
  STATE_MEM_START:
    .align 16
    TEST_123: .fill 32,0
    .align 4
    AA: .d16 1, 2, 3, 4, 65535, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    .align 2
    ITERATIONS: .fill 4,0
    .align 4
    OFFSET: .fill 32,0
    .align 4
    SCALE: .fill 32,0
  STATE_MEM_END:

  TEMP_STATE_MEM_START:
    .align 16
    TEST_ABC: .fill 32,0
  TEMP_STATE_MEM_END:
.close

.create "imem", 0x1000
OVERLAY_CODE_START:
RSPQ_Loop:

Cmd_Test123:
  andi $s6, $a1, 65535                               // L:26   |      ^ | s32 sizeY = sizeXY & 0xFFFF;
  srl $s5, $a1, 16                                   // L:29   |      2 | u32 copySize = sizeXY >> 16;
  sll $s5, $s5, 1                                    // L:30   |      3 | copySize *= 2;
  addiu $s3, $zero, 12340                            // L:33   |      4 | u32 colorBuff = 12340;
  addu $s2, $s3, $s5                                 // L:34   |      5 | const u32 colorBuffEnd = colorBuff + copySize;
  ori $at, $zero, %lo(SCALE)                         // L:36   |      6 | const vec32 posScale = load(SCALE);
  lqv $v27, 0, 0, $at                                // L:36   |      7 | const vec32 posScale = load(SCALE);
  lqv $v28, 0, 16, $at                               // L:36   |      8 | const vec32 posScale = load(SCALE);
  ori $at, $zero, %lo(OFFSET)                        // L:37   |      9 | const vec32 offset = load(OFFSET);
  lqv $v25, 0, 0, $at                                // L:37   |     10 | const vec32 offset = load(OFFSET);
  lqv $v26, 0, 16, $at                               // L:37   |     11 | const vec32 offset = load(OFFSET);
  vxor $v23, $v00, $v00.e0                           // L:39   |      ^ | vec32 incX = 0;
  vxor $v24, $v00, $v00                              // L:39   |     12 | vec32 incX = 0;
  vmov $v23.e0, $v30.e5                              // L:40   |     13 | incX:sint.x = 4;
  vmov $v23.e2, $v30.e5                              // L:41   |     14 | incX:sint.z = 4;
  vmov $v23.e4, $v30.e5                              // L:42   |     15 | incX:sint.X = 4;
  vmov $v23.e6, $v30.e5                              // L:43   |     16 | incX:sint.Z = 4;
  vxor $v21, $v00, $v00.e0                           // L:45   |     17 | vec32 incY = 0;
  vxor $v22, $v00, $v00                              // L:45   |     18 | vec32 incY = 0;
  vmov $v21.e1, $v30.e6                              // L:46   |     19 | incY:sint.y = 2;
  vmov $v21.e3, $v30.e6                              // L:47   |     20 | incY:sint.w = 2;
  vmov $v21.e5, $v30.e6                              // L:48   |     21 | incY:sint.Y = 2;
  vmov $v21.e7, $v30.e6                              // L:49   |     22 | incY:sint.W = 2;
  vmudl $v29, $v24, $v28.v                           // L:51   |     23 | incX *= posScale;
  vmadm $v29, $v23, $v28.v                           // L:51   |     24 | incX *= posScale;
  vmadn $v24, $v24, $v27.v                           // L:51   |     25 | incX *= posScale;
  vmadh $v23, $v23, $v27.v                           // L:51   |     26 | incX *= posScale;
  vmudl $v29, $v22, $v28.v                           // L:52   |     27 | incY *= posScale;
  vmadm $v29, $v21, $v28.v                           // L:52   |     28 | incY *= posScale;
  vmadn $v22, $v22, $v27.v                           // L:52   |     29 | incY *= posScale;
  vmadh $v21, $v21, $v27.v                           // L:52   |     30 | incY *= posScale;
  vxor $v19, $v00, $v00.e0                           // L:54   |     31 | vec32 pos = 0;
  vxor $v20, $v00, $v00                              // L:54   |     32 | vec32 pos = 0;
  vmov $v19.e2, $v30.e7                              // L:56   |     33 | pos:sint.z = 1;
  vmov $v19.e4, $v30.e6                              // L:57   |     34 | pos:sint.X = 2;
  addiu $at, $zero, 3                                // L:58   |      ^ | pos:sint.Z = 3;
  mtc2 $at, $v19.e6                                  // L:58   |     35 | pos:sint.Z = 3;
  beq $a2, $zero, LABEL_Cmd_Test123_0001             // L:60   |     36 | if(isOddLine) {
  lw $s7, %lo(ITERATIONS)                            // L:25   |    *38 | const u32 maxIter = load(ITERATIONS);
  addu $a0, $a0, $s5                                 // L:61   |     39 | ptrScreen += copySize;
  vmov $v19.e1, $v30.e7                              // L:62   |      ^ | pos:sint.y = 1;
  vmov $v19.e3, $v30.e7                              // L:63   |     40 | pos:sint.w = 1;
  vmov $v19.e5, $v30.e7                              // L:64   |     41 | pos:sint.Y = 1;
  vmov $v19.e7, $v30.e7                              // L:65   |     42 | pos:sint.W = 1;
  LABEL_Cmd_Test123_0001:
  vmudl $v29, $v20, $v28.v                           // L:68   |     43 | pos *= posScale;
  vmadm $v29, $v19, $v28.v                           // L:68   |   **46 | pos *= posScale;
  vmadn $v20, $v20, $v27.v                           // L:68   |     47 | pos *= posScale;
  vmadh $v19, $v19, $v27.v                           // L:68   |     48 | pos *= posScale;
  vaddc $v20, $v20, $v26.v                           // L:69   |   **51 | pos += offset;
  vxor $v18, $v00, $v00.e0                           // L:72   |     52 | vec16 maskOddEven = 0;
  ori $at, $zero, 0xFFFF                             // L:73   |      ^ | maskOddEven.y = 0xFFFF;
  mtc2 $at, $v18.e1                                  // L:73   |     53 | maskOddEven.y = 0xFFFF;
  vmov $v18.e3, $v18.e1                              // L:74   |  ***57 | maskOddEven.w = maskOddEven.y;
  vmov $v18.e5, $v18.e1                              // L:75   |  ***61 | maskOddEven.Y = maskOddEven.y;
  vmov $v18.e7, $v18.e1                              // L:76   |  ***65 | maskOddEven.W = maskOddEven.y;
  j RSPQ_Loop                                        // L:77   |      ^ | }
  vadd $v19, $v19, $v25.v                            // L:69   |    *67 | pos += offset;

OVERLAY_CODE_END:
.close