/* This is the same SKI version returned on my ROM 03, so we'll use it.	*/

#define ADB_VERSION	6

/* Struct for SKI command processing. Command begins execution after we	*/
/* read bytes_to_read bytes, and ends after bytes_to_write bytes are	*/
/* written. Note read = get from user, write = send to user.		*/

typedef struct {
	int	command;		/* command byte */
	int	bytes_to_read;		/* bytes command will read */
	int	bytes_to_write;		/* bytes command will write */
} ski_command;

#define ADB_INPUT_BUFFER	128

extern byte		adb_m2mouseenable;
extern byte		adb_m2mousemvirq;
extern byte		adb_m2mouseswirq;

extern int	adb_pdl0, adb_pdl1;
extern word32	adb_pdl0_time,adb_pdl1_time;

extern byte	ski_kbd_reg;
extern byte	ski_modifier_reg;
extern byte	ski_data_reg;
extern byte	ski_status_reg;
extern byte	ski_mode_byte;
extern byte	ski_conf[3];
extern byte	ski_error;

extern byte	ski_ram[96];

extern byte	ski_data[16];

extern byte	ski_button0;
extern byte	ski_button1;
extern int	ski_xdelta;
extern int	ski_ydelta;

extern int	ski_input_index;
extern int	ski_output_index;
extern byte	ski_input_buffer[ADB_INPUT_BUFFER];

extern int	ski_read,ski_written;

extern byte	ski_status_irq;

extern ski_command ski_curr;

extern const	ski_command ski_table[];

int ADB_init(void);
void ADB_update(void);
void ADB_reset(void);
void ADB_shutdown(void);

byte ADB_readKey(byte);
byte ADB_clearKey(byte);

byte ADB_readMouse(byte);
byte ADB_readModifiers(byte);
byte ADB_readCommand(byte);
byte ADB_readStatus(byte);

byte ADB_readM2MouseX(byte);
byte ADB_readM2MouseY(byte);

byte ADB_readCommandKey(byte);
byte ADB_readOptionKey(byte);

byte ADB_setCommand(byte);
byte ADB_setStatus(byte);

byte ADB_triggerPaddles(byte);
byte ADB_readPaddle0(byte);
byte ADB_readPaddle1(byte);
byte ADB_readPaddle2(byte);
byte ADB_readPaddle3(byte);
