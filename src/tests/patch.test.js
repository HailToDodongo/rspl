import * as transpiler from "../lib/transpiler";
import {transpileSource} from "../lib/transpiler";

// Function patching (--patch), 
// only the named functions are optimized and spliced into an existing file

const getFunctionStartEnd = (source, funcName) => transpiler.getFunctionStartEnd(source, funcName);
const patchAsmFunctions = (oldAsm, newAsm, funcNames) => transpiler.patchAsmFunctions(oldAsm, newAsm, funcNames);

function funcBody(source, funcName) {
  const start = source.indexOf(funcName + ":\n");
  if(start < 0) throw new Error("Function " + funcName + " not found in output file!");
  for(let i = start; i + 1 < source.length; ++i) {
    if(source[i] === "\n" && /[A-Za-z0-9]/.test(source[i + 1])) return source.substring(start, i);
  }
  return source.substring(start);
}

const OLD_ASM = "## header\n"
              + ".text\n"
              + "funcA:\n"
              + "  old a1\n"
              + "  old a2\n"
              + "funcB:\n"
              + "  old b1\n"
              + "funcC:\n"
              + "  old c1\n"
              + "OVERLAY_CODE_END:\n";

const NEW_ASM = "## header\n"
              + ".text\n"
              + "funcA:\n"
              + "  new a1\n"
              + "funcB:\n"
              + "  new b1\n"
              + "  new b2\n"
              + "  new b3\n"
              + "funcC:\n"
              + "  new c1\n"
              + "OVERLAY_CODE_END:\n";

describe('Patch', () =>
{
  test('getFunctionStartEnd covers label and body', () => {
    const [start, end] = getFunctionStartEnd(OLD_ASM, "funcA");
    expect(OLD_ASM.substring(start, end)).toBe("funcA:\n"
                                             + "  old a1\n"
                                             + "  old a2");
  });

  test('getFunctionStartEnd on last function', () => {
    const [start, end] = getFunctionStartEnd(OLD_ASM, "funcC");
    expect(OLD_ASM.substring(start, end)).toBe("funcC:\n"
                                             + "  old c1");
  });

  test('unknown function throws', () => {
    expect(() => getFunctionStartEnd(OLD_ASM, "nope"))
      .toThrow("Function nope not found in output file!");
  });

  test('single function replaced, rest untouched', () => {
    const out = patchAsmFunctions(OLD_ASM, NEW_ASM, ["funcB"]);
    expect(out).toBe("## header\n"
                   + ".text\n"
                   + "funcA:\n"
                   + "  old a1\n"
                   + "  old a2\n"
                   + "funcB:\n"
                   + "  new b1\n"
                   + "  new b2\n"
                   + "  new b3\n"
                   + "funcC:\n"
                   + "  old c1\n"
                   + "OVERLAY_CODE_END:\n");
  });

  test('multiple functions of differing length', () => {
    const out = patchAsmFunctions(OLD_ASM, NEW_ASM, ["funcA", "funcB"]);
    expect(out).toBe("## header\n"
                   + ".text\n"
                   + "funcA:\n"
                   + "  new a1\n"
                   + "funcB:\n"
                   + "  new b1\n"
                   + "  new b2\n"
                   + "  new b3\n"
                   + "funcC:\n"
                   + "  old c1\n"
                   + "OVERLAY_CODE_END:\n");
  });

  test('patching every function equals the new listing', () => {
    const out = patchAsmFunctions(OLD_ASM, NEW_ASM, ["funcA", "funcB", "funcC"]);
    expect(out).toBe(NEW_ASM);
  });

  test('last function ends at EOF', () => {
    const OLD = "funcA:\n  old a1\nfuncB:\n  old b1\n\n.set at\n";
    const NEW = "funcA:\n  new a1\nfuncB:\n  new b1\n  new b2\n\n.set at\n";
    const out = patchAsmFunctions(OLD, NEW, ["funcB"]);
    expect(out).toBe("funcA:\n  old a1\nfuncB:\n  new b1\n  new b2\n\n.set at\n");
  });

  test('empty function list is a no-op', () => {
    expect(patchAsmFunctions(OLD_ASM, NEW_ASM, [])).toBe(OLD_ASM);
  });

  test('missing in target file throws', () => {
    expect(() => patchAsmFunctions(OLD_ASM, NEW_ASM, ["funcD"]))
      .toThrow("Function funcD not found in output file!");
  });

  test('restrict optimization to listed functions', async () => {
    const src = `function funcA() {
      u32<$t0> a = 1;
      u32<$t1> b = 2;
    }
    function funcB() {
      u32<$t0> a = 1;
      u32<$t1> b = 2;
    }
    function funcTail() {
      u32<$t0> a = 1;
    }`;

    const full = {rspqWrapper: false, optimize: true};
    const patched = {...full, patchFunctions: ["funcA"]};

    const asmFull = (await transpileSource(src, full)).asm;
    const asmPatched = (await transpileSource(src, patched)).asm;

    expect(funcBody(asmPatched, "funcA")).toBe(funcBody(asmFull, "funcA"));
    expect(funcBody(asmPatched, "funcB")).not.toBe(funcBody(asmFull, "funcB"));
  });
});
