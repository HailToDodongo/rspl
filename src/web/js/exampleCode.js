/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

export const EXAMPLE_CODE = `include "rsp_queue.inc"
include "rdpq_macros.h"

state
{
  vec32 OFFSET;
  vec32 SCALE; 
  
  vec16 COLOR_BUFF[128];
  u32 ITERATIONS;
}

macro flushScreen(u32 ptrScreen, u32 copySize)
{
  u32<$t0> dmaSize = copySize;
  // async, the next time this data-slice is touched is in the next frame
  dma_out_async(COLOR_BUFF, ptrScreen, dmaSize);
  ptrScreen += copySize;
  ptrScreen += copySize; // skip every other line
}

macro complexMul(vec32 res, vec16 mask) 
{
  const vec32 resSq = res * res;
  const vec32 resSqDiff = resSq - resSq.yywwYYWW;
  
  vec32 res2 = res * res.xxzzXXZZ;
  res2 += res2;

  res = mask != 0 ? res2 : resSqDiff;
}

macro mandelbrot(vec16 color, vec32 c, u32 maxIter, vec16 maskOddEven)
{
  vec32 res = 0;
  
  // 1=unset, 0=done, pre-shifted for easy color values
  vec16 colorMaskAdd = 0b1'0000'0000;

  u32 i = maxIter - 1;
  u32 iEnd = 0xFFFF;
  vec16 isOutside;
  
  LOOP: // @TODO: implement do-while loop
  {
    i -= 1;
    complexMul(res, maskOddEven);
    res += c;
    
    // test if we can stop the loop
    isOutside = res:uint + res:uint.yywwYYWW;
    isOutside:sfract *= isOutside; //distance check
    
    // lanes .xzXZ become either 0x0000 or 0xFFFF
    isOutside = isOutside >= 1 ? maskOddEven : maskOddEven.w;
    
    // mask out color-addend (if set), and apply to the color
    colorMaskAdd &= isOutside;
    color += colorMaskAdd;
    
    const u32 maskA = colorMaskAdd.x;
    const u32 maskB = colorMaskAdd.z;
    const u32 maskC = colorMaskAdd.X;
    const u32 maskD = colorMaskAdd.Z; 
    
    // if all pixels are set (mask=0), stop
    // otherwise use the large number as the loop condition
    iEnd = maskA | maskB;
    iEnd |= maskC;
    iEnd |= maskD;
    
    // vector version (slower)
    //vec16 maskAll = colorMaskAdd | colorMaskAdd.zzzzZZZZ;
    //maskAll |= maskAll.X; // result is in .x

    // Abuse undeflow here, once 'i' is below zero, it's bigger than 'iEnd'.
    // The other exit condition sets 'iEnd' itself to zero.
    if(i < iEnd)goto LOOP;
  }
}

command<0> Cmd_Render(u32 ptrScreen, u32 sizeXY, u32 isOddLine)
{
  u32<$s4> _; // reserved for dma stuff
  
  const u32 maxIter = load(ITERATIONS);
  s32 sizeY = sizeXY & 0xFFFF;
  
  // bytes to copy per batch, a batch is a screen-line
  u32 copySize = sizeXY >> 16;
  copySize *= 2; // 2-bytes per pixel
  
  // internal buffer in DMEM, start/end for a single batch
  u32 colorBuff = COLOR_BUFF;
  const u32 colorBuffEnd = colorBuff + copySize;
  
  const vec32 posScale = load(SCALE);
  const vec32 offset = load(OFFSET);
  
  vec32 incX = 0;
  incX:sint.x = 4;
  incX:sint.z = 4;
  incX:sint.X = 4;
  incX:sint.Z = 4;
  
  vec32 incY = 0;
  incY:sint.y = 2;
  incY:sint.w = 2;
  incY:sint.Y = 2;
  incY:sint.W = 2;
  
  incX *= posScale;
  incY *= posScale;

  vec32 pos = 0;
  //pos:sint.x = 0;
  pos:sint.z = 1;
  pos:sint.X = 2;
  pos:sint.Z = 3;
  
  if(isOddLine) {
    ptrScreen += copySize;
    pos:sint.y = 1;
    pos:sint.w = 1;
    pos:sint.Y = 1;
    pos:sint.W = 1;
  }
  
  pos *= posScale;
  pos += offset;

  // mask used in complexMul() & mandelbrot loop as a color mask
  vec16 maskOddEven = 0;
  maskOddEven.y = 0xFFFF;
  maskOddEven.w = maskOddEven.y;
  maskOddEven.Y = maskOddEven.y;
  maskOddEven.W = maskOddEven.y;
  
  vec32 posOrgX = pos;

  while(sizeY != 0)
  {
    while(colorBuff != colorBuffEnd)
    {
      vec16 color = 0;
      mandelbrot(color, pos, maxIter, maskOddEven);

      store_vec_s8(color, colorBuff);
      
      colorBuff += 0x08;
      pos += incX;
    }
    
    flushScreen(ptrScreen, copySize);
    
    posOrgX += incY;
    colorBuff = COLOR_BUFF;
    sizeY -= 2;
    pos = posOrgX;
  }
  
}

command<1> Cmd_SetScale(u32 iter, s32 scaleXY, s32 offsetX, s32 offsetY)
{
  vec32 scale = 0;
  scale.x = scaleXY;
  scale.y = scaleXY;
  scale.z = scale.x;
  scale.w = scale.y;
  scale.X = scale.x;
  scale.Y = scale.y;
  scale.Z = scale.x;
  scale.W = scale.y;
  
  store(scale, SCALE);
  
  vec32 offset = 0;
  offset.x = offsetX;
  offset.y = offsetY;
  offset.z = offset.x;
  offset.w = offset.y;
  offset.X = offset.x;
  offset.Y = offset.y;
  offset.Z = offset.x;
  offset.W = offset.y;
  
  store(offset, OFFSET);
  
  iter &= 0xFFFF;
  store(iter, ITERATIONS);
}

include "rsp_rdpq.inc"
`;