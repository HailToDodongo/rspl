/**
* @copyright 2026 - Max Bebök
* @license Apache-2.0
*/
import {getAsmLabels, getAttrLoaderLabel, getAttrPatchLabel, stringifyInstr} from './utils';

/**
 * 
 * @param {ASMFunc[]} functionsAsm
 */
function collectAttrLoaders(functionsAsm) {
  const res = {};

  const addEntry = (attrName) => {
    if(!res[attrName]) {
      res[attrName] = {
        loaders: [],
        patches: []
      };
    }
  };

  for(const asmFunc of functionsAsm) {
    for(const asm of asmFunc.asm) {
      if (asm.attrLoader) {
        addEntry(asm.attrLoader);
        res[asm.attrLoader].loaders.push(asm);
      }
      if (asm.attrPatch) {
        addEntry(asm.attrPatch.name);
        res[asm.attrPatch.name].patches.push(asm);
      }
    }
  }
  return res;
}

/**
 * 
 * @param {AST} ast 
 * @param {ASMFunc[]} functionsAsm
 * @param {(line: any) => void} writeLine 
 * @param {(line: any[]) => void} writeLines 
 * @returns 
 */
export function writeMagmaHeader(ast, functionsAsm, writeLines, writeStateVar) {
  let totalSaveByteSize = 0;

  writeLines(["", "MgBeginShaderUniforms"]);
  for(const uniform of ast.uniforms) {
    writeLines(`  MgBeginUniform ${uniform.name}, ${uniform.binding}`);
    for(const stateVar of uniform.state) {
      if(stateVar.extern)continue;
      totalSaveByteSize += writeStateVar(stateVar, writeLines);
    }
    writeLines(["  MgEndUniform", ""]);
  }
  writeLines(["MgEndShaderUniforms", ""]);

  const attrLoaders = collectAttrLoaders(functionsAsm);
  
  writeLines("MgBeginVertexInput");
  for(const attribute of ast.attributes) {
    writeLines(`  MgBeginVertexAttribute ${attribute.binding}, ${attribute.optional ? 1 : 0}`);
    const loaders = attrLoaders[attribute.name]?.loaders;
    if(typeof loaders !== 'undefined' && loaders.length > 0) {
      writeLines("    MgVertexAttributeLoaders " + loaders.map(getAttrLoaderLabel).join(", "));
    }
    const patches = attrLoaders[attribute.name]?.patches;
    if(patches) {
      for(const patch of patches) {
        writeLines(`    MgBeginVertexAttributePatch ${getAttrPatchLabel(patch)}`);
        writeLines(`      ${patch.attrPatch.op || "nop"}`);
        writeLines("    MgEndVertexAttributePatch");
      };
    }
    writeLines(["  MgEndVertexAttribute", ""]);
  }
  writeLines(["MgEndVertexInput", ""]);

  writeLines("MgBeginShader");

  return totalSaveByteSize;
}