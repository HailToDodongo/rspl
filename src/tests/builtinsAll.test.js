import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

function expectThrow(src, msg) {
  return expect(transpileSource(src, CONF)).rejects.toThrow(msg);
}

// ============================================================================
// MFC0 Reads (get_dma_busy, get_rdp_start, get_rdp_end, get_rdp_current)
// ============================================================================

function testMfc0Read(name, cop0Reg) {
  describe(`Builtins - ${name}()`, () => {
    test('basic', async () => {
      const {asm, warn} = await transpileSource(`function test() {
        u32<$t0> a = ${name}();
      }`, CONF);
      expect(warn).toBe("");
      expect(asm).toBe(`test:
  mfc0 $t0, ${cop0Reg}
  jr $ra
  nop`);
    });

    test('fails with no left side', async () => {
      await expectThrow(`function test() {
        ${name}();
      }`, "must have a left side");
    });

    test('fails with arguments', async () => {
      await expectThrow(`function test() {
        u32<$t0> a = ${name}(42);
      }`, "requires no arguments");
    });

    test('fails with vector left side', async () => {
      await expectThrow(`function test() {
        vec16<$v01> a = ${name}();
      }`, "scalar variable");
    });
  });
}

testMfc0Read("get_dma_busy", "COP0_DMA_BUSY");
testMfc0Read("get_rdp_start", "COP0_DP_START");
testMfc0Read("get_rdp_end", "COP0_DP_END");
testMfc0Read("get_rdp_current", "COP0_DP_CURRENT");

// ============================================================================
// MTC0 Writes
// ============================================================================

function testMtc0Write(name, cop0Reg) {
  describe(`Builtins - ${name}()`, () => {
    test('basic - scalar variable', async () => {
      const {asm, warn} = await transpileSource(`function test() {
        u32<$t0> a;
        ${name}(a);
      }`, CONF);
      expect(warn).toBe("");
      expect(asm).toBe(`test:
  mtc0 $t0, ${cop0Reg}
  jr $ra
  nop`);
    });

    test('basic - literal', async () => {
      const {asm, warn} = await transpileSource(`function test() {
        ${name}(42);
      }`, CONF);
      expect(warn).toBe("");
      expect(asm).toBe(`test:
  addiu $at, $zero, 42
  mtc0 $at, ${cop0Reg}
  jr $ra
  nop`);
    });

    test('fails with left side', async () => {
      await expectThrow(`function test() {
        u32<$t0> a = ${name}(42);
      }`, "must not have a left side");
    });

    test('fails with no argument', async () => {
      await expectThrow(`function test() {
        ${name}();
      }`, "requires 1 scalar");
    });

    test('fails with vector argument', async () => {
      await expectThrow(`function test() {
        vec16<$v01> a;
        ${name}(a);
      }`, "scalar argument");
    });
  });
}

testMtc0Write("set_rdp_start", "COP0_DP_START");
testMtc0Write("set_rdp_end", "COP0_DP_END");
testMtc0Write("set_rdp_current", "COP0_DP_CURRENT");
testMtc0Write("set_dma_addr_rsp", "COP0_DMA_SPADDR");
testMtc0Write("set_dma_addr_rdram", "COP0_DMA_RAMADDR");
testMtc0Write("set_dma_write", "COP0_DMA_WRITE");
testMtc0Write("set_dma_read", "COP0_DMA_READ");

// ============================================================================
// Accumulator / VCC
// ============================================================================

describe('Builtins - clear_vcc()', () => {
  test('basic', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      clear_vcc();
    }`, CONF);
    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vsubc $v29, $v00, $v00
  jr $ra
  nop`);
  });

  test('fails with left side', async () => {
    await expectThrow(`function test() {
      u32<$t0> a = clear_vcc();
    }`, "must not have a left side");
  });

  test('fails with arguments', async () => {
    await expectThrow(`function test() {
      clear_vcc(42);
    }`, "requires no arguments");
  });
});

describe('Builtins - get_acc()', () => {
  test('basic', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v01> a = get_acc();
    }`, CONF);
    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vsar $v01, COP2_ACC_HI
  vsar $v02, COP2_ACC_MD
  jr $ra
  nop`);
  });

  test('fails with no left side', async () => {
    await expectThrow(`function test() {
      get_acc();
    }`, "must have a left side");
  });

  test('fails with arguments', async () => {
    await expectThrow(`function test() {
      vec32<$v01> a = get_acc(42);
    }`, "requires no arguments");
  });

  test('fails with scalar left side', async () => {
    await expectThrow(`function test() {
      u32<$t0> a = get_acc();
    }`, "vector variable");
  });

  test('fails with vec16 left side', async () => {
    await expectThrow(`function test() {
      vec16<$v01> a = get_acc();
    }`, "vec32");
  });
});

describe('Builtins - get_acc_high / mid / low', () => {
  function testAccSingle(name, cop2Reg) {
    test(`${name}() - basic`, async () => {
      const {asm, warn} = await transpileSource(`function test() {
        vec16<$v03> a = ${name}();
      }`, CONF);
      expect(warn).toBe("");
      expect(asm).toBe(`test:
  vsar $v03, ${cop2Reg}
  jr $ra
  nop`);
    });

    test(`${name}() - fails with scalar left side`, async () => {
      await expectThrow(`function test() {
        u32<$t0> a = ${name}();
      }`, "vector variable");
    });

    test(`${name}() - fails with vec32 left side`, async () => {
      await expectThrow(`function test() {
        vec32<$v01> a = ${name}();
      }`, "vec16");
    });

    test(`${name}() - fails with arguments`, async () => {
      await expectThrow(`function test() {
        vec16<$v01> a = ${name}(1);
      }`, "requires no arguments");
    });
  }

  testAccSingle("get_acc_high", "COP2_ACC_HI");
  testAccSingle("get_acc_mid", "COP2_ACC_MD");
  testAccSingle("get_acc_low", "COP2_ACC_LO");
});

describe('Builtins - get_vcc()', () => {
  test('basic', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      u32<$t0> a = get_vcc();
    }`, CONF);
    expect(warn).toBe("");
    expect(asm).toBe(`test:
  cfc2 $t0, $vcc
  jr $ra
  nop`);
  });

  test('fails with no left side', async () => {
    await expectThrow(`function test() {
      get_vcc();
    }`, "must have a left side");
  });

  test('fails with arguments', async () => {
    await expectThrow(`function test() {
      u32<$t0> a = get_vcc(1);
    }`, "requires no arguments");
  });

  test('fails with vector left side', async () => {
    await expectThrow(`function test() {
      vec16<$v01> a = get_vcc();
    }`, "scalar variable");
  });
});

describe('Builtins - set_vcc()', () => {
  test('basic - scalar', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      u32<$t0> a;
      set_vcc(a);
    }`, CONF);
    expect(warn).toBe("");
    expect(asm).toBe(`test:
  ctc2 $t0, $vcc
  jr $ra
  nop`);
  });

  test('basic - literal', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      set_vcc(1);
    }`, CONF);
    expect(warn).toBe("");
    expect(asm).toBe(`test:
  addiu $at, $zero, 1
  ctc2 $at, $vcc
  jr $ra
  nop`);
  });

  test('fails with left side', async () => {
    await expectThrow(`function test() {
      u32<$t0> a = set_vcc(1);
    }`, "must not have a left side");
  });

  test('fails with no argument', async () => {
    await expectThrow(`function test() {
      set_vcc();
    }`, "requires 1 scalar");
  });

  test('fails with vector argument', async () => {
    await expectThrow(`function test() {
      vec16<$v01> a;
      set_vcc(a);
    }`, "scalar argument");
  });
});

// ============================================================================
// Inline ASM
// ============================================================================

describe('Builtins - asm()', () => {
  test('basic - string only', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      asm("nop");
    }`, CONF);
    expect(warn).toBe("");
    expect(asm).toBe(`test:
  nop # inline-ASM
  jr $ra
  nop`);
  });

  test('with substitution - number', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      asm("addiu $t0, $zero, %0", 42);
    }`, CONF);
    expect(warn).toBe("");
    expect(asm).toContain("addiu $t0, $zero, 42");
    expect(asm).toContain("# inline-ASM");
  });

  test('with substitution - register', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      u32<$t1> a;
      asm("addu $t0, $zero, %0", a);
    }`, CONF);
    expect(warn).toBe("");
    expect(asm).toContain("addu $t0, $zero, $t1");
    expect(asm).toContain("# inline-ASM");
  });

  test('fails with left side', async () => {
    await expectThrow(`function test() {
      u32<$t0> a = asm("nop");
    }`, "cannot have a left side");
  });

  test('fails with first arg not string', async () => {
    await expectThrow(`function test() {
      u32<$t0> a;
      asm(42);
    }`, "first argument to be a string");
  });
});

describe('Builtins - asm_op()', () => {
  test('basic - single register arg', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      u32<$t0> a;
      asm_op("mtc0", a);
    }`, CONF);
    expect(warn).toBe("");
    expect(asm).toBe(`test:
  mtc0 $t0
  jr $ra
  nop`);
  });

  test('fails with left side', async () => {
    await expectThrow(`function test() {
      u32<$t0> a = asm_op("nop");
    }`, "cannot have a left side");
  });

  test('fails with first arg not string', async () => {
    await expectThrow(`function test() {
      asm_op(42);
    }`, "opcode");
  });
});

// ============================================================================
// dma_await
// ============================================================================

describe('Builtins - dma_await()', () => {
  test('basic', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      dma_await();
    }`, CONF);
    expect(warn).toBe("");
    expect(asm).toBe(`test:
  jal DMAWaitIdle
  nop
  jr $ra
  nop`);
  });

  test('fails with left side', async () => {
    await expectThrow(`function test() {
      u32<$t0> a = dma_await();
    }`, "cannot have a left side");
  });

  test('fails with arguments', async () => {
    await expectThrow(`function test() {
      dma_await(1);
    }`, "requires no arguments");
  });
});

// ============================================================================
// get_cmd_address
// ============================================================================

describe('Builtins - get_cmd_address()', () => {
  test('basic - with offset', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      u32<$t1> a = get_cmd_address(12);
    }`, CONF);
    expect(warn).toBe("");
    expect(asm).toBe(`test:
  addiu $t1, $gp, %lo(RSPQ_DMEM_BUFFER) + 12
  jr $ra
  nop`);
  });

  test('fails with no left side', async () => {
    await expectThrow(`function test() {
      get_cmd_address();
    }`, "must have a left side");
  });

  test('fails with too many arguments', async () => {
    await expectThrow(`function test() {
      u32<$t0> a = get_cmd_address(1, 2);
    }`, "zero or one argument");
  });

  test('fails with non-number argument', async () => {
    await expectThrow(`function test() {
      u32<$t0> a;
      u32<$t1> b = get_cmd_address(a);
    }`, "number");
  });

  test('fails with vector left side', async () => {
    await expectThrow(`function test() {
      vec16<$v01> a = get_cmd_address();
    }`, "scalar variable");
  });
});

// ============================================================================
// load_arg
// ============================================================================

describe('Builtins - load_arg()', () => {
  test('basic - with offset', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      u32<$t1> a = load_arg(8);
    }`, CONF);
    expect(warn).toBe("");
    expect(asm).toBe(`test:
  lw $t1, %lo(RSPQ_DMEM_BUFFER + 8)($gp)
  jr $ra
  nop`);
  });

  test('fails with no left side', async () => {
    await expectThrow(`function test() {
      load_arg();
    }`, "must have a left side");
  });

  test('fails with too many arguments', async () => {
    await expectThrow(`function test() {
      u32<$t0> a = load_arg(1, 2);
    }`, "zero or one argument");
  });

  test('fails with non-number argument', async () => {
    await expectThrow(`function test() {
      u32<$t0> a;
      u32<$t1> b = load_arg(a);
    }`, "number");
  });

  test('fails with vector left side', async () => {
    await expectThrow(`function test() {
      vec16<$v01> a = load_arg();
    }`, "scalar variable");
  });
});

// ============================================================================
// max / min
// ============================================================================

describe('Builtins - max()', () => {
  test('basic', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> a, b;
      vec16<$v03> c = max(a, b);
    }`, CONF);
    expect(warn).toBe("");
    expect(asm).toContain("vge");
  });

  test('fails with wrong arg count', async () => {
    await expectThrow(`function test() {
      vec16<$v01> a;
      vec16<$v03> c = max(a);
    }`, "exactly two arguments");
  });

  test('fails with type mismatch', async () => {
    await expectThrow(`function test() {
      vec16<$v01> a;
      vec32<$v03> b;
      vec16<$v05> c = max(a, b);
    }`, "same type");
  });
});

describe('Builtins - min()', () => {
  test('basic', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> a, b;
      vec16<$v03> c = min(a, b);
    }`, CONF);
    expect(warn).toBe("");
    expect(asm).toContain("vlt");
  });

  test('fails with wrong arg count', async () => {
    await expectThrow(`function test() {
      vec16<$v01> a;
      vec16<$v03> c = min(a);
    }`, "exactly two arguments");
  });
});

// ============================================================================
// assert
// ============================================================================

describe('Builtins - assert()', () => {
  test('basic', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      assert(42);
    }`, CONF);
    expect(warn).toBe("");
    expect(asm).toBe(`test:
  lui $at, 42
  j assertion_failed
  nop
  jr $ra
  nop`);
  });

  test('fails with left side', async () => {
    await expectThrow(`function test() {
      u32<$t0> a = assert(1);
    }`, "cannot have a left side");
  });

  test('fails with wrong arg count', async () => {
    await expectThrow(`function test() {
      assert(1, 2);
    }`, "exactly one argument");
  });

  test('fails with non-number argument', async () => {
    await expectThrow(`function test() {
      u32<$t0> a;
      assert(a);
    }`, "number");
  });
});