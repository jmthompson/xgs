/* This is the difference (in seconds) between the IIGS's time	*/
/* (secs since 1/1/04 00:00:00) and Unix time (secs since	*/
/* 1/1/70 00:00:00).						*/

#define	CLK_OFFSET	2082826800

/* The clock chip data and control registers */

/*
extern byte	clk_data_reg;
extern byte	clk_ctl_reg;
*/

extern byte	bram[256];

/* Clock state. 0 = waiting for command, 1 = waiting for second	*/
/* byte of two-part command, 2 = waiting for data to read	*/
/* Bit 7 is set for read and clear for write. Bit 6 is set if	*/
/* BRAM is being accessed, or clear if clock register.		*/

/*
extern int	clk_state;
*/

/* Clock register/BRAM Location  being accessed. */

/*
extern int	clk_addr;
*/

int clockInit(void);
void clockShutdown(void);
void clockReset(void);

byte clockGetData(byte);
byte clockSetData(byte);

byte clockGetControl(byte);
byte clockSetControl(byte);
