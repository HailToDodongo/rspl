/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

// The flags here are either N64 hardware register/flags or composite values from libdragon.
// They are mainly used directly to avoid macros that my expand into multiple instructions.
  // For all macros/flags see: include/rsp.inc in libdragon

export const SP_STATUS = {
  HALTED    : 1<<0,
  BROKE     : 1<<1,
  DMA_BUSY  : 1<<2,
  DMA_FULL  : 1<<3,
  IO_FULL   : 1<<4,
  SSTEP     : 1<<5,
  INTR_BREAK: 1<<6,
  SIG0      : 1<<7,
  SIG1      : 1<<8,
  SIG2      : 1<<9,
  SIG3      : 1<<10,
  SIG4      : 1<<11,
  SIG5      : 1<<12,
  SIG6      : 1<<13,
  SIG7      : 1<<14,
};

export const DMA_FLAGS = {
  DMA_IN_ASYNC : 0x00000000,
  DMA_OUT_ASYNC: 0xFFFF8000,
  DMA_IN       : 0x00000000 | SP_STATUS.DMA_BUSY | SP_STATUS.DMA_FULL,
  DMA_OUT      : 0xFFFF8000 | SP_STATUS.DMA_BUSY | SP_STATUS.DMA_FULL,
};
