/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * This class implements the memory-management functionality of the
 * MegaII and the FPI/CYA. It listens for accesses to the memory
 * control softswitches and rebuilds the System's memory mapping as
 * needed.
 */

#include <cstdint>
#include <cstdlib>

#include "interrupts.h"
#include "memory.h"
#include "soft_switches.h"

using std::uint8_t;

static uint8_t last_offset;
static bool in_vbl;

#define DEFINE_TOGGLE(name)                                                            \
  static uint8_t clear_##name(const uint8_t offset, const uint8_t val)             \
  {                                                                                \
    sw_##name = false;                                                             \
    updateMemoryMaps();                                                            \
    last_offset = offset;                                                          \
    return 0;                                                                      \
  }                                                                                \
  static uint8_t get_##name(const uint8_t offset, const uint8_t _v)                \
  {                                                                                \
    last_offset = offset;                                                          \
    return sw_##name ? 0x80 : 0x00;                                                \
  }                                                                                \
  static uint8_t set_##name(const uint8_t offset, const uint8_t val)               \
  {                                                                                \
    sw_##name = true;                                                              \
    updateMemoryMaps();                                                            \
    last_offset = offset;                                                          \
    return 0;                                                                      \
  }

namespace Mega2
{

DEFINE_TOGGLE(80store)
DEFINE_TOGGLE(auxrd)
DEFINE_TOGGLE(auxwr)
DEFINE_TOGGLE(intcxrom)
DEFINE_TOGGLE(slotc3rom)
DEFINE_TOGGLE(altzp)
DEFINE_TOGGLE(lcbank2)
DEFINE_TOGGLE(lcread)

static uint8_t get_in_vbl(const uint8_t offset, const uint8_t _v)
{
  last_offset = offset;
  return in_vbl ? 0x80 : 0x00;
}

static uint8_t get_slot_reg(const uint8_t offset, const uint8_t _v)
{
  uint8_t val = 0;

  for (int i = 7; i >= 0; --i) {
    val <<= 1;
    val |= sw_slot_reg[i];
  }

  last_offset = offset;
  return val;
}

static uint8_t set_slot_reg(const uint8_t offset, const uint8_t val)
{
  for (int i = 0; i < 8; ++i) {
    sw_slot_reg[i] = val & (1 << i);
  }

  updateMemoryMaps();
  last_offset = offset;
  return val;
}

static uint8_t get_shadow_reg(const uint8_t offset, const uint8_t _v)
{
  uint8_t val = 0;

  if (!sw_shadow_text) val |= 0x01;
  if (!sw_shadow_hires1) val |= 0x02;
  if (!sw_shadow_hires2) val |= 0x04;
  if (!sw_shadow_super) val |= 0x08;
  if (!sw_shadow_aux) val |= 0x10;
  // if (!sw_shadow_text2)  val |= 0x20;
  if (!sw_shadow_lc) val |= 0x40;

  last_offset = offset;
  return val;
}

static uint8_t set_shadow_reg(const uint8_t offset, const uint8_t val)
{
  sw_shadow_text = !(val & 0x01);
  sw_shadow_hires1 = !(val & 0x02);
  sw_shadow_hires2 = !(val & 0x04);
  sw_shadow_super = !(val & 0x08);
  sw_shadow_aux = !(val & 0x10);
  sw_shadow_text2 = !(val & 0x20);
  sw_shadow_lc = !(val & 0x40);

  updateMemoryMaps();
  last_offset = offset;
  return 0;
}

static uint8_t get_speed_reg(const uint8_t offset, const uint8_t _v)
{
  uint8_t val = 0;

  if (sw_fastmode) val |= 0x80;
  if (sw_slot7_motor) val |= 0x08;
  if (sw_slot6_motor) val |= 0x04;
  if (sw_slot5_motor) val |= 0x02;
  if (sw_slot4_motor) val |= 0x01;

  last_offset = offset;
  return val;
}

static uint8_t set_speed_reg(const uint8_t offset, const uint8_t val)
{
  sw_fastmode = val & 0x80;
  sw_slot7_motor = val & 0x08;
  sw_slot6_motor = val & 0x04;
  sw_slot5_motor = val & 0x02;
  sw_slot4_motor = val & 0x01;

  last_offset = offset;
  return 0;
}

static uint8_t get_vbl_mask(const uint8_t offset, const uint8_t _v)
{
  uint8_t val = 0;

  if (sw_qtrsecirq_enable) val |= 0x10;
  if (sw_vblirq_enable) val |= 0x08;
  if (sw_m2mouseswirq) val |= 0x04;
  if (sw_m2mousemvirq) val |= 0x02;
  if (sw_m2mouseenable) val |= 0x01;

  last_offset = offset;
  return val;
}

static uint8_t set_vbl_mask(const uint8_t offset, const uint8_t val)
{
  sw_m2mouseenable = val & 0x01;
  sw_m2mousemvirq = val & 0x02;
  sw_m2mouseswirq = val & 0x04;
  sw_vblirq_enable = val & 0x08;
  sw_qtrsecirq_enable = val & 0x10;

  return 0;
}

static uint8_t get_diagtype(const uint8_t offset, const uint8_t _v)
{
  last_offset = offset;
  return sw_diagtype;
}

static uint8_t set_diagtype(const uint8_t offset, const uint8_t val)
{
  if (sw_diagtype & 0x08) {
    sw_diagtype &= ~0x08;

    if (!(sw_diagtype & 0x18)) {
      lowerInterrupt(MEGA2_IRQ);
    }
  }
  last_offset = offset;
  return sw_diagtype;
}

static uint8_t get_state_reg(const uint8_t offset, const uint8_t _v)
{
  uint8_t val = 0;

  if (sw_intcxrom) val |= 0x01;
  if (sw_rombank) val |= 0x02;
  if (sw_lcbank2) val |= 0x04;
  if (!sw_lcread) val |= 0x08;
  if (sw_auxwr) val |= 0x10;
  if (sw_auxrd) val |= 0x20;
  if (sw_page2) val |= 0x40;
  if (sw_altzp) val |= 0x80;

  last_offset = offset;
  return val;
}

static uint8_t set_state_reg(const uint8_t offset, const uint8_t val)
{
  sw_intcxrom = val & 0x01;
  sw_rombank = val & 0x02;
  sw_lcbank2 = val & 0x04;
  sw_lcread = !(val & 0x08);
  sw_auxwr = val & 0x10;
  sw_auxrd = val & 0x20;
  sw_page2 = val & 0x40;
  sw_altzp = val & 0x80;
  // FIXME: tell vgc that page2 changed
  updateMemoryMaps();
  return 0;
}

static uint8_t get_rom(const uint8_t offset, const uint8_t _v)
{
  last_offset = offset;
  return getPageReadPointer(0xFFC0)[offset];
}

static uint8_t set_c08x(const uint8_t offset, const uint8_t _v)
{
  sw_lcbank2 = !(offset & 0x08);
  sw_lcread = ((offset & 0x03) == 0x01) || ((offset & 0x03) == 0x03);
  sw_lcwrite = !(offset & 0x01) && (offset == last_offset);
  last_offset = offset;
  updateMemoryMaps();
  return 0;
}

static uint8_t clear_vbl_int(const uint8_t offset, const uint8_t _v)
{
  if (sw_diagtype & 0x10) lowerInterrupt(MEGA2_IRQ);
  if (sw_diagtype & 0x08) lowerInterrupt(MEGA2_IRQ);

  sw_diagtype &= ~0x18;

  return 0;
}

void start()
{
  setIoReadHandler(0x02, set_auxrd);
  setIoReadHandler(0x11, get_lcbank2);
  setIoReadHandler(0x12, get_lcread);
  setIoReadHandler(0x13, get_auxrd);
  setIoReadHandler(0x14, get_auxwr);
  setIoReadHandler(0x15, get_intcxrom);
  setIoReadHandler(0x16, get_altzp);
  setIoReadHandler(0x17, get_slotc3rom);
  setIoReadHandler(0x18, get_80store);
  setIoReadHandler(0x19, get_in_vbl);
  setIoReadHandler(0x2D, get_slot_reg);
  setIoReadHandler(0x35, get_shadow_reg);
  setIoReadHandler(0x36, get_speed_reg);
  setIoReadHandler(0x41, get_vbl_mask);
  setIoReadHandler(0x46, get_diagtype);
  setIoReadHandler(0x68, get_state_reg);
  setIoReadHandler(0x71, get_rom);
  setIoReadHandler(0x72, get_rom);
  setIoReadHandler(0x73, get_rom);
  setIoReadHandler(0x74, get_rom);
  setIoReadHandler(0x75, get_rom);
  setIoReadHandler(0x76, get_rom);
  setIoReadHandler(0x77, get_rom);
  setIoReadHandler(0x78, get_rom);
  setIoReadHandler(0x79, get_rom);
  setIoReadHandler(0x7A, get_rom);
  setIoReadHandler(0x7B, get_rom);
  setIoReadHandler(0x7C, get_rom);
  setIoReadHandler(0x7D, get_rom);
  setIoReadHandler(0x7E, get_rom);
  setIoReadHandler(0x7F, get_rom);
  setIoReadHandler(0x80, set_c08x);
  setIoReadHandler(0x81, set_c08x);
  setIoReadHandler(0x82, set_c08x);
  setIoReadHandler(0x83, set_c08x);
  setIoReadHandler(0x84, set_c08x);
  setIoReadHandler(0x85, set_c08x);
  setIoReadHandler(0x86, set_c08x);
  setIoReadHandler(0x87, set_c08x);
  setIoReadHandler(0x88, set_c08x);
  setIoReadHandler(0x89, set_c08x);
  setIoReadHandler(0x8A, set_c08x);
  setIoReadHandler(0x8B, set_c08x);
  setIoReadHandler(0x8C, set_c08x);
  setIoReadHandler(0x8D, set_c08x);
  setIoReadHandler(0x8E, set_c08x);
  setIoReadHandler(0x8F, set_c08x);

  setIoWriteHandler(0x00, clear_80store);
  setIoWriteHandler(0x01, set_80store);
  setIoWriteHandler(0x02, clear_auxrd);
  setIoWriteHandler(0x03, set_auxrd);
  setIoWriteHandler(0x04, clear_auxwr);
  setIoWriteHandler(0x05, set_auxwr);
  setIoWriteHandler(0x06, clear_intcxrom);
  setIoWriteHandler(0x07, set_intcxrom);
  setIoWriteHandler(0x08, clear_altzp);
  setIoWriteHandler(0x09, set_altzp);
  setIoWriteHandler(0x0A, clear_slotc3rom);
  setIoWriteHandler(0x0B, set_slotc3rom);
  setIoWriteHandler(0x2D, set_slot_reg);
  setIoWriteHandler(0x35, set_shadow_reg);
  setIoWriteHandler(0x36, set_speed_reg);
  setIoWriteHandler(0x41, set_vbl_mask);
  setIoWriteHandler(0x47, clear_vbl_int);
  setIoWriteHandler(0x68, set_state_reg);
}

void stop(void) {}

/**
 * Reset the memory switches to their powerup state.
 */
void reset()
{
  sw_80store = false;
  sw_auxrd = false;
  sw_auxwr = false;
  sw_altzp = false;

  sw_lcbank2 = false;
  sw_lcread = false;
  sw_lcwrite = false;

  sw_intcxrom = false;

  sw_qtrsecirq_enable = false;
  sw_vblirq_enable = false;

  last_offset = 0xff;

  sw_shadow_text = true;
  sw_shadow_text2 = false;
  sw_shadow_hires1 = true;
  sw_shadow_hires2 = true;
  sw_shadow_super = false;
  sw_shadow_aux = true;
  sw_shadow_lc = true;

  for (int i = 0; i < 5; i++)
    sw_slot_reg[i] = false;

  sw_slot_reg[7] = true;

  sw_fastmode = true;
  sw_slot7_motor = false;
  sw_slot6_motor = false;
  sw_slot5_motor = false;
  sw_slot4_motor = false;

  updateMemoryMaps();
}

void tick(const unsigned int frame_number)
{
  if (sw_qtrsecirq_enable && !(frame_number % 15) && !(sw_diagtype & 0x10)) {
    sw_diagtype |= 0x10;

    raiseInterrupt(MEGA2_IRQ);
  }
}

void microtick(const unsigned int line_number)
{
  if (line_number == 192) {
    in_vbl = true;

    if (sw_vblirq_enable && !(sw_diagtype & 0x08)) {
      sw_diagtype |= 0x08;

      raiseInterrupt(MEGA2_IRQ);
    }
  } else {
    in_vbl = false;
  }
}

} // namespace Mega2