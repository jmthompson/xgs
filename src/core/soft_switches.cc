#include <cstdint>

/**
 * Softswitch state variables
 *
 * These variables hold the state of all of the soft switches. They are
 * here in one spot because some of these are used across subsystems.
 */

bool sw_fastmode;

bool sw_80store;
bool sw_auxrd;
bool sw_auxwr;
bool sw_altzp;

bool sw_lcbank2;
bool sw_lcread;
bool sw_lcwrite;
bool sw_lcsecond;

bool sw_shadow_text;
bool sw_shadow_text2;
bool sw_shadow_hires1;
bool sw_shadow_hires2;
bool sw_shadow_super;
bool sw_shadow_aux;
bool sw_shadow_lc;

bool sw_slot_reg[8];

bool sw_intcxrom;
bool sw_slotc3rom;
bool sw_rombank;

uint8_t sw_diagtype;

bool sw_qtrsecirq_enable;
bool sw_vblirq_enable;

bool sw_slot7_motor;
bool sw_slot6_motor;
bool sw_slot5_motor;
bool sw_slot4_motor;

bool sw_super;
bool sw_linear;
bool sw_a2mono;

unsigned int sw_bordercolor;
unsigned int sw_textfgcolor;
unsigned int sw_textbgcolor;

bool sw_80col;
bool sw_altcharset;

bool sw_text;
bool sw_mixed;
bool sw_page2;
bool sw_hires;
bool sw_dblres;

unsigned int sw_vgcint;

uint16_t sw_vert_cnt;
uint16_t sw_horiz_cnt;

bool sw_onesecirq_enable;
bool sw_scanirq_enable;

bool sw_m2mouseenable;
bool sw_m2mousemvirq;
bool sw_m2mouseswirq;
