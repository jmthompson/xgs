#ifndef _SMARTPORT_H_
#define _SMARTPORT_H_

extern byte	smpt_rom[256];

int		SMPT_init(void);
void		SMPT_update(void);
void		SMPT_reset(void);
void		SMPT_shutdown(void);
int		SMPT_loadDrive(int, char *);
int		SMPT_unloadDrive(int);
void		SMPT_prodosEntry(void);
void		SMPT_smartportEntry(void);

void		SMPT_statusCmd(int);
void		SMPT_readBlockCmd(int);
void		SMPT_writeBlockCmd(int);
void		SMPT_formatCmd(int);
void		SMPT_controlCmd(int);
void		SMPT_initCmd(int);
void		SMPT_openCmd(int);
void		SMPT_closeCmd(int);
void		SMPT_readCmd(int);
void		SMPT_writeCmd(int);
void		SMPT_statusCmdExt(int);
void		SMPT_readBlockCmdExt(int);
void		SMPT_writeBlockCmdExt(int);
void		SMPT_formatCmdExt(int);
void		SMPT_controlCmdExt(int);
void		SMPT_initCmdExt(int);
void		SMPT_openCmdExt(int);
void		SMPT_closeCmdExt(int);
void		SMPT_readCmdExt(int);
void		SMPT_writeCmdExt(int);

#endif
