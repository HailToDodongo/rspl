import {transpileSource} from "../../lib/transpiler";

const CONF = {rspqWrapper: true};

describe('Define (ASM)', () =>
{
  // defines in RSPL should be put into the ASM output too
  test('Define in ASM', async () => {
    const {asm, warn} = await transpileSource(`
      include "rsp_queue.inc"
      include "rdpq_macros.h"

      #define SOME_DEF_A 1
      #define SOME_DEF_B 2
      
      state{}
      
      #define SOME_DEF_C 3
      
      command<0> test(u32 a)
      {
      }
      
      #define SOME_DEF_D 4
    `, CONF);

    expect(warn).toBe("");
    expect(asm).toContain("#define SOME_DEF_A 1\n");
    expect(asm).toContain("#define SOME_DEF_B 2\n");
    expect(asm).toContain("#define SOME_DEF_C 3\n");
    expect(asm).toContain("#define SOME_DEF_D 4\n");
  });
});
