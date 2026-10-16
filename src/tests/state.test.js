import {transpileSource} from "../lib/transpiler";
import state from "../lib/state.js";
import {isVecReg} from "../lib/syntax/registers.js";

const CONF = {rspqWrapper: true};

const getDataSection = asm => {
  const idxData = asm.indexOf(".data");
  const idxText = asm.indexOf(".text");
  return asm.substring(idxData, idxText);
}

describe('State', () =>
{
  test('Empty State', async () => {
    const {asm, warn} = await transpileSource(`
      state {}
      `, CONF);

    expect(warn).toBe("");
    expect(getDataSection(asm)).toBe(`.data
  RSPQ_BeginOverlayHeader
  RSPQ_EndOverlayHeader

  RSPQ_EmptySavedState

`);
  });

  test('Types', async () => {
    const {asm, warn} = await transpileSource(`
      state {
        u8 a;
        u16 b;
        u32 c;
        vec16 d;
        vec32 e;
      }
      `, CONF);

    expect(warn).toBe("");
    expect(getDataSection(asm)).toBe(`.data
  RSPQ_BeginOverlayHeader
  RSPQ_EndOverlayHeader

  RSPQ_BeginSavedState
    STATE_MEM_START:
    a: .ds.b 1
    .align 1
    b: .ds.b 2
    .align 2
    c: .ds.b 4
    .align 4
    d: .ds.b 16
    .align 4
    e: .ds.b 32
    STATE_MEM_END:
  RSPQ_EndSavedState

`);
  });

  test('Arrays', async () => {
    const {asm, warn} = await transpileSource(`
      state {
        u32 a0[1];
        u32 a1[4];
        u32 a2[2][4];
        vec32 b0[1];
        vec32 b1[2];
        vec32 b2[4][2];
      }
      `, CONF);

    expect(warn).toBe("");
    expect(getDataSection(asm)).toBe(`.data
  RSPQ_BeginOverlayHeader
  RSPQ_EndOverlayHeader

  RSPQ_BeginSavedState
    STATE_MEM_START:
    .align 2
    a0: .ds.b 4
    .align 2
    a1: .ds.b 16
    .align 2
    a2: .ds.b 32
    .align 4
    b0: .ds.b 32
    .align 4
    b1: .ds.b 64
    .align 4
    b2: .ds.b 256
    STATE_MEM_END:
  RSPQ_EndSavedState

`);
  });

  test('Extern', async () => {
    const {asm, warn} = await transpileSource(`
      state {
        u32 a;
        extern u32 b;
        u32 c;
      }
      `, CONF);

    expect(warn).toBe("");
    expect(getDataSection(asm)).toBe(`.data
  RSPQ_BeginOverlayHeader
  RSPQ_EndOverlayHeader

  RSPQ_BeginSavedState
    STATE_MEM_START:
    .align 2
    a: .ds.b 4
    .align 2
    c: .ds.b 4
    STATE_MEM_END:
  RSPQ_EndSavedState

`);
  });

  test('Align', async () => {
    const {asm, warn} = await transpileSource(`
      state {
        u16 a;
        alignas(8) u16 b;
        alignas(4) u8 c;
      }
      `, CONF);

    expect(warn).toBe("");
    expect(getDataSection(asm)).toBe(`.data
  RSPQ_BeginOverlayHeader
  RSPQ_EndOverlayHeader

  RSPQ_BeginSavedState
    STATE_MEM_START:
    .align 1
    a: .ds.b 2
    .align 3
    b: .ds.b 2
    .align 2
    c: .ds.b 1
    STATE_MEM_END:
  RSPQ_EndSavedState

`);
  });

  test('Align lower', async () => {
    const {asm, warn} = await transpileSource(`
      state {
        vec16 VEC_A;
        alignas(8) vec16 VEC_A;
      }
      `, CONF);

    expect(warn).toBe("");
    expect(getDataSection(asm)).toBe(`.data
  RSPQ_BeginOverlayHeader
  RSPQ_EndOverlayHeader

  RSPQ_BeginSavedState
    STATE_MEM_START:
    .align 4
    VEC_A: .ds.b 16
    .align 3
    VEC_A: .ds.b 16
    STATE_MEM_END:
  RSPQ_EndSavedState

`);
  });

test('Data State', async () => {
    const {asm, warn} = await transpileSource(`
      data {
        u32 BBB;
        u32 CCC;
      }
      `, CONF);

    expect(warn).toBe("");
    expect(getDataSection(asm)).toBe(`.data
  RSPQ_BeginOverlayHeader
  RSPQ_EndOverlayHeader

  RSPQ_EmptySavedState

    .align 2
    BBB: .ds.b 4
    .align 2
    CCC: .ds.b 4

`);
  });

  test('BSS Only', async () => {
    const {asm, warn} = await transpileSource(`
      bss {
        u32 DDD;
      }
      `, CONF);

    expect(warn).toBe("");
    expect(getDataSection(asm)).toBe(`.data
  RSPQ_BeginOverlayHeader
  RSPQ_EndOverlayHeader

  RSPQ_EmptySavedState

.bss
  TEMP_STATE_MEM_START:
    .align 2
    DDD: .ds.b 4
  TEMP_STATE_MEM_END:

`);
  });

  test('Data + State', async () => {
    const {asm, warn} = await transpileSource(`
      state {
        u32 AAA;
      }
      data {
        u32 BBB;
        u32 CCC;
      }
      `, CONF);

    expect(warn).toBe("");
    expect(getDataSection(asm)).toBe(`.data
  RSPQ_BeginOverlayHeader
  RSPQ_EndOverlayHeader

  RSPQ_BeginSavedState
    STATE_MEM_START:
    .align 2
    AAA: .ds.b 4
    STATE_MEM_END:
  RSPQ_EndSavedState

    .align 2
    BBB: .ds.b 4
    .align 2
    CCC: .ds.b 4

`);
  });

  test('Data + State + BSS', async () => {
    const {asm, warn} = await transpileSource(`
      state {
        u32 AAA;
      }
      data {
        u32 BBB;
        u32 CCC;
      }
      bss {
        u32 DDD;
      }
      `, CONF);

    expect(warn).toBe("");
    expect(getDataSection(asm)).toBe(`.data
  RSPQ_BeginOverlayHeader
  RSPQ_EndOverlayHeader

  RSPQ_BeginSavedState
    STATE_MEM_START:
    .align 2
    AAA: .ds.b 4
    STATE_MEM_END:
  RSPQ_EndSavedState

    .align 2
    BBB: .ds.b 4
    .align 2
    CCC: .ds.b 4

.bss
  TEMP_STATE_MEM_START:
    .align 2
    DDD: .ds.b 4
  TEMP_STATE_MEM_END:

`);
  });

  test('Extern state variables registered for lookup', async () => {
    const {asm, warn} = await transpileSource(`
state {
  extern u32 RDPQ_CMD_STAGING;
  extern u16 RSPQ_Loop;
  vec16 MY_VAR;
}
function test(u32 dummy)
{
  u32 x = RDPQ_CMD_STAGING;
  u32 y = RSPQ_Loop;
}
`, CONF);
    expect(warn).toBe("");
    expect(asm).toContain("%lo(RDPQ_CMD_STAGING)");
    expect(asm).toContain("%lo(RSPQ_Loop)");
  });
});

describe('state', () =>
{
  test('basic lifecycle', () => {
    state.reset();
    state.enterFunction("test", "function", 0);

    expect(state.func).toBe("test");
    expect(state.funcType).toBe("function");
    expect(state.varExists("ZERO")).toBe(true);
    expect(state.varExists("VZERO")).toBe(true);
    expect(state.varExists("RA")).toBe(true);

    state.leaveFunction();
    expect(state.func).toBe("");
  });

  test('register allocation scalar', () => {
    state.reset();
    state.enterFunction("test", "function", 0);

    const reg = state.allocRegister("u32");
    expect(reg).toBeTruthy();
    expect(isVecReg(reg)).toBe(false); // Scalar type gets scalar register

    state.declareVar("a", "u32", reg);
    expect(state.varExists("a")).toBe(true);

    // Register is marked used, next allocation gets a different one
    const reg2 = state.allocRegister("u32");
    expect(reg2).not.toBe(reg);

    state.leaveFunction();
  });

  test('register allocation vector', () => {
    state.reset();
    state.enterFunction("test", "function", 0);

    const reg = state.allocRegister("vec16");
    expect(reg).toBeTruthy();
    expect(isVecReg(reg)).toBe(true);

    state.leaveFunction();
  });

  test('scope push/pop', () => {
    state.reset();
    state.enterFunction("test", "function", 0);

    const reg = state.allocRegister("u32");
    state.declareVar("outer", "u32", reg);

    state.pushScope();
    expect(state.varExists("outer")).toBe(true); // Inherited from parent
    state.declareVar("inner", "u32", state.allocRegister("u32"));
    expect(state.varExists("inner")).toBe(true);
    state.popScope();

    expect(state.varExists("outer")).toBe(true);
    expect(state.varExists("inner")).toBe(false); // Gone after pop

    state.leaveFunction();
  });

  test('label generation', () => {
    state.reset();
    state.enterFunction("myFunc", "function", 0);

    const label1 = state.generateLabel();
    const label2 = state.generateLabel();
    expect(label1).not.toBe(label2);
    expect(label1.startsWith("LABEL_myFunc_")).toBe(true);

    state.leaveFunction();
  });

  test('const and modify tracking', () => {
    state.reset();
    state.enterFunction("test", "function", 0);

    const reg = state.allocRegister("u32");
    state.declareVar("x", "u32", reg, true); // const

    state.markVarModified("x");
    const v = state.getRequiredVar("x", "test", "");
    expect(v.modifyCount).toBe(1);
    expect(v.isConst).toBe(true);

    state.leaveFunction();
  });
});
