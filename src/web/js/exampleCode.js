/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

export const EXAMPLE_CODE = `/***************************************
 *  _____ _             _____ ____     *
 * |_   _(_)_ __  _   _|___ /|  _ \\    *
 *   | | | | '_ \\| | | | |_ \\| | | |   *
 *   | | | | | | | |_| |___) | |_| |   *
 *   |_| |_|_| |_|\\__, |____/|____/    *
 *                |___/                *
 *      3D-microcode for libdragon     *
 *                                     *
 * @copyright Max Bebök 2023-2024      *
 * @license MIT                        *
 ***************************************/

#define MATRIX_STACK_SIZE 2
#define TRI_BUFFER_COUNT 64
#define LIGHT_COUNT 8

// settings for RDPQ_Triangle
#define RDPQ_TRIANGLE_CUSTOM_VTX 1
#define VTX_ATTR_XY          0x00
#define VTX_ATTR_Z           0x04
#define VTX_ATTR_CLIPFLAGS   0x06
#define VTX_ATTR_REJFLAGS    0x07
#define VTX_ATTR_RGBA        0x08
#define VTX_ATTR_ST          0x0C
#define VTX_ATTR_CLIPPOSi    0x10
#define VTX_ATTR_Wi          0x16
#define VTX_ATTR_CLIPPOSf    0x18
#define VTX_ATTR_Wf          0x1E
#define VTX_ATTR_INVWi       0x20
#define VTX_ATTR_INVWf       0x22

// Size of the internal triangle format
// @TODO: add const expr. eval to RSPL
#define TRI_SIZE   36
#define TRI_SIZE_2 72
#define TRI_SIZE_3 108

// Single light (RGBA duplicated, duplicated direction packed as s8)
#define LIGHT_SIZE 16

// Debug-Flag, used to measure performance excl. triangle draws
#define DRAW_TRIS 1
// Debug-Flag, enables metrics on how many tris are drawn & clipped
//#define DEBUG_LOG_TRIS 1

// RDPQ Stuff (@TODO: move RDPQ_Send back into libdragon)
#define DP_STATUS_END_VALID 512
#define RDPQ_DYNAMIC_BUFFER_SIZE 65536

include "rsp_queue.inc"
include "rdpq_macros.h"

state
{
  // external libdragon labels
  extern u32 RDPQ_CURRENT;
  extern u32 RDPQ_SENTINEL;
  extern u32 RDPQ_DYNAMIC_BUFFERS;
  extern u8 RDPQ_SYNCFULL_ONGOING;
  extern u32 RDPQ_OTHER_MODES;
  extern u16 RSPQ_Loop;
  extern u16 RSPQCmd_RdpAppendBuffer;
  extern u16 RSPQCmd_RdpSetBuffer;
  extern u16 RSPQ_SCRATCH_MEM;

  vec16 MAT_MODEL_DATA[MATRIX_STACK_SIZE][4]; // model matrices

  vec16 MAT_PROJ_DATA[4]; // one projection matrix
  vec16 MAT_MODEL_RROJ[4]; // model * projection
  vec16 MAT_MODEL_NORM[2]; // normal * projection (@TODO: only needs 1.5 vec16's)

  /**
   * Buffer format (RDPQ_Triangle compatible):
   *
   *   Type |     Name  | Offset
   * -------+-----------+--------
   * s16[2] | Pos-XY    | 0x00
   * s16    | Depth     | 0x04
   * u8     | Clip-Code | 0x06  (t3d specific)
   * u8     | Rej. Code | 0x07  (t3d specific)
   * u8[4]  | Color     | 0x08
   * s16[2] | UV        | 0x0C
   * s16[3] | Clip-Pos  | 0x10  (t3d specific)
   * s16    | W         | 0x16
   * f16[3] | Clip-Pos  | 0x18  (t3d specific)
   * f16    | W         | 0x1E
   * s16.16 | InvW      | 0x20
   * -------+-----------+-0x24---
   */
  u8 TRI_BUFFER[TRI_BUFFER_COUNT][TRI_SIZE];

  // Temp. buffer for clipped extra vertices.
  // This is also used as a temp. buffer for DMA'ing in new vertices.
  // For clipping data, the format is the same as TRI_BUFFER.
  alignas(8)
  u8 CLIP_BUFFER_TMP[7][TRI_SIZE];

  // current screen size (.xyz) and offset (.XYZ)
  vec16 SCREEN_SCALE_OFFSET;

  vec16 NORMAL_MASK_SHIFT = {
    // Mask to extract 5.6.5-bit values for normal vectors (.xyzw)
    0b11111'000000'00000,
    0b00000'111111'00000,
    0b00000'000000'11111,
    0,
    // And the shift to transform them into a fraction (.XYZW)
    1, 32, 2048, 0
  };

  // Last buffer where final clipping triangles are stored.
  // During clipping, vertices alternate between CLIP_BUFFER_TMP & B.
  u8 CLIP_BUFFER_RESULT[8][TRI_SIZE];

  // Lighting
  u32 COLOR_AMBIENT[2];   // RGBA8 (duplicated)
  alignas(4) u8 LIGHT_DIR_COLOR[LIGHT_SIZE][LIGHT_COUNT]; // RGBA8 (duplicated) | Direction packed as s8 (duplicated)

  #ifdef DEBUG_LOG_TRIS
    u16 DEBUG_TRI_COUNT[4] = {0};
  #endif

  u32 TRI_COMMAND = {0}; // for RDPQ_Triangle

  // Fog settings: index 0/1 is the scale as a s16.16, index 2 the scale as a s16 int.
  // The last value is used as a limiter to prevent overflows.
  s16 FOG_SCALE_OFFSET[4] = {0, 0, 0, 32767};

  u16 CURRENT_MAT_ADDR = {0};

  // Plane-normals for clipping, the guard value can be overwritten with a command.
  s8 CLIPPING_PLANES[5][4] = {
  // X  Y  Z | guard
     1, 0, 0,   1,
     0, 1, 0,   1,
     0, 0, 1,   1,
     1, 0, 0,  -1,
     0, 1, 0,  -1
  };

  u8 USE_REJECT   = {0}; // 0=use-clipping, 1=use-rejection
  u8 FACE_CULLING = {0}; // 0=cull front, 1=cull back, 2=no-culling
  u8 USE_FOG      = {0}; // 0=no-fog, 1=use-fog

  u8 ACTIVE_LIGHT_SIZE = {0}; // light count * light size
}

// Libdragon functions
function DMAIn(u32<$t0> size, u32<$t1> pitch, u32<$s0> rdram, u32<$s4> dmem);
function DMAOut(u32<$t0> size);

// function RDPQ_Send(u32<$s4> buffStart, u32<$s3> buffEnd);
function RSPQ_RdpWait();

// original version from libdragon...
function RDPQ_Triangle(
  u32<$a0> triCmd,
  u32<$a1> ptrVert0, u32<$a2> ptrVert1, u32<$a3> ptrVert2,
  u8<$v0> cull, u32<$s3>buffOut
);

// ...modified copy found in 'rspq_triangle.inc'
function RDPQ_Tri(
  u32<$a0> ptrVert0, u32<$a1> ptrVert1, u32<$a2> ptrVert2,
  u32<$s3> buffOut
);

macro debugLogClip() {
#ifdef DEBUG_LOG_TRIS
  u16 tmp = load(DEBUG_TRI_COUNT, 2); tmp += 1;
  store(tmp, DEBUG_TRI_COUNT, 2);
#endif
}

macro debugLogTri() {
#ifdef DEBUG_LOG_TRIS
  u16 tmp = load(DEBUG_TRI_COUNT); tmp += 1;
  store(tmp, DEBUG_TRI_COUNT);
#endif
}

// Packs +xyz & -xyz into a single byte
macro packClipCode(u32 res, u32 clipCode)
{
  res = clipCode & 0b0000'0111'0000'0111;
  u32 tmp = res >> 5; //  -zyx      +xyx
  res |= tmp;
}

// Same as packClipCode, inverts the clipcode to only use one operation later on
macro packClipCodeInverted(u32 res, u32 clipCode)
{
  res = clipCode & 0b0000'0111'0000'0111;
  u32 tmp = res >> 5; //  -zyx      +xyx
  res = res ~| tmp;
}

// Fractional 3D dot-product with two vectors at once, result is in 'res.x' and 'res.X'
macro dotXYZ(vec16 res, vec16 a, vec16 b)
{
  res:sfract = a * b;
  vec16 tmp:sfract = res + res.yyyyYYYY;
  res:sfract += tmp.zzzzZZZZ;
}

// 4D dot-product with two vectors at once, result is in 'res.x' and 'res.X'
macro dotXYZW(vec32 res, vec32 a, vec32 b)
{
  res = a * b;
  res += res.yywwYYWW;
  res += res.zzzzZZZZ;
}

// Loads currently active model matrix (premultiplied with proj. matrix) into registers
// This already duplicates it for 2-vector operations.
macro loadCurrentModelProjMat(vec32 mat0, vec32 mat1, vec32 mat2, vec32 mat3)
{
  u16 address = MAT_MODEL_RROJ;
  mat0 = load(address, 0x00).xyzwxyzw;
  mat1 = load(address, 0x10).xyzwxyzw;
  mat2 = load(address, 0x20).xyzwxyzw;
  mat3 = load(address, 0x30).xyzwxyzw;
}

// Loads currently active model matrix into registers
// This already duplicates it for 2-vector operations.
macro loadCurrentNormalMat(vec16 mat0, vec16 mat1, vec16 mat2)
{
  u16 address = MAT_MODEL_NORM;
  mat0 = load(address, 0x00).xyzwxyzw;
  mat1 = load(address, 0x08).xyzwxyzw;
  mat2 = load(address, 0x10).xyzwxyzw;
}

/**
 * Loads & transforms 3D vertices into the internal buffer, later to be used by triangles.
 * This fully prepares them to be used by RDPQ_Triangle, while still being able to
 * be clipped and transformed into screen-space again if needed.
 *
 * @param bufferSize size in bytes to load (@TODO: add offset too)
 * @param rdramVerts RDRAM address to load vertices from
 */
command<4> T3DCmd_VertLoad(u32 bufferSize, u32 rdramVerts)
{
  // load all vertices in a single DMA, processing them as the loop goes.
  // NOTE: the input vertex-size is smaller than the internal one, so we can't do it in place.
  // Instead, offset the buffer enough to not cause an overlap with read/writes on the same vertex.
  u32<$s4> prt3d = TRI_BUFFER;
  prt3d += 1392; // (@TODO calc. dynamically on CPU)

  u32<$t0> copySize = bufferSize & 0xFFFF;
  u32 ptr3dEnd = prt3d + copySize;
  dma_in_async(prt3d, rdramVerts, copySize);

  vec32 mat0, mat1, mat2, mat3;
  loadCurrentModelProjMat(mat0, mat1, mat2, mat3);

  vec16 matN0, matN1, matN2;
  loadCurrentNormalMat(matN0, matN1, matN2);

  vec16 colorAmbient = load_vec_u8(COLOR_AMBIENT);

  vec16 normMask = load(NORMAL_MASK_SHIFT, 0x00).xyzwxyzw;
  vec16 normShift = load(NORMAL_MASK_SHIFT, 0x08).xyzwxyzw;

  vec16 guardBandScale = load(CLIPPING_PLANES).xy;
  guardBandScale = guardBandScale.y;

  vec16 screenSize = load(SCREEN_SCALE_OFFSET).xyzwxyzw;
  vec16 screenOffset = load(SCREEN_SCALE_OFFSET, 0x08).xyzwxyzw;
  vec16 fogScaleOffset = load(FOG_SCALE_OFFSET).xyzw;

  u32 ptrBuffA = TRI_BUFFER; // ptr to first output vertex
  u32 ptrBuffB = ptrBuffA + TRI_SIZE; // ptr to second output vertex

  u16 ptrLight = LIGHT_DIR_COLOR;

  u8 ptrLightEnd = load(ACTIVE_LIGHT_SIZE);
  ptrLightEnd += ptrLight;

  // @TODO: racing the DMA is ~40us faster, doesn't work in ares, retest later again
  dma_await();

  // Process all vertices and apply transformation & lighting.
  // This always handles 2 vertices at once, most sitting in one register.
  loop
  {
    // Position & Normals
    vec32 posClip;
    vec16 color;
    {
      vec16 pos = load(prt3d, 0x00);
      // load & transform normals, this in not needed when no directional light is present
      // however, an if-statement + the lost ability for reordering is not worth it. (@TODO: do more tests)
      vec16 norm = normMask & pos.wwwwWWWW;
      norm *= normShift;

      vec16 normNew;
      normNew = matN0:sfract  * norm.xxxxXXXX;
      normNew = matN1:sfract +* norm.yyyyYYYY;
      norm    = matN2:sfract +* norm.zzzzZZZZ;

      pos.w = load(CLIPPING_PLANES, 4).x; // loads "1"
      pos.W = load(CLIPPING_PLANES, 4).x;

      // object-space to clip-space
      mulMat4Vec8(mat0, mat1, mat2, mat3, pos, posClip);
      undef pos;

      // input vertex color
      color = load_vec_u8(prt3d, 0x10);
      vec16 lightColor = colorAmbient; // light color, accumulates directional lights

      // directional
      while(ptrLight != ptrLightEnd)
      {
        vec16 lightDirColor = load_vec_u8(ptrLight);
        vec16 lightDirVec   = load_vec_s8(ptrLight, 8);

        vec16 lightDirScale;
        dotXYZ(lightDirScale, norm, lightDirVec);

        lightDirScale = lightDirColor:ufract * lightDirScale:ufract.xxxxXXXX;

        lightColor:sfract += lightDirScale;
        ptrLight += LIGHT_SIZE;
      }

      ptrLight = LIGHT_DIR_COLOR;
      color:sfract *= lightColor:ufract;
    }

    // before we overwrite W, save clip-pos
    store(posClip.xyzw, ptrBuffA, VTX_ATTR_CLIPPOSi);
    store(posClip.XYZW, ptrBuffB, VTX_ATTR_CLIPPOSi);

    // calc. clipping codes (rejection & clip-prevention)
    u32 clipCodeA, clipCodeB, rejCodesA, rejCodesB;
    {
      vec32<$v02> clipPlaneW = guardBandScale * posClip.wwwwWWWW;

      // Clip code for clipping (incl. band-guard)
      u32 clipCodes = clip(posClip, clipPlaneW);
      clipCodeB = clipCodes >> 4;
      packClipCode(clipCodeA, clipCodes);
      packClipCode(clipCodeB, clipCodeB);

      // Clip-Code for rejection (no band-guard)
      u32 rejCodes = clip(posClip, posClip.wwwwWWWW);
      rejCodesB = rejCodes >> 4;
      packClipCodeInverted(rejCodesA, rejCodes);
      packClipCodeInverted(rejCodesB, rejCodesB);
    }

    // now optimistically assume it's on-screen and conv. to screen-space
    posClip.w = invert_half(posClip).w;
    posClip.W = invert_half(posClip).W;

    // Fog
    u8 useFog = load(USE_FOG);
    {
      vec32 fog;

      // add offset, to avoid loads, int/fract are stored in the same vector
      fog:ufract = posClip + fogScaleOffset.z;
      fog:sint   = posClip + fogScaleOffset.y;
      fog *= fogScaleOffset:sint.x;

      vec16 fogMax = fogScaleOffset.w; // invert fog -> 0=no-fog, max=fog
      fog:sint = fogMax - fog:sint;

      if(useFog) {
        color.w = fog:sint.z; // store fog as alpha
        color.W = fog:sint.Z;
        //printf("%v: %v\\n", fogScaleOffset.x, color.w);
      }
    }

    // backup clip-pos in case that clipping is required
    // also store inv-W here (in place of W)
    store(posClip.w, ptrBuffA, VTX_ATTR_INVWi);
    store(posClip.W, ptrBuffB, VTX_ATTR_INVWi);

    posClip *= posClip.wwwwWWWW;
    posClip *= screenSize;
    posClip:sint += screenOffset;

    @Barrier("pos-cc") store(posClip:sint.xyzw, ptrBuffA, VTX_ATTR_XY);
    @Barrier("pos-cc") store(posClip:sint.XYZW, ptrBuffB, VTX_ATTR_XY);

    @Barrier("uv") store_vec_u8(color.x, ptrBuffA, VTX_ATTR_RGBA);
    @Barrier("uv") store_vec_u8(color.X, ptrBuffB, VTX_ATTR_RGBA);

    undef posClip;

    // copy UV over,we re-use "color" here to prevent any wrong reordering,
    // color overlaps UV during a save
    vec16 uv = load(prt3d, 0x18).xyzw;
    @Barrier("uv") store(uv.xy, ptrBuffA, VTX_ATTR_ST);
    @Barrier("uv") store(uv.zw, ptrBuffB, VTX_ATTR_ST);

    prt3d += 0x20;

    @Barrier("pos-cc") store(clipCodeA:u8, ptrBuffA, VTX_ATTR_CLIPFLAGS);
    @Barrier("pos-cc") store(clipCodeB:u8, ptrBuffB, VTX_ATTR_CLIPFLAGS);

    @Barrier("pos-cc") store(rejCodesA:u8, ptrBuffA, VTX_ATTR_REJFLAGS);
    @Barrier("pos-cc") store(rejCodesB:u8, ptrBuffB, VTX_ATTR_REJFLAGS);

    ptrBuffA += TRI_SIZE_2;
    ptrBuffB += TRI_SIZE_2;

  } while(prt3d != ptr3dEnd)
}

/**
 * Calculates the intersection point against a clipping plane.
 * ---- Output ----
 * @param resPosUV    new point / UV (in .xyzw / .XY)
 * @param resColor    new color (in .xyzw)
 * @param newClipCode new clip-code for the interpolated-point
 *
 * ---- Input ----
 * @param planePtr  DMEM address for the plane normals
 *
 * @param posBase   base point inside the frustum
 * @param colorBase base color inside the frustum
 * @param pos       position/UV to be clipped
 * @param color     color to be clipped
 */
macro intersection(
  vec32 resPosUV, vec16 resColor, u32 newClipCode,
  u32 planePtr,
  vec32 posBase, vec16 colorBase,
  vec32 pos, vec16 color
) {
  {
    vec16 colorDiff = color - colorBase;
    vec32 posUVDiff = pos - posBase;

    vec32 planeNorm:sfract = 0;

    planeNorm:sint = load_vec_s8(planePtr);
    planeNorm >>= 8;
    planeNorm:sint.XYZW = planeNorm:sint.xyzw;

    vec32 vpos = posBase;
    vpos.XYZW = pos.xyzw;

    vec32 dot;
    dotXYZW(dot, vpos, planeNorm);

    vec32 fac = dot - dot.X;

    fac.x = invert_half(fac).x;
    fac += fac;
    fac *= dot.x;
    //printf("%v d:%v\\t", fac.x, dot.x);

    // From now on we only need the fractional part.
    // Prevent overflow (>1.0) by checking the integer part
    // and then clamping the fraction if it's not in 0<=x<1.
    // @TODO: check if both checks are really needed
    vec16 fractMax:sint = VZERO - 1; // puts 0.99.. into 'fractMax'
    fac:ufract = fac:sint < 1 ? fac:ufract : fractMax:ufract.y;
    fac:ufract = fac:sint >= 0 ? fac:ufract : VZERO;

    dot = posUVDiff * fac:sfract.x;
    // @TODO: add auto opt. for this in RSPL
    resPosUV = posBase +* 1; // abuse add-mul to add diff onto pos.

    colorDiff:sfract *= fac:sfract.x;
    resColor:uint = colorBase + colorDiff;
  }

  newClipCode = clip(resPosUV, resPosUV.w);
  packClipCode(newClipCode, newClipCode);
}

/**
 * Copies a vertex, only includes data relevant for clipping.
 * @param ptrDst DMEM address to copy to
 * @param ptrSrc DMEM address to copy from
 */
function copyClippingVertex(u32<$s0> ptrDst, u32<$s1> ptrSrc)
{
  vec16 tmp1 = load(ptrSrc, 0x00).xyzw;
  vec16 tmp2 = load(ptrSrc, 0x08).xyzw;
  vec16 tmp3 = load(ptrSrc, 0x10).xyzw;
  vec16 tmp4 = load(ptrSrc, 0x18).xyzw;
  store(tmp1.xyzw, ptrDst, 0x00);
  store(tmp2.xyzw, ptrDst, 0x08);
  store(tmp3.xyzw, ptrDst, 0x10);
  store(tmp4.xyzw, ptrDst, 0x18);

  ptrDst += TRI_SIZE;
}

/**
 * Emits a clipped triangle into the current clipping-buffer.
 * Note: this advances the buffer-pointer by TRI_SIZE.
 *
 * @param ptrWrite destination buffer
 * @param lastClipCode CC to save
 * @param lastPosUV  pos + UV to save
 * @param lastColor color to save
 */
function emitClippedTri(u32<$s2> ptrWrite, u8<$s0> lastClipCode, vec32<$v26> lastPosUV, vec16<$v28> lastColor)
{
  @Barrier("uv") store_vec_u8(lastColor,  ptrWrite, VTX_ATTR_RGBA);
  @Barrier("uv") store(lastPosUV:sint.XY, ptrWrite, VTX_ATTR_ST);

  store(lastClipCode:u8,   ptrWrite, VTX_ATTR_CLIPFLAGS);
  store(lastPosUV.xyzw,    ptrWrite, VTX_ATTR_CLIPPOSi);
  ptrWrite += TRI_SIZE;
}

/**
 * Performs clipping on a 3D triangle.
 * This implements the Sutherland-Hodgman algorithm:
 * (See: https://en.wikipedia.org/wiki/Sutherland%E2%80%93Hodgman_algorithm)
 *
 * @param arraySize output size in bytes of the clipped triangle buffer
 * @param ptrVertA vertex 0 of the tri. to be clipped
 * @param ptrVertB vertex 1
 * @param ptrVertC vertex 2
 */
macro clipTriangle(u32 arraySize, u32 ptrVertA, u32 ptrVertB, u32 ptrVertC)
{
  arraySize = TRI_SIZE_3; // 3-verts by default

  // The loop streams vertices in and out at the same time.
  // To avoid corruption, alternate between two buffers.
  u32 ptrBuff0 = CLIP_BUFFER_RESULT;
  u32 ptrBuff1 = CLIP_BUFFER_TMP;

  // copy original verts into the first buffer (@TODO: avoid this somehow?)
  {
    u32<$s0> ptrDst, ptrSrc;
    ptrDst = CLIP_BUFFER_TMP;
    ptrSrc = ptrVertA; copyClippingVertex(ptrDst, ptrSrc);
    ptrSrc = ptrVertB; copyClippingVertex(ptrDst, ptrSrc);
    ptrSrc = ptrVertC; copyClippingVertex(ptrDst, ptrSrc);
  }

  u32 planePtr = CLIPPING_PLANES;
  s32 planeIdx = 1;

  u32 ptrWrite = ptrBuff1 + arraySize;

  // check 5 planes, we shift instead of incr. for direct clip-code checks
  // the z-far plane is ignored
  loop
  {
    u32 ptrLast = ptrWrite - TRI_SIZE;

    vec16 lastColor   = load_vec_u8(ptrLast, VTX_ATTR_RGBA);
    vec32 lastPosUV   = load(ptrLast, VTX_ATTR_CLIPPOSi).xyzw;
    lastPosUV:sint.XY = load(ptrLast, VTX_ATTR_ST).xy;
    u8 lastClipCode   = load(ptrLast, VTX_ATTR_CLIPFLAGS);

    undef ptrLast;

    swap(ptrBuff0, ptrBuff1);

    ptrWrite = ptrBuff1;
    u32 ptrRead = ptrBuff0;
    u32 ptrReadEnd = ptrBuff0 + arraySize;

    // check all clipped points so far
    loop
    {
      vec32 currentPosUV   = load(ptrRead, VTX_ATTR_CLIPPOSi).xyzw;
      currentPosUV:sint.XY = load(ptrRead, VTX_ATTR_ST).xy;
      vec16 currentColor   = load_vec_u8(ptrRead, VTX_ATTR_RGBA);

      u8 clipCode = load(ptrRead, VTX_ATTR_CLIPFLAGS);

      // check if the current and last point are contained in the clip-plane
      u8 exclIntersect = clipCode ^ lastClipCode;
      u8 isInsideCurr = ~clipCode;
      exclIntersect &= planeIdx;
      isInsideCurr &= planeIdx;

      if(exclIntersect)
      {
        intersection(
          lastPosUV, lastColor, lastClipCode, // output
          planePtr,
          lastPosUV, lastColor, currentPosUV, currentColor);
        emitClippedTri(ptrWrite, lastClipCode, lastPosUV, lastColor);
      }

      lastPosUV = currentPosUV;
      lastClipCode = clipCode;
      lastColor = currentColor;

      if(isInsideCurr) {
        emitClippedTri(ptrWrite, lastClipCode, lastPosUV, lastColor);
      }

      ptrRead += TRI_SIZE;

    } while(ptrRead != ptrReadEnd)

    planeIdx <<= 1;
    arraySize = ptrWrite - ptrBuff1;
    if(arraySize == 0)exit; // @TODO: change for multi-triangle draws

    planePtr += 0x04;
  } while(planeIdx != 0b100'000)

  CLIP_END:
}

/**
 * Converts a triangle generated from clipping back into screen-space.
 * @param ptr vertex to be converted
 */
function triToScreenSpace(u32<$s6> ptr)
{
  // Note: these values need to be loaded each time, as RDPQ_Triangle wipes most regs
  vec16 screenSize = load(SCREEN_SCALE_OFFSET).xyzwxyzw;
  vec16 screenOffset = load(SCREEN_SCALE_OFFSET, 0x08).xyzwxyzw;

  vec32 posClip = load(ptr, VTX_ATTR_CLIPPOSi).xyzw;
  posClip.w = invert_half(posClip).w;
  store(posClip.w, ptr, VTX_ATTR_INVWi);

  posClip *= posClip.w;
  posClip *= screenSize;
  posClip:sint += screenOffset;

  @Barrier("cc") store(posClip:sint.xyzw, ptr, VTX_ATTR_XY);

  // write dummy clip-codes to avoid re-clipping
  u16 fakeCC = 0x00FF;
  @Barrier("cc") store(fakeCC, ptr, VTX_ATTR_CLIPFLAGS);

  ptr -= TRI_SIZE;
}

/**
 * Draws a list of triangles that have been clipped.
 *
 * @param buffer RDP buffer start (DMEM)
 * @param buffEnd RDP buffer end (DMEM)
 * @param arraySize size in bytes of the list to draw (multiple of 0x20)
 */
macro drawTrianglesClipped(u32<$s3> buffEnd, u32<$t0> arraySize)
{
  u32<$s6> ptrVertIn = CLIP_BUFFER_RESULT;
  u32<$s5> ptrVertInEnd = ptrVertIn + arraySize;
  ptrVertInEnd -= TRI_SIZE_2;

  // we need to save the vertex-addresses to memory for RDPQ_Triangle.
  // set the first one here (is constant) the rest is set in each loop iteration
  store(ptrVertIn:u16, RSPQ_SCRATCH_MEM, 0);

  // convert the base-vertex and the one from the first iteration
  // for each triangle-fan part we then only need to convert one new vert
  triToScreenSpace(ptrVertIn);
  ptrVertIn += TRI_SIZE_2;
  triToScreenSpace(ptrVertIn);

  // draw clipped triangles as a triangle-fan
  loop
  {
    ptrVertIn += TRI_SIZE_2;
    triToScreenSpace(ptrVertIn); // Note: this moves 'ptrVertIn' back by TRI_SIZE

    vert0 = CLIP_BUFFER_RESULT;
    vert2 = ptrVertIn;
    vert1 = ptrVertIn + TRI_SIZE;

    u32<$v1> vertAddr = RSPQ_SCRATCH_MEM;
    store(vert2:u16, RSPQ_SCRATCH_MEM, 2);
    store(vert1:u16, RSPQ_SCRATCH_MEM, 4);

    #ifdef DRAW_TRIS
      u32<$sp> cullDest = CLIP_AFTER_EMIT;
      RDPQ_Tri(vert0, vert1, vert2, buffEnd);
      if(vertAddr == 0) {
        u32<$s4> buffer = CLIP_BUFFER_TMP;
        EmitTriangle(buffer, buffEnd, cullDest);
      }
      CLIP_AFTER_EMIT:
    #endif

    debugLogClip();
  } while(ptrVertIn != ptrVertInEnd)
}

/**
 * Ported over and modified from libdragons RDPQ_Send.
 * This will emit a Triangle to the RDP by moving it into the RDRAM buffer.
 *
 * @param buffStart
 * @param buffEnd
 * @param returnAddress address to jump to after the triangle has been emitted
 */
function EmitTriangle(u32<$s4> buffStart, u32<$s3> buffEnd, u32<$sp> returnAddress)
{
  u32<$t7> rspq_cmd_size = buffEnd - buffStart;
  if(rspq_cmd_size != 0) {

    s32<$s0> rdram_cur = load(RDPQ_CURRENT);
    rdram_cur &= 0xFF'FFFF;

    s32<$a2> sentinel = load(RDPQ_SENTINEL);
    sentinel -= rspq_cmd_size;

    u32<$t5> next_func = RSPQCmd_RdpAppendBuffer;
    if(sentinel < rdram_cur)
    {
      u8<$t3> fullSync = load(RDPQ_SYNCFULL_ONGOING);
      fullSync |= DP_STATUS_END_VALID;

      RSPQ_RdpWait();

      // Switch to the next dynamic buffer.
      // Since there are two of them, also switch between
      // them so next time we will pick the other one.
      rdram_cur = load(RDPQ_DYNAMIC_BUFFERS, 4);
      u32<$t1> rdram_sw = load(RDPQ_DYNAMIC_BUFFERS, 0);
      store(rdram_cur, RDPQ_DYNAMIC_BUFFERS, 0);
      store(rdram_sw, RDPQ_DYNAMIC_BUFFERS, 4);

      // Calculate new sentinel (end of buffer)
      sentinel = rdram_cur + RDPQ_DYNAMIC_BUFFER_SIZE;

      // Run the DMA transfer now, and after that, run RSPQCmd_RdpSetBuffer via
      // tailcall. Prepare a1 for it, containing the pointer to the new buffer,
      // which will be written into DP_START.
      u32<$a1> rdram_cur_temp = rdram_cur;
      next_func = RSPQCmd_RdpSetBuffer;
    }

    u32<$t0> dmaSize = rspq_cmd_size - 1;
    s32<$a0> rdram_next = rdram_cur + rspq_cmd_size;
    DMAOut(dmaSize);

    RA = returnAddress;
    goto next_func;
  }
}


/**
 * Draws a triangle to the screen, if necessary also performs clipping.
 *
 * @param vert0 16-LSB contain DMEM address for the first vertex
 * @param vert1 LSB/MSB contain DMEM address for the second + third vertex
 */
command<0> T3DCmd_TriDraw(u32 vert0, u32 vert1)
{
  //asm("emux_trace_start"); //asm("emux_trace_stop");

  // Note: vert1 & vert2 are switched, this avoids an additional instruction here.
  // We can't switch them on the CPU, as RDPQ_Triangle loads them from DMEM, so the order is different
  u32<$a2> vert2 = vert1 >> 16;
  u32<$s3> buffEnd = CLIP_BUFFER_TMP;

  {
    u32<$v1> vertAddr = get_cmd_address(2); // where to load the vertex pointers from
    u32<$sp> rejectDest = RSPQ_Loop; // jump-target for rejected/culled tris, also used by EmitTriangle

    RDPQ_Tri(vert0, vert1, vert2, buffEnd);
    if(vertAddr == 0) {
      u32<$s4> buffStart = CLIP_BUFFER_TMP;
      EmitTriangle(buffStart, buffEnd, rejectDest);
    }

    // reload vertex indices, RDPQ_Tri may have modified them
    vert0:u16 = load_arg(0x02);
    vert1:u16 = load_arg(0x04);
    vert2:u16 = load_arg(0x06);

    //u8 useReject = load(USE_REJECT); //@TODO: do this branch-less somehow
    //if(useReject)exit;
  }

  u32<$t0> arraySize;
  clipTriangle(arraySize, vert0, vert1, vert2);
  drawTrianglesClipped(buffEnd, arraySize);
}

/**
 * Sets current screen-size in pixel.
 * @param guardBandFactor s16 with the guard multiplier (should be 1-4)
 * @param screenScale screen-size * 2
 */
command<1> T3DCmd_SetScreenSize(s8 guardBandFactor, u32 screenOffsetXY, u32 screenScaleXY)
{
  // Scale & offset to transform position into screen-space and normalize depth.
  s16 screenScaleZ = 32767; // @TODO: share this with the fog constant
  s16 screenOffsetZ = 16383;

  store(screenScaleXY, SCREEN_SCALE_OFFSET, 0x00);
  store(screenScaleZ,  SCREEN_SCALE_OFFSET, 0x04);
  store(screenOffsetXY, SCREEN_SCALE_OFFSET, 0x08);
  store(screenOffsetZ,  SCREEN_SCALE_OFFSET, 0x0C);

  u8 useReject = guardBandFactor >> 16;
  store(useReject, USE_REJECT);

  // guard-band multiplier (W value of the clipping plane, integer)
  guardBandFactor &= 0xF;
  s8 guardBandFactorNeg = ZERO - guardBandFactor;

  store(guardBandFactor,    CLIPPING_PLANES, 3); // +X Plane
  store(guardBandFactor,    CLIPPING_PLANES, 7); // +Y Plane
  //store(guardBandFactor,    CLIPPING_PLANES, 11); // +Z Plane
  store(guardBandFactorNeg, CLIPPING_PLANES, 15); // -X Plane
  store(guardBandFactorNeg, CLIPPING_PLANES, 19); // -Y Plane

   // printf("\\033[2J"); // clear console
}

/**
 * Sets the range of fog, or disables it.
 *
 * @param fogScale 16.16 scale factor (set to 0 to disable fog)
 * @param fogOffset 16.16 offset
 */
command<10> T3DCmd_SetFogRange(s16 fogScale, s32 fogOffset)
{
  store(fogScale,  FOG_SCALE_OFFSET, 0x00);
  store(fogOffset, FOG_SCALE_OFFSET, 0x02);

  u8 useFog = fogScale >> 16;
  useFog |= fogScale;
  store(useFog, USE_FOG);
}

/**
 * Sets a light (ambient or directional)
 *
 * @param address ambient or dir. light address (DMEM)
 * @param rgba8 color RGBA8
 * @param dirXYZ normalized packed direction, ignored if zero
 */
command<5> T3DCmd_LightSet(u32 addr, u32 rgba8, u32 dirXYZ)
{
  store(rgba8, addr, 0);
  store(rgba8, addr, 4);

  if(dirXYZ) {
    store(dirXYZ, addr, 8);
    store(dirXYZ, addr, 12);
  }
}

/**
 * Sets the number of active directional lights.
 * @param count range: 0 - LIGHT_COUNT
 */
command<9> T3DCmd_LightCount(u8 count)
{
  store(count, ACTIVE_LIGHT_SIZE);
}

/**
 * Sets various render-mode settings.
 * @param culling for RDPQ_Triangle, 0=front, 1=back, >1=none
 * @param triCommand for RDPQ_Triangle
 */
command<6> T3DCmd_RenderMode(u8 culling, u32 triCommand) {
  store(culling, FACE_CULLING);

  // Mipmap setting (pulled out from RDPQ_Triangle)
  u8 mipmap = load(RDPQ_OTHER_MODES);
  mipmap &= 0x38;
  triCommand |= mipmap;

  store(triCommand, TRI_COMMAND);
}

/**
 * Sets current camera, only used for culling and FX.
 * @param dirXYZ normalized direction, packed s8
 * @param posXY world-space pos (XY) as s16
 * @param posZ  world-space pos (Z) as s16
 */
command<7> T3DCmd_SetCam(u32 dirXYZ, u32 posXY, u16 posZ)
{
  // @TODO: implement this if anything needs the camera
}

/**
 * Multiplies two matrices in memory.
 *
 * @param addrOut destination pointer
 * @param addrMatL pointer to left matrix
 * @param addrMatR pointer to right matrix
 */
function mulMat4Mat4(u32<$s2> addrOut, u32<$s3> addrMatL, u32<$s4> addrMatR)
{
  vec32 matL0 = load(addrMatL, 0x00).xyzwxyzw;
  vec32 matL1 = load(addrMatL, 0x10).xyzwxyzw;
  vec32 matL2 = load(addrMatL, 0x20).xyzwxyzw;
  vec32 matL3 = load(addrMatL, 0x30).xyzwxyzw;

  vec32 matR01, matR23;

  matR01.xyzw = load(addrMatR, 0x00).xyzw;
  matR01.XYZW = load(addrMatR, 0x10).xyzw;
  matR23.xyzw = load(addrMatR, 0x20).xyzw;
  matR23.XYZW = load(addrMatR, 0x30).xyzw;

  vec32 tmp;
  tmp    = matL0  * matR01.xxxxXXXX;
  tmp    = matL1 +* matR01.yyyyYYYY;
  tmp    = matL2 +* matR01.zzzzZZZZ;
  matR01 = matL3 +* matR01.wwwwWWWW;

  tmp    = matL0  * matR23.xxxxXXXX;
  tmp    = matL1 +* matR23.yyyyYYYY;
  tmp    = matL2 +* matR23.zzzzZZZZ;
  matR23 = matL3 +* matR23.wwwwWWWW;

  store(matR01.xyzw, addrOut, 0x00);
  store(matR01.XYZW, addrOut, 0x10);
  store(matR23.xyzw, addrOut, 0x20);
  store(matR23.XYZW, addrOut, 0x30);
}

/**
 * Multiplies a matrix with a 3D-vector.
 * This assumes a matrix duplicated in each register to
 * multiply 2 different vectors in one register at the same time.
 */
macro mulMat4Vec8(
  vec32 mat0, vec32 mat1, vec32 mat2, vec32 mat3,
  vec16 vec, vec32 out
) {
  out = mat0  * vec.xxxxXXXX;
  out = mat1 +* vec.yyyyYYYY;
  out = mat2 +* vec.zzzzZZZZ;
  out = mat3 +* vec.wwwwWWWW;
}

/**
 * Normalize a matrix vector (column) in memory and saves it as a fraction.
 * This is used to normalize the normal matrix.
 *
 * @param addrIn vector input address (s16.16 format)
 * @param addrOut vector output address (s0.16 format)
 */
function normalizeMatrixVector(u32<$t0> addrIn, u32<$t1> addrOut)
{
  vec32 v = load(addrIn, 0x00).xyzw;

  // get vector length
  vec32 vLenInv = v * v;
  vec32 tmp = vLenInv + vLenInv.yyyyYYYY;
  vLenInv   = vLenInv + tmp.zzzzZZZZ;

  // inverse of the length
  vLenInv.w = invert_half_sqrt(vLenInv).x;
  vLenInv >>= 9;

  v *= vLenInv.wwwwWWWW; // normalize
  v:sfract += v:sfract; // prevent over/underflow

  store(v:sfract.xyzw, addrOut, 0x00);

  addrIn += 0x10;
  addrOut += 0x8;
}

/**
 * Sets a new model matrix (optionally with a multiply).
 *
 * @param addressMat RDRAM address to load matrix from
 * @param offsetDestMul destination offset & multiply target (optional)
 */
command<2> T3DCmd_MatSet(u32 addressMat, u32 offsetDestMul)
{
  addressMat &= 0xFFFFFF;

  u32<$s4> addrDst = offsetDestMul >> 16;
  addrDst += MAT_MODEL_DATA;
  store(addrDst:u16, CURRENT_MAT_ADDR);

  // DMA matrix into the target slot (already the correct format)
  dma_in(addrDst, addressMat, 0x40);

  u32<$s2> addMulDst = addrDst;
  u32<$s3> addrMul = offsetDestMul & 0xFFFF;
  addrMul += MAT_MODEL_DATA;

  // optionally, multiply by another matrix first
  if(addrMul != addrDst) {
    mulMat4Mat4(addMulDst, addrMul, addrDst);
  }

  // now grab the normal matrix and store it in a special slot.
  u32<$t0> normIn = addrDst;
  u32<$t1> normOut = MAT_MODEL_NORM;
  normalizeMatrixVector(normIn, normOut);
  normalizeMatrixVector(normIn, normOut);
  normalizeMatrixVector(normIn, normOut);

  // ...followed by applying the projection matrix, storing it in a special slot too.
  // Together, these two special slots are used for vertex transformation later on.
  addMulDst = MAT_MODEL_RROJ;
  addrMul = MAT_PROJ_DATA;
  mulMat4Mat4(addMulDst, addrMul, addrDst);
}

/**
 * Sets a new projection matrix.
 * @param addressMat RDRAM address to load matrix from
 */
command<8> T3DCmd_MatProjectionSet(u32 addressMat) {
  addressMat &= 0xFFFFFF;
  dma_in(MAT_PROJ_DATA, addressMat, 0x40);
}

/**
 * Reads out debugging data (e.g. face-count).
 * The flag 'DEBUG_LOG_TRIS' must be set.
 */
command<3> T3DCmd_DebugRead(u32 _, u32 addressRDRAM)
{
#ifdef DEBUG_LOG_TRIS
  dma_out(DEBUG_TRI_COUNT, addressRDRAM, 16);
  store(ZERO, DEBUG_TRI_COUNT);
  store(ZERO, DEBUG_TRI_COUNT, 4);
#endif
}

// RDPQ_Tri
include "./rspq_triangle.inc"
`;