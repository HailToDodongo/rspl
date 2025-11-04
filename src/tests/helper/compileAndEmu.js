import {transpileSource} from "../../lib/transpiler";
import {createRSP} from 'rsp-wasm';
import {assemble} from 'armips';

const CONF = {rspqWrapper: false};

/**
 * Compile RSPL into IMEM/DMEM buffers
 * @param {string} source
 * @return {Promise<RSP>}
 */
export async function compileAndCreateEmu(source)
{
  let {asm, warn} = await transpileSource(source, CONF);
  if(warn)throw new Error(warn);

  // @TODO: add proper armips support
  for(let i=0; i<8; ++i) {
    asm = asm.replaceAll(".e"+i, "["+i+"]");
  }

  const asmSrc = `
    .rsp 
    
    .create "dmem", 0x0000
      TEST_CONST: .db 0x11
      .db 0x22
      .db 0x33
      .db 0x44
      .align 2
    .close
    
    .create "imem", 0x1000
        ${asm}
    
        RSPQ_Loop:
          j RSPQ_Loop
          nop
    .close
    `;

  const files = await assemble(asmSrc);
  return await createRSPEmu(files.imem, files.dmem);
}

/**
 * Creates an RSP emulation instance
 * @param {Uint8Array} imem
 * @param {Uint8Array} dmem
 * @return {Promise<RSP>}
 */
export async function createRSPEmu(imem, dmem)
{
  const rsp = await createRSP();

  const imemView = new DataView(imem.buffer);
  for(let i=0; i<imem.byteLength; i += 4) {
    rsp.IMEM.setUint32(i, imemView.getUint32(i, false), true);
  }

  const dmemView = new DataView(dmem.buffer);
  for(let i=0; i<dmem.byteLength; i += 4) {
    rsp.DMEM.setUint32(i, dmemView.getUint32(i, false), true);
  }

  rsp.setVPR("$v30", [0x0080, 0x0040, 0x0020, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001]);
  rsp.setVPR("$v31", [0x8000, 0x4000, 0x2000, 0x1000, 0x0800, 0x0400, 0x0200, 0x0100]);

  return rsp;
}
