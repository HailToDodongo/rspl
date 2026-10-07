import {evalFunctionCost} from "../../../lib/optimizer/eval/evalCost.js";
import {asm, asmLabel, asmNOP, ASM_TYPE} from "../../../lib/intsructions/asmWriter.js";
import {asmInitDeps} from "../../../lib/optimizer/asmScanDeps.js";

// Path-aware optimization:

const W_HOT = 64;
const W_ALT = 16;
const W_COLD = 1;

function textToFunc(text)
{
  const func = {asm: []};
  for(let line of text.split("\n")) {
    line = line.trim();
    if(!line) continue;
    if(line.endsWith(":")) {
      func.asm.push(asmLabel(line.slice(0, -1)));
      continue;
    }
    const [op, ...args] = line.split(/\s+/).map(a => a.endsWith(",") ? a.slice(0, -1) : a);
    func.asm.push(op === "nop" ? asmNOP() : asm(op, args));
  }
  asmInitDeps(func);
  return func;
}

/** @return {{cost: number, hot: number}} */
function costOf(text)
{
  const f = textToFunc(text);
  const cost = evalFunctionCost(f);
  return {cost, hot: f.hotCycles};
}

describe('Eval - Path', () =>
{
  test('straight line is all hot', async () => {
    const {cost, hot} = costOf(`
      or $t0, $zero, $zero
      addiu $t0, $t0, 1
      addiu $t0, $t0, 1
      jr $ra
      nop
    `);
    expect(hot).toBe(6);
    expect(cost).toBe(6 * W_HOT);
  });

  test('cold block after jr costs almost nothing', async () => {
    // An @Unlikely-style block parked after the function tail.
    const base = costOf(`
      or $t0, $zero, $zero
      bne $t0, $zero, COLD
      nop
      JOIN:
      addiu $t0, $t0, 2
      jr $ra
      nop
    `);
    const withCold = costOf(`
      or $t0, $zero, $zero
      bne $t0, $zero, COLD
      nop
      JOIN:
      addiu $t0, $t0, 2
      jr $ra
      nop
      COLD:
      addiu $t0, $t0, 1
      addiu $t0, $t0, 1
      addiu $t0, $t0, 1
      j JOIN
      nop
    `);
    expect(base.hot).toBe(withCold.hot);                 // hot path unchanged by the cold block
    expect(withCold.cost).toBeGreaterThan(base.cost);    // ...but it is not free
    expect(withCold.cost - base.cost).toBeLessThan(W_HOT); // ...and far below one hot cycle
    expect((withCold.cost - base.cost) % W_COLD).toBe(0);
  });

  test('forward jump skips an alternative arm', async () => {
    
    const {cost, hot} = costOf(`
      or $t0, $zero, $zero
      bne $t0, $zero, ELSE
      nop
      addiu $t1, $zero, 1
      beq $zero, $zero, END
      nop
      ELSE:
      addiu $t1, $zero, 2
      addiu $t1, $t1, 2
      addiu $t1, $t1, 2
      addiu $t1, $t1, 2
      END:
      jr $ra
      nop
    `);
    // hot
    expect(hot).toBeLessThan(12);
    // else-arm
    expect(cost).toBe(hot * W_HOT + 4 * W_ALT);
  });

  test('loop body is weighted up', async () => {
    const loop = costOf(`
      or $t0, $zero, $zero
      LOOP:
      addiu $t0, $t0, 1
      addiu $t1, $t1, 1
      bne $t0, $t2, LOOP
      nop
      jr $ra
      nop
    `);
    const flat = costOf(`
      or $t0, $zero, $zero
      addiu $t0, $t0, 1
      addiu $t1, $t1, 1
      bne $t0, $t2, ELSEWHERE
      nop
      jr $ra
      nop
    `);
    expect(loop.hot).toBe(flat.hot);               // same instructions, same hot cycles
    expect(loop.cost).toBeGreaterThan(flat.cost);  // ...but the loop body counts 8x
  });

  test('all hot ops get a cycle, cold ops restart at 1', async () => {
    const f = textToFunc(`
      or $t0, $zero, $zero
      jr $ra
      nop
      COLD:
      addiu $t0, $t0, 1
      addiu $t0, $t0, 1
    `);
    evalFunctionCost(f);
    const cycles = f.asm.filter(inst => inst.type === ASM_TYPE.OP).map(inst => inst.debug.cycle);
    expect(cycles).toEqual([1, 2, 4, 1, 2]);
  });
});
