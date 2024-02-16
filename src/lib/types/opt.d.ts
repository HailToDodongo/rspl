/**
 * NOTE: this project does NOT use TypeScript.
 * Types defined here are solely used by JSDoc.
 */

import './types';
import './asm';

declare global
{
  type OptInstruction = {
    latency: number;
    asm: ASM;
  };
}