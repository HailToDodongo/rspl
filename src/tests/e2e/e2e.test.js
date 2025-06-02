import {describe, expect, jest, test} from '@jest/globals';
import {compileAndCreateEmu} from "../helper/compileAndEmu.js";

describe('E2E', () =>
{
  // @TODO: write proper tests, add basic armips support
  test('Test', async () =>
  {
    const rsp = await compileAndCreateEmu(`
      function main() {
         vec16<$v02> b = 2;
         b.x = 4;
         b *= 2;
    }`);
    expect(rsp.getVPR("$v02")).toEqual([0,0,0,0, 0,0,0,0]);
    rsp.step(3);
    expect(rsp.getVPR("$v02")).toEqual([8,4,4,4, 4,4,4,4]);
  });
});
