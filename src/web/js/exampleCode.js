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
    vec32 resSq = res * res;
    vec32 resSqDiff = resSq - resSq.yywwYYWW;
    
    vec32 res2 = res * res.xxzzXXZZ;
    res2 += res2;

    res = mask != 0 ? res2 : resSqDiff;
}

macro mandelbrot(vec16 color, vec32 c, u32 maxIter, vec16 maskMulInv, u32 maskAllSet)
{
  vec32 res = 0;
  s32 maskDone = 0;
  u32 iteration = maxIter;
  
  while(iteration != 0) 
  {
    complexMul(res, maskMulInv);
    res += c;
    
    // test if we can stop the loop
    vec16 isOutside = res:uint + res:uint.yywwYYWW;
    isOutside:sfract *= isOutside;
    isOutside = isOutside < 1;

    // 4 pixels are prcessed at the same time across lanes
    // keep track which pixel is ready, and stop only if all are
    s32 isOutsideA = isOutside.x;
    s32 isOutsideB = isOutside.z;
    s32 isOutsideC = isOutside.X;
    s32 isOutsideD = isOutside.Z;

    if(isOutsideA) 
    {
      // only do this once per pixel to lock in the correct color
      u32 isDone = maskDone & 0b0001;
      if(!isDone) {
        color.x = iteration;
        maskDone |= 0b0001;
        if(maskDone == maskAllSet)break;
      }
    }
    
    if(isOutsideB) 
    {
      u32 isDone = maskDone & 0b0010;
      if(!isDone) {
        color.z = iteration;
        maskDone |= 0b0010;
        if(maskDone == maskAllSet)break;
      }
    }
    
    if(isOutsideC) 
    {
      u32 isDone = maskDone & 0b0100;
      if(!isDone) {
        color.X = iteration;
        maskDone |= 0b0100;
        if(maskDone == maskAllSet)break;
      }
    }
    
    if(isOutsideD) 
    {
      u32 isDone = maskDone & 0b1000;
      if(!isDone) {
        color.Z = iteration;
        maskDone |= 0b1000;
        if(maskDone == maskAllSet)break;
      }
    }
  
    iteration -= 4096; // 1 << 12
  }
}

command<0> Cmd_Render(u32 ptrScreen, u32 sizeXY, u32 isOddLine)
{
  u32<$s4> _; // reserved for dma stuff
  
  u32 maxIter = load(ITERATIONS);
  s32 sizeY = sizeXY & 0xFFFF;
  
  // bytes to copy per batch, a batch is a screen-line
  u32 copySize = sizeXY >> 16;
  copySize *= 2; // 2-bytes per pixel
  
  // internal buffer in DMEM, start/end for a single batch
  u32 colorBuff = COLOR_BUFF;
  u32 colorBuffEnd = colorBuff + copySize;
  
  vec32 posScale = load(SCALE);
  vec32 offset = load(OFFSET);
  
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

  // mask used in complexMul()
  vec16 maskMul = 0;
  maskMul.y = 0xFFFF;
  maskMul.w = maskMul.y;
  maskMul.Y = maskMul.y;
  maskMul.W = maskMul.y;
  
  u32 maskAllSet = 0b1111; // full per-pixel mask (4 at a time)
  vec32 posOrgX = pos;

  while(sizeY != 0)
  {
    while(colorBuff != colorBuffEnd)
    {
      vec16 color = 0;
      mandelbrot(color, pos, maxIter, maskMul, maskAllSet);

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
  iter <<= 12;
  store(iter, ITERATIONS);
}


include "rsp_rdpq.inc"
`;