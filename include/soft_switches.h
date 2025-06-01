#pragma once

#include <cstdint>

extern bool sw_fastmode;

extern bool sw_80store;
extern bool sw_auxrd;
extern bool sw_auxwr;
extern bool sw_altzp;

extern bool sw_lcbank2;
extern bool sw_lcread;
extern bool sw_lcwrite;
extern bool sw_lcsecond;

extern bool sw_shadow_text;
extern bool sw_shadow_text2;
extern bool sw_shadow_hires1;
extern bool sw_shadow_hires2;
extern bool sw_shadow_super;
extern bool sw_shadow_aux;
extern bool sw_shadow_lc;

extern bool sw_slot_reg[8];

extern bool sw_intcxrom;
extern bool sw_slotc3rom;
extern bool sw_rombank;

extern uint8_t sw_diagtype;

extern bool sw_qtrsecirq_enable;
extern bool sw_vblirq_enable;

extern bool sw_slot7_motor;
extern bool sw_slot6_motor;
extern bool sw_slot5_motor;
extern bool sw_slot4_motor;

extern bool sw_super;
extern bool sw_linear;
extern bool sw_a2mono;

extern unsigned int sw_bordercolor;
extern unsigned int sw_textfgcolor;
extern unsigned int sw_textbgcolor;

extern bool sw_80col;
extern bool sw_altcharset;

extern bool sw_text;
extern bool sw_mixed;
extern bool sw_page2;
extern bool sw_hires;
extern bool sw_dblres;

extern unsigned int sw_vgcint;

extern uint16_t sw_vert_cnt;
extern uint16_t sw_horiz_cnt;

extern bool sw_onesecirq_enable;
extern bool sw_scanirq_enable;

extern bool sw_m2mouseenable;
extern bool sw_m2mousemvirq;
extern bool sw_m2mouseswirq;
