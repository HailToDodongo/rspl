import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Scalar - Ops', () =>
{
  test('32-Bit Arithmetic', () => {
    const asm = transpileSource(`state { u32 TEST_CONST; }
      function test_scalar_ops()
      {
        u32<$t0> a, b, c;
        s32<$t3> sa, sb, sc;
        
        // Add
        c = a + b; sc = sa + sb;
        c = a + 1; sc = sa + 1;
        c = a + TEST_CONST; sc = sa + TEST_CONST;
        
        // Sub
        c = a - b; sc = sa - sb;
        c = a - 1; sc = sa - 1;
        //c = a - TEST_CONST; sc = sa - TEST_CONST; Invalid
        
        // Mul/Div not possible
    }`, CONF).trim();

    expect(asm).toBe(
`test_scalar_ops:
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
  ##c = a - TEST_CONST; sc = sa - TEST_CONST; Invalid
  ## Mul/Div not possible
  jr ra`);
  });
  
  test('32-Bit - Logic', () => {
    const asm = transpileSource(`state { u32 TEST_CONST; }
      function test_scalar_ops()
      {
        u32<$t0> a, b, c;
        s32<$t3> sa, sb, sc;
                
        // And
        c = a & b;
        c = a & 1;
        c = a & TEST_CONST;
        
        // Or
        c = a | b;
        c = a | 2;
        c = a | TEST_CONST;
        
        // XOR
        c = a ^ b;
        c = a ^ 2;
        c = a ^ TEST_CONST;
        
        // Not
        c = ~b;
        
        // Shift-Left
        c = a << b;
        c = a << 2;
        //c = a << TEST_CONST; Invalid
        
        // Shift-Right
        c = a >> b;
        c = a >> 2;
        //c = a >> TEST_CONST; Invalid
      }`, CONF).trim();

    expect(asm).toBe(
`test_scalar_ops:
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
  ##c = a << TEST_CONST; Invalid
  ## Shift-Right
  srlv t2, t0, t1
  srl t2, t0, 2
  ##c = a >> TEST_CONST; Invalid
  jr ra`);
  });
});