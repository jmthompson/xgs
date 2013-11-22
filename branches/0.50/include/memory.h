#ifndef _GSMEMORY_H_
#define _GSMEMORY_H_

extern int	mem_ramsize;

/* Flags that make up the Shadow Register ($C035). */

extern int	mem_shadow_text;
extern int	mem_shadow_hires1;
extern int	mem_shadow_hires2;
extern int	mem_shadow_super;
extern int	mem_shadow_aux;
extern int	mem_shadow_lc;

/* The slot control bits in the slot register */

extern int mem_slot_reg[8];

/* Language card control registers */

extern int	mem_lcbank2;
extern int	mem_lcread;
extern int	mem_lcwrite;
extern int	mem_lcsecond;

/* Auxillary memory access registers */

extern int	mem_auxrd;
extern int	mem_auxwrt;
extern int	mem_altzp;
extern int	mem_80store;

extern int	mem_intcxrom;
extern int	mem_slotc3rom;
extern int	mem_rombank;

/* Diagnostics and Interrupt status register */

extern int	mem_diagtype;

/* Master pointers to our slow and fast RAM blocks	*/

extern byte	*slow_memory;
extern byte	*fast_memory;
extern byte	*rom_memory;

extern byte	*c07x_memory;

/* Memory for saving banks $00 and $01 on an NMI	*/

extern byte	*save_memory;

/* Junk memory for writing to when an area is read-protected */

extern byte	junk_memory[256];

/* Structure used to regulate all memory access	*/
/* on a page-by-page basis. This allows the	*/
/* emulator to easily rearrange memory as the	*/
/* softswitches are flipped.			*/

typedef struct {
	byte	*readPtr;
	byte	*writePtr;
	word16	readFlags;
	word16	writeFlags;
	word16	extraFlags1;
	word16	extraFlags2;
} mem_pagestruct;

/* Read/write flag definitions for above struct	*/

#define MEM_FLAG_SHADOW_E0	0x0001
#define MEM_FLAG_SHADOW_E1	0x0002
#define MEM_FLAG_IO		0x0004
#define MEM_FLAG_SPECIAL	0x0008
#define MEM_FLAG_INVALID	0x8000

/* Tables for determining which bytes in slow_memory have changed, to	*/
/* within 8 bytes of resolution.					*/

extern const word32	mem_change_masks[256];
extern word32		mem_slowram_changed[512];

extern mem_pagestruct	mem_pages[65536];

/* Nonzero if we are a ROM 03; otherwise we're a ROM 01 */

extern int	mem_rom03;

extern int	MEM_init(void);
extern void	MEM_update(void);
extern void	MEM_reset(void);
extern void	MEM_shutdown(void);

extern void	MEM_rebuildMainMem(void);
extern void	MEM_rebuildAltZpMem(void);
extern void	MEM_rebuildLangCardMem(void);
extern void	MEM_rebuildShadowMem(void);
extern void	MEM_buildLanguageCard(int, int, byte *);

extern void	MEM_writeMem2(word32,byte);

extern byte (*mem_io_read_dispatch[256])(byte);
extern byte (*mem_io_write_dispatch[256])(byte);

extern byte	MEM_ioUnimp(byte);

extern byte	MEM_clear80store(byte val);
extern byte	MEM_set80store(byte val);
extern byte	MEM_get80store(byte val);
extern byte	MEM_clearAuxRd(byte val);
extern byte	MEM_setAuxRd(byte val);
extern byte	MEM_getAuxRd(byte val);
extern byte	MEM_clearAuxWrt(byte val);
extern byte	MEM_setAuxWrt(byte val);
extern byte	MEM_getAuxWrt(byte val);
extern byte	MEM_clearIntCXROM(byte val);
extern byte	MEM_setIntCXROM(byte val);
extern byte	MEM_getIntCXROM(byte val);
extern byte	MEM_clearAltZP(byte val);
extern byte	MEM_setAltZP(byte val);
extern byte	MEM_getAltZP(byte val);
extern byte	MEM_clearSlotC3ROM(byte val);
extern byte	MEM_setSlotC3ROM(byte val);
extern byte	MEM_getSlotC3ROM(byte val);
extern byte	MEM_getLCbank(byte val);
extern byte	MEM_getLCread(byte val);
extern byte	MEM_getSlotReg(byte val);
extern byte	MEM_setSlotReg(byte val);
extern byte	MEM_getShadowReg(byte val);
extern byte	MEM_setShadowReg(byte val);
extern byte	MEM_getCYAReg(byte val);
extern byte	MEM_setCYAReg(byte val);
extern byte	MEM_getIntEnReg(byte val);
extern byte	MEM_setIntEnReg(byte val);
extern byte	MEM_getDiagType(byte val);
extern byte	MEM_setVBLClear(byte val);
extern byte	MEM_getStateReg(byte val);
extern byte	MEM_setStateReg(byte val);
extern byte	MEM_setLCx80(byte val);
extern byte	MEM_setLCx81(byte val);
extern byte	MEM_setLCx82(byte val);
extern byte	MEM_setLCx83(byte val);
extern byte	MEM_setLCx88(byte val);
extern byte	MEM_setLCx89(byte val);
extern byte	MEM_setLCx8A(byte val);
extern byte	MEM_setLCx8B(byte val);

extern byte	MEM_getC071(byte val);
extern byte	MEM_getC072(byte val);
extern byte	MEM_getC073(byte val);
extern byte	MEM_getC074(byte val);
extern byte	MEM_getC075(byte val);
extern byte	MEM_getC076(byte val);
extern byte	MEM_getC077(byte val);
extern byte	MEM_getC078(byte val);
extern byte	MEM_getC079(byte val);
extern byte	MEM_getC07A(byte val);
extern byte	MEM_getC07B(byte val);
extern byte	MEM_getC07C(byte val);
extern byte	MEM_getC07D(byte val);
extern byte	MEM_getC07E(byte val);
extern byte	MEM_getC07F(byte val);

static inline byte MEM_readMem(word32 addr)
{
	word16 page = (word16) (addr >> 8);
	if (mem_pages[page].readFlags & MEM_FLAG_IO)
		 return (*mem_io_read_dispatch[addr & 0xFF])(0);
	else
		return *(mem_pages[page].readPtr + (addr & 0xFF));
}

static inline void MEM_writeMem(word32 addr, byte val)
{
	word16 page = (word16) (addr >> 8);
	if (mem_pages[page].writeFlags)
		MEM_writeMem2(addr,val);
	else
		*(mem_pages[page].writePtr + (addr & 0xFF)) = val;
}

#endif /* _GSMEMORY_H_ */
