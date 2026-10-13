/**
* @copyright 2026 - Max Bebök
* @license Apache-2.0
*/
import {ASM_TYPE} from "../intsructions/asmWriter.js";
import {asmGetReorderIndices, OP_FLAG_IS_BRANCH, OP_FLAG_IS_NOP, ACC_BIT} from "./asmScanDeps.js";
import {relocateElement} from "./asmOptimizer.js";

export const CHAIN_MOVES = !(typeof process !== "undefined" && process.env && process.env.RSPL_CHAIN_MOVES === "0");

/**
 * An op that writes $acc without reading it starts a chain, 
 * every following reader extends it. 
 * A chain with a label or branch between its members is not movable.
 * @param {ASM[]} lines
 * @return {{chains: {ids: number[], movable: boolean, size: number}[], chainOf: number[]}}
 */
export function buildChains(lines)
{
  const chains = [];
  const chainOf = new Array(lines.length).fill(-1);
  for(const inst of lines)inst.chain = undefined;

  let cur = null;
  for(let id = 0; id < lines.length; ++id) 
  {
    const o = lines[id];
    if(o.type !== ASM_TYPE.OP || (o.opFlags & OP_FLAG_IS_NOP))continue;
    const reads = (o.depsSourceMask & ACC_BIT) !== 0n;
    const writes = (o.depsTargetMask & ACC_BIT) !== 0n;
    if(!reads && !writes)continue;

    if(!reads || !cur) {
      cur = {ids: [], movable: true, size: 0};
      chains.push(cur);
    }
    cur.ids.push(id);
    cur.size++;
    chainOf[id] = chains.length - 1;
    o.chain = cur;
  }

  for(const c of chains) 
  {
    for(let id = c.ids[0]; id <= c.ids[c.ids.length-1]; ++id) 
    {
      const o = lines[id];
      if(o.type !== ASM_TYPE.OP || (o.opFlags & OP_FLAG_IS_BRANCH)) { 
        c.movable = false; break; 
      }
    }
  }

  return {chains, chainOf};
}

/**
 * Move the whole chain of the item at "leaderPos". 
 * the leader lands on `target`, the other members are pulled behind it one by one. 
 * The leader must be the last member for a forward move, the first for a backward one
 * @return {boolean} false (list untouched) when a member cannot reach its slot
 */
export function relocateChain(lines, leaderPos, target)
{
  if(leaderPos === target)return true;
  const leader = lines[leaderPos];
  const chain = leader.chain;
  if(!chain)return false;
  const fwd = target > leaderPos;

  const members = lines.filter(o => o.chain === chain);
  if((fwd ? members[members.length-1] : members[0]) !== leader)return false;

  const backup = [...lines];
  relocateElement(lines, leaderPos, target);
  let anchor = lines.indexOf(leader);
  const range = {maskAcc: true};

  for(let k = 1; k < members.length; ++k) 
  {
    const follower = fwd ? members[members.length-1-k] : members[k];
    const pos = lines.indexOf(follower);
    asmGetReorderIndices(lines, pos, range);
    const ok = fwd
      ? pos < anchor && range.plainHi >= anchor
      : pos > anchor && range.plainLo <= anchor + 1;

    if(!ok) {
      lines.length = 0;
      lines.push(...backup);
      return false;
    }
    relocateElement(lines, pos, fwd ? anchor : anchor + 1);
    anchor = lines.indexOf(follower);
  }
  return true;
}

