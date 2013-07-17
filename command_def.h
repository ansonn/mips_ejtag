/*----------------------------------------------------------------------------*/
/*
** define EJTAG command code & USB Protocol command code
**
** Date 	: 2012-10-12
** NOTES 	: Create By wangshuke<anson.wang@gmail.com>
**----------------------------------------------------------------------------*/

#ifndef _COMMAND_DEF_H
#define _COMMAND_DEF_H


#include <string.h>
#include <libusb-1.0/libusb.h>

#define DONT_CARE			0x00
#define EJTAG_BUFF_SIZE		1024
#define DEFAULT_DATA_SIZE	4
/*----------------------------------------------------------------------------*/
typedef int				s32;
typedef unsigned int 	u32;
typedef char			s8;
typedef unsigned char	u8;
typedef short			s16;
typedef unsigned short	u16;


/*----------------------------------------------------------------------------*/
/* USB Protocol command define
*/
#define	EMU_CMD_HW_JTAG_WRITE 	0xD5
#define EMU_CMD_HW_JTAG3		0xCF
#define EMU_CMD_HW_TRST0		0xDE
#define EMU_CMD_HW_TRST1		0xDF
#define EMU_CMD_GET_CAPS		0xE8
#define EMU_CMD_GET_HW_VERSION	0xF0
#define	EMU_CMD_VERSION			0x01
#define EMU_CMD_GET_STATE		0x07
#define EMU_CMD_HW_CLOCK		0xC8
#define EMU_CMD_SET_SPEED		0x05


/*----------------------------------------------------------------------------*/
/* EJTAG Protocal command define
*/
typedef enum {
	EJG_FREE = 0x00,
	EJG_IDCODE,
	//resve
	EJG_IMPCODE	= 0x03,
	//resve
	EJG_ADDRESS	= 0x08,
	EJG_DATA,
	EJG_CONTROL,
	EJG_ALL,
	EJG_EJTAGBOOT,
	EJG_NORMALBOOT,
	EJG_FASTDATA,
	//resved
	//EJG_PCSAMPLE = 0x14,     he32xx ejtag ver 2.6,  PCSAMPLE shouled 3.xx

	EJG_BYPASS = 0xFFFFFFFF
} EJTAG_INSTRUCTION;

/* implementaion register bits */
#define EJTAG_IMP_R3K			(1 << 28)
#define EJTAG_IMP_DINT			(1 << 24)
#define EJTAG_IMP_NODMA			(1 << 14)
#define EJTAG_IMP_MIPS16		(1 << 16)
#define EJTAG_DCR_MIPS64		(1 << 0)
/*----------------------------------------------------------------------------*/
/* EJTAG Internal Reg
*/
typedef enum {
	REG_IDCODE,
	REG_IMPCODE,
	REG_DATA,
	REG_ADDRESS,
	REG_CONTROL,
	REG_BYPASS,
	REG_FASTDATA,
	REG_TCBCONTROL_A,
	REG_TCBCONTROL_B,
	REG_TCBDATA,
	REG_TCBCONTROL_C,
	REG_PCSAMPLE,
	REG_TCBCONTROL_D,
	REG_TCBCONTROL_E,
	REG_BUTT
} EJTAG_REG;
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
typedef enum {
	SUCCESS = 0,
	CMD_PARAM_ERR,
	USB_INIT_FAILED,
	NO_FOUND_JTAG,
	LOAD_PARAM_ERR,
	LOAD_MEM_PARAM_ERR,
	DEBUG_PARAM_ERR = 5,

	OPEN_FILE_FAILED,
	INIT_NAND_FAILED,
	WRITE_NAND_NO_ALIGNED,
	WRITE_NAND_ADDR_FAILED,
	NAND_ERASE_FAILED,

	CREATE_SOCK_FAILED = 20,
	SOCK_BIND_FAILED,
	SOCK_LISTEN_FAILED,
	SOCK_ACCEPT_FAILED,

	SYS_CALL_FAIL,
	OPT_NO_SUPPORT,
	
} EJTAG_EXIT_CODE;

/*----------------------------------------------------------------------------*/
typedef enum {
	EJG_RET,
	EJG_NO_RET,
} USB_RET_FLAG;

/*----------------------------------------------------------------------------*/
typedef enum {
	NON_DEBUG = 0,
	DEBUG
} MIPS_EJTAG_MODE;

/*----------------------------------------------------------------------------*/
typedef enum {
	DATA_CACHE = 0,
	CODE_CACHE
} MIPS_CACHE_MODE;

/*----------------------------------------------------------------------------*/
typedef enum {
	NON_STEP = 0,
	USE_STEP
} MIPS_STEP_MODE;

/*----------------------------------------------------------------------------*/
typedef enum {
	MODE_NULL = 0,
	EJTAG_MODE,
	ECLIPSE_MODE,
} MIPS_PRINT_MODE;

/*----------------------------------------------------------------------------*/
typedef enum {
	NO_CMD_RUN = 0,
	CMD_RUN,
	CMD_DDRINIT
} MIPS_CMD_RUN_STATE;

/*----------------------------------------------------------------------------*/
typedef struct {
	u8 tmsbuf[EJTAG_BUFF_SIZE];
	u8 tdibuf[EJTAG_BUFF_SIZE];
	u8 send_buf[EJTAG_BUFF_SIZE * 2];
	u8 read_buf[EJTAG_BUFF_SIZE * 2 + 1];
	s32 pos;
	s32 nbits;
	MIPS_EJTAG_MODE ejtag_mode;
	MIPS_CMD_RUN_STATE cmd_run_state;
	pthread_mutex_t tap_mutex;
} MIPS_EJTAG_STRU;

/*----------------------------------------------------------------------------*/
struct break_point {
	u32 address;
	u32 code;
	u32 bp_time;
	struct break_point *next;
};
typedef struct break_point BREAK_POINT;

/*----------------------------------------------------------------------------*/
typedef struct {

	u32 bp_num;
	BREAK_POINT list_bp_head;
	pthread_mutex_t list_mutex;
	MIPS_STEP_MODE step_mode;
	MIPS_PRINT_MODE run_mode;
} MIPS_DEBUG_INFO;

/*----------------------------------------------------------------------------*/
extern MIPS_EJTAG_STRU gst_mips_ejtag;
extern MIPS_DEBUG_INFO gst_mips_debug;

/*----------------------------------------------------------------------------*/
#define FOR_ALL			2
#define FOR_ECLIPSE		1
#define FOR_EJTAG		0

#define ejtag_print(print_flag, fmt, args...) \
	do {\
		if (ECLIPSE_MODE == gst_mips_debug.run_mode\
						&& FOR_ECLIPSE == print_flag)\
		{\
			fprintf(stdout, fmt, ##args);\
			fflush(stdout);\
		}\
		else if (EJTAG_MODE == gst_mips_debug.run_mode\
						&& FOR_EJTAG == print_flag)\
		{\
			fprintf(stdout, fmt, ##args);\
			fflush(stdout);\
		}\
		else if (FOR_ALL == print_flag)\
		{\
			fprintf(stdout, fmt, ##args);\
			fflush(stdout);\
		}\
} while (0)



/*----------------------------------------------------------------------------*/
#define ejtag_write(addr, data) ejtag_write_one_data(addr, data, 4)
#define ejtag_read(addr) ejtag_read_one_data(addr, 4)
#define ejtag_write_b(addr, data) ejtag_write_one_data(addr, data, 1)
#define ejtag_read_b(addr) ejtag_read_one_data(addr, 1)



/*----------------------------------------------------------------------------*/

#define LOCK_BP_LIST() \
	do {\
		pthread_mutex_lock(&gst_mips_debug.list_mutex);\
	} while (0)
#define UNLOCK_BP_LIST() \
	do {\
		pthread_mutex_unlock(&gst_mips_debug.list_mutex);\
	} while (0)
#define LOCK_MIPS_TAP() \
	do {\
		pthread_mutex_lock(&gst_mips_ejtag.tap_mutex);\
	} while (0)
#define UNLOCK_MIPS_TAP() \
	do {\
		pthread_mutex_unlock(&gst_mips_ejtag.tap_mutex);\
	} while (0)
/*----------------------------------------------------------------------------*/
#endif



