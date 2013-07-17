/*----------------------------------------------------------------------------*/
/*
** EJTAG Command
**
** Date 	: 2012-10-12
** NOTES 	: Modify By wangshuke<anson.wang@gmail.com>
**----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>


#include "command.h"
#include "command_def.h"
#include "mips_ejtag.h"


/*----------------------------------------------------------------------------*/
#define MAX_BUF	1024

#define	GDB_BLOCKING_OFF	0
#define	GDB_BLOCKING_ON		1

#define	GDB_RET_CTRL_C			-2	
#define	GDB_RET_KILL_REQUEST	-1
#define	GDB_RET_OK				0	
#define	MAX_READ_RETRY			10	



/*----------------------------------------------------------------------------*/
#define ejtag_new(type, count) ((type *)malloc((unsigned)sizeof(type)*(count)))
#define ejtag_free(p) free(p)


/*----------------------------------------------------------------------------*/
//param
u32 gdb_debug_flg = 0;
u32 gdb_port = 12345;

static char HEX_DIGIT[] = "0123456789abcdef";

static pthread_t cont_p = 0;
static s32 p_stop = 0;
static s32 b_cnt = 0;

static s32 single_mode = 0;


/*----------------------------------------------------------------------------*/

static u8 hex2nib (s8 hex);
static void gdb_send_reply(int fd, char *reply);
/*----------------------------------------------------------------------------*/
static void gdb_write(int fd, const void *buf, size_t count)
{
	int res;

	res = write(fd, buf, count);
	if (res < 0)
		ejtag_print(FOR_EJTAG, "write failed: %s\n", strerror(errno));
	if (res != count)
		ejtag_print(FOR_EJTAG, "write only wrote %d of %d bytes\n", res, count);
}

/*----------------------------------------------------------------------------*/
/* Wrap read(2) so we can read a byte without having
   to do a shit load of error checking every time. */
static int gdb_read_byte(int fd)
{
	char c;
	int res;
	int cnt = MAX_READ_RETRY;

	while (cnt--)
	{
		res = read(fd, &c, 1);
		if (res < 0)
		{
			if (errno == EAGAIN)
				/* fd was set to non-blocking and no data was available */
				return -1;

			ejtag_print(FOR_EJTAG, "read failed: %s", strerror(errno));
		}

		if (res == 0)
		{
			ejtag_print(FOR_EJTAG, "incomplete read\n");
			continue;
		}

		return c;
	}
	
	ejtag_print(FOR_EJTAG, "Maximum read reties reached");

	return 0;
}

/*----------------------------------------------------------------------------*/
/* GDB needs the 32 8-bit, gpw registers (r00 - r31), the 
** bit SREG, the 16-bit SP (stack pointer) and the 32-bit PC
** (program counter). Thus need to send a reply with
** r00, r01, ..., r31, SREG, SPL, SPH, PCL, PCH
** Low bytes before High since AVR is little endian. 
*/
static void gdb_read_registers(int fd)
{
	int i;
	u32 val;               /* ensure it's 32 bit value */

	/* (32 gpwr, SREG, SP, PC) * 8 hex bytes + terminator */
	//size_t buf_sz = (39) * 8 ;
	size_t buf_sz = (39) * 8 ;
	char *buf;

	buf = ejtag_new(char, buf_sz);

	/* 32 gen purpose working registers */
	for (i = 0; i < 37; i++)
	{
		val = gdb_get_reg(i);
		buf[i * 8] = HEX_DIGIT[(val >> 4) & 0xf];
		buf[i * 8 + 1] = HEX_DIGIT[val & 0xf];

		buf[i * 8 + 2] = HEX_DIGIT[(val >> 12) & 0xf];
		buf[i * 8 + 3] = HEX_DIGIT[(val >> 8) & 0xf];

		buf[i * 8 + 4] = HEX_DIGIT[(val >> 20) & 0xf];
		buf[i * 8 + 5] = HEX_DIGIT[(val >> 16) & 0xf];

		buf[i * 8 + 6] = HEX_DIGIT[(val >> 28) & 0xf];
		buf[i * 8 + 7] = HEX_DIGIT[(val >> 24) & 0xf];
	}

	buf[i * 8] = 0;
	gdb_send_reply (fd, buf);
	ejtag_free(buf);
}

/*----------------------------------------------------------------------------*/
/* GDB is sending values to be written to the registers. Registers are the
** same and in the same order as described in gdb_read_registers() above. 
*/
static void gdb_write_registers(int fd, char *pkt)
{
	int i;
	u8 bval;
	u32 val;               /* ensure it's a 32 bit value */

	/* 32 gen purpose working registers */
	for (i = 0; i < 37; i++)
	{
		val += ((u32)hex2nib(*pkt++)) << 4;
		val += ((u32)hex2nib(*pkt++)) << 0;

		val += ((u32)hex2nib(*pkt++)) << 12;
		val += ((u32)hex2nib(*pkt++)) << 8;

		val += ((u32)hex2nib(*pkt++)) << 20;
		val += ((u32)hex2nib(*pkt++)) << 16;

		val += ((u32)hex2nib(*pkt++)) << 28;
		val += ((u32)hex2nib(*pkt++)) << 24;

		gdb_set_reg(i, val);
	}
	gdb_send_reply (fd, "OK");
}

/*----------------------------------------------------------------------------*/
/* Extract a hexidecimal number from the pkt. Keep scanning pkt until stop
** char is reached or size of int is exceeded or a NULL is reached. pkt is
** modified to point to stop char when done.

** Use this function to extract a num with an arbitrary num of hex
** digits. This should _not_ be used to extract n digits from a m len string
** of digits (n <= m). 
*/
static int gdb_extract_hex_num(char **pkt, char stop)
{
	int i = 0;
	int num = 0;
	char *p = *pkt;
	
	 /* max number of nibbles to shift through */
	int max_shifts = sizeof(int) * 2 - 1;

	while ((*p != stop) && (*p != '\0'))
	{
		if (i > max_shifts)
			ejtag_print(FOR_EJTAG, "number too large");

		num = (num << 4)  | hex2nib(*p);
		i++;
		p++;
	}

	*pkt = p;
	return num;
}

/*----------------------------------------------------------------------------*/
/* Read a single register. Packet form: 'pn' where n is a hex number with no
** zero padding. 
*/
static void gdb_read_register(int fd, char *pkt)
{
	int reg;
	char reply[16+1];
	int val = 0;

	memset (reply, '\0', sizeof (reply));

	reg = gdb_extract_hex_num (&pkt, '\0');
	if(reg == 70 || reg == 71)
		val = 0;
	else
		val = gdb_get_reg(reg);
	
	snprintf(reply, sizeof(reply) - 1, "%02x%02x%02x%02x",
					val & 0xff, 
					(val >> 8) & 0xff, 
					(val >> 16) & 0xff,
					(val >> 24) & 0xff);
	
	gdb_send_reply (fd, reply);

	return;
}

/*----------------------------------------------------------------------------*/
/* Write a single register. Packet form: 'Pn=r' where n is a hex number with
** no zero padding and r is two hex digits for each byte in register (target
** byte order). 
*/
static void gdb_write_register(int fd, char *pkt)
{
	s32 reg;
	u32 dval = 0, hval = 0;

	reg = gdb_extract_hex_num(&pkt, '=');
	/* skip over '=' character */
	pkt++;                      

	
	dval += ((u32) hex2nib(*pkt++)) << 4;
	dval += ((u32) hex2nib(*pkt++));

	dval += ((u32) hex2nib(*pkt++)) << 12;
	dval += ((u32) hex2nib(*pkt++)) << 8;

	dval += ((u32) hex2nib(*pkt++)) << 20;
	dval += ((u32) hex2nib(*pkt++)) << 16;

	dval += ((u32) hex2nib(*pkt++)) << 28;
	dval += ((u32) hex2nib(*pkt++)) << 24;
	
	gdb_set_reg(reg, dval);
	gdb_send_reply (fd, "OK");
}



/*----------------------------------------------------------------------------*/
/* Parse the pkt string for the addr and length.
** a_end is first char after addr.
** l_end is first char after len.
** Returns number of characters to advance pkt. 
*/
static int gdb_get_addr_len(char *pkt, char a_end, 
							char l_end, int *addr, 
							int *len)
{
	char *orig_pkt = pkt;

	*addr = 0;
	*len = 0;

	/* Get the addr from the packet */
	while (*pkt != a_end)
		*addr = (*addr << 4) + hex2nib (*pkt++);
	pkt++;                      /* skip over a_end */

	/* Get the length from the packet */
	while (*pkt != l_end)
		*len = (*len << 4) + hex2nib (*pkt++);
	pkt++;                      /* skip over l_end */

	/*      fprintf( stderr, "+++++++++++++ addr = 0x%08x\n", *addr ); */
	/*      fprintf( stderr, "+++++++++++++ len  = %d\n", *len ); */

	return (pkt - orig_pkt);
}

/*----------------------------------------------------------------------------*/
static void gdb_read_memory(int fd, char *pkt)
{
	s32 addr = 0;
	s32 len = 0;
	u8 *buf;
	u8 bval;
	u16 wval;
	s32 val;
	s32 i;
	s32 is_odd_addr;

	pkt += gdb_get_addr_len(pkt, ',', '\0', &addr, &len);

	buf = ejtag_new(u8, (len * 8) + 1);

	for (i = 0; i < len/4; i ++)
	{
		val = gdb_read_mem(addr + i * 4);
		buf[i * 8] = HEX_DIGIT[val >> 4 & 0xf];
		buf[i * 8 + 1] = HEX_DIGIT[val & 0xf];

		buf[i * 8 + 2] = HEX_DIGIT[val >> 12 & 0xf];
		buf[i * 8 + 3] = HEX_DIGIT[val >> 8 & 0xf];

		buf[i * 8 + 4] = HEX_DIGIT[val >> 20 & 0xf];
		buf[i * 8 + 5] = HEX_DIGIT[val >> 16 & 0xf];
		
		buf[i * 8 + 6] = HEX_DIGIT[val >> 28 & 0xf];
		buf[i * 8 + 7] = HEX_DIGIT[val >> 24 & 0xf];
	}
	buf[i * 8] = 0;
	gdb_send_reply (fd, buf);

	ejtag_free(buf);
}

/*----------------------------------------------------------------------------*/
static void gdb_write_memory(int fd, char *pkt)
{
	s32 addr = 0;
	s32 len = 0;
	u8 bval;
	u16 wval;
	s32 is_odd_addr;
	s32 i;
	s8 reply[10];

	/* Set the default reply. */
	strncpy (reply, "OK", sizeof (reply));

	pkt += gdb_get_addr_len (pkt, ',', ':', &addr, &len);
	for (i = addr; i < addr + len; i++)
	{
		bval = hex2nib (*pkt++) << 4;
		bval += hex2nib (*pkt++);
		gdb_writeb_mem(i, bval);
	}

	gdb_send_reply (fd, reply);
}


/*----------------------------------------------------------------------------*/
/* Format of breakpoint commands (both insert and remove):

   "z<t>,<addr>,<length>"  -  remove break/watch point
   "Z<t>,<add>r,<length>"  -  insert break/watch point

   In both cases t can be the following:
   t = '0'  -  software breakpoint
   t = '1'  -  hardware breakpoint
   t = '2'  -  write watch point
   t = '3'  -  read watch point
   t = '4'  -  access watch point

   addr is address.
   length is in bytes

   For a software breakpoint, length specifies the size of the instruction to
   be patched. For hardware breakpoints and watchpoints, length specifies the
   memory region to be monitored. To avoid potential problems, the operations
   should be implemented in an idempotent way. -- GDB 5.0 manual. 
*/
static void gdb_break_point (s32 fd, s8 *pkt)
{
	s32 addr = 0;
	s32 len = 0;

	s8 z = *(pkt - 1);        /* get char parser already looked at */
	s8 t = *pkt++;
	pkt++;                      /* skip over first ',' */

	gdb_get_addr_len (pkt, ',', '\0', &addr, &len);
	switch (t)
	{
		case '0':
			if (z == 'z')
			{	
				gdb_del_break_point(addr);
				b_cnt--;
			}
			else
			{
				gdb_add_break_point(addr);
				b_cnt++;
			}
			break;

		case '1':              /* hardware breakpoint */
		case '2':              /* write watchpoint */
		case '3':              /* read watchpoint */
		case '4':              /* access watchpoint */
			gdb_send_reply (fd, "");
			return;             /* unsupported yet */
	}
	
	gdb_send_reply (fd, "OK");
}

/*----------------------------------------------------------------------------*/
/* Handle an io registers query. Query has two forms:
   "avr.io_reg" and "avr.io_reg:addr,len".

   The "avr.io_reg" has already been stripped off at this point. 

   The first form means, "return the number of io registers for this target
   device." Second form means, "send data len io registers starting with
   register addr." 
*/
static void gdb_fetch_io_registers(s32 fd, s8 *pkt)
{
	s32 addr, len;
	s32 i;
	u8 val;
	s8 reply[400];
	s8 reg_name[80];
	int pos = 0;


	if (pkt[0] == '\0')
	{
		/* gdb is asking how many io registers the device has. */
		gdb_send_reply(fd, "40");
	}
	else if (pkt[0] == ':')
	{
		/* gdb is asking for io registers addr to (addr + len) */
		gdb_get_addr_len (pkt + 1, ',', '\0', &addr, &len);
		memset (reply, '\0', sizeof (reply));
		gdb_send_reply (fd, reply); /* do nothing for now */
	}
	else
		gdb_send_reply (fd, "E01"); /* An error occurred */

}

/*----------------------------------------------------------------------------*/
/* Dispatch various query request to specific handler functions. If a query is
   not handled, send an empry reply. 
*/
static void gdb_query_request(int fd, char *pkt)
{
	int len;

	switch (*pkt++)
	{
		case 'R':
			len = strlen ("avr.io_reg");
			if (strncmp(pkt, "avr.io_reg", len) == 0)
			{
				gdb_fetch_io_registers (fd, pkt + len);
				return;
			}
		case 'T':
			if (!strncmp(pkt,"Status", 6))
			{
				gdb_send_reply (fd, "T0");
				return;
			}
			break;
		default:
			;

	}

	gdb_send_reply (fd, "");
}


/*----------------------------------------------------------------------------*/
/* Continue command format: "c<addr>" or "s<addr>"
** If addr is given, resume at that address, otherwise, resume at current
** address. 
*/
void *monitor_breakpoint(void *argv)
{
	s32 fd = *(s32 *)argv;
	int signo = SIGTRAP;
	int pc =0 ;
    BREAK_POINT *pbp = NULL;

	while (1)
	{
		if (p_stop)
			return;

		if (gdb_get_ejtag_ctrl() & (0x01 << 3)) 
		{ 
			gdb_send_reply(fd, "T05");
			return;
		}
		usleep(1000);
	}
}

/*----------------------------------------------------------------------------*/
static void gdb_continue (int fd, char *pkt)
{
	int res;
	char step = *(pkt - 1);     /* called from 'c' or 's'? */


	if (*pkt != '\0')
	{
		/* NOTE: from what I've read on the gdb lists, gdb never uses the
		   "continue at address" functionality. That may change, so let's
		   catch that case. */

		/* get addr to resume at */
		ejtag_print(FOR_EJTAG, "attempt to resume at other than current");
	}

	if ((step == 'c') || (step == 'C'))
	{
	#if 0  
		//gdb can support this step debug?
		if (single_mode) {
			set_singlestep_gdbserver(0);
			single_mode = 0;
		}
	#endif
		gdb_release();
	}

	if (!cont_p && b_cnt)
	{
		pthread_create(&cont_p, NULL, monitor_breakpoint, &fd);
		p_stop = 0;
	}
}


/*----------------------------------------------------------------------------*/
/* Continue with signal command format: "C<sig>;<addr>" or "S<sig>;<addr>"
   "<sig>" should always be 2 hex digits, possibly zero padded.
   ";<addr>" part is optional.

   If addr is given, resume at that address, otherwise, resume at current
   address. 
*/
static void gdb_continue_with_signal(s32 fd, s8 *pkt)
{
	s32 signo;
	s8 step = *(pkt - 1);

	/* strip out the signal part of the packet */
	signo = (hex2nib (*pkt++) << 4);
	signo += (hex2nib (*pkt++) & 0xf);

	if (gdb_debug_flg)
		ejtag_print(FOR_EJTAG, "GDB sent signal: %d\n", signo);

	/* Process signals send via remote protocol from gdb. Signals really don't
	   make sense to the program running in the simulator, so we are using
	   them sort of as an 'out of band' data. */

	switch (signo)
	{
		case SIGHUP:
			/* Gdb user issuing the 'signal SIGHUP' command tells sim to reset
			   itself. We reply with a SIGTRAP the same as we do when gdb
			   makes first connection with simulator. */
			//comm->reset (comm->user_data);
			//reset();
			gdb_send_reply (fd, "S05");
			return;
		default:
			/* Gdb user issuing the 'signal <signum>' command where signum is
			   >= 80 is interpreted as a request to trigger an interrupt  
			   vector. The vector to trigger is signo-80. 
			   (there's an offset of 14)  */
			if (signo >= 94)
			{
				ejtag_print(FOR_EJTAG, "signo is %x\n", signo);
			}
	}

	/* Modify pkt to look like what gdb_continue() expects and send it to
	   gdb_continue(): *pkt should now be either '\0' or ';' */

	if (*pkt == '\0')
	{
		*(pkt - 1) = step;
	}
	else if (*pkt == ';')
	{
		*pkt = step;
	}
	else
	{
		ejtag_print(FOR_EJTAG, "Malformed packet: \"%s\"\n", pkt);
		gdb_send_reply (fd, "");
		return;
	}

	gdb_continue (fd, pkt);
}


/*----------------------------------------------------------------------------*/
/* Convert a hexidecimal digit to a 4 bit nibble. */
static u8 hex2nib (s8 hex)
{
	if ((hex >= 'A') && (hex <= 'F'))
		return (10 + (hex - 'A'));

	else if ((hex >= 'a') && (hex <= 'f'))
		return (10 + (hex - 'a'));

	else if ((hex >= '0') && (hex <= '9'))
		return (hex - '0');

	/* Shouldn't get here unless the developer screwed up ;) */
	ejtag_print(FOR_EJTAG, "Invalid hexidecimal digit: 0x%02x", hex);

	return 0;
}

/*----------------------------------------------------------------------------*/
static char *gdb_last_reply(char *reply)
{
	static char *last_reply = NULL;

	if (reply == NULL)
	{
		if (last_reply == NULL)
			return "";
		else
			return last_reply;
	}

	free(last_reply);
	last_reply = strdup(reply);

	return last_reply;
}
/*----------------------------------------------------------------------------*/
static void gdb_send_ack(int fd)
{
	if (gdb_debug_flg)
		ejtag_print(FOR_EJTAG, " Ack -> gdb\n");
	gdb_write(fd, "+", 1);
}

/*----------------------------------------------------------------------------*/
/* Parse the packet. Assumes that packet is null terminated.
** Return GDB_RET_KILL_REQUEST if packet is 'kill' command,
** GDB_RET_OK otherwise. 
*/
static int gdb_parse_packet (int fd, char *pkt)
{
	if (cont_p) {
		p_stop = 1;
		pthread_join(cont_p, NULL);
		cont_p = 0;
	}

	switch (*pkt++)
	{
		case '?':              /* last signal */
			gdb_send_reply (fd, "S05"); /* signal # 5 is SIGTRAP */
			break;

		case 'g':              /* read registers */
			gdb_read_registers(fd);
			break;

		case 'G':              /* write registers */
			gdb_write_registers(fd, pkt);
			break;

		case 'p':              /* read a single register */
			gdb_read_register(fd, pkt);
			break;

		case 'P':              /* write single register */
			gdb_write_register(fd, pkt);
			break;

		case 'm':              /* read memory */
			gdb_read_memory(fd, pkt);
			break;

		case 'M':              /* write memory */
			gdb_write_memory(fd, pkt);
			break;

		case 'k':              /* kill request */
		case 'D':              /* Detach request */
			/* Reset the simulator since there may be another connection
			   before the simulator stops running. */
			ejtag_print(FOR_EJTAG, "unspport kill now !!!!\n");
			//comm->reset (comm->user_data);
			//reset
			gdb_send_reply (fd, "OK");
			return GDB_RET_KILL_REQUEST;

		case 'C':              /* continue with signal */
		case 'S':              /* step with signal */
			gdb_continue_with_signal (fd, pkt);
			break;

		case 'c':              /* continue */
		case 's':              /* step */
			gdb_continue(fd, pkt);
			break;

		case 'z':              /* remove break/watch point */
		case 'Z':              /* insert break/watch point */
			gdb_break_point(fd, pkt);
			break;

		case 'q':              /* query requests */
			gdb_query_request(fd, pkt);
			break;

		default:
			gdb_send_reply(fd, "");
	
	}
	return GDB_RET_OK;
}

/*----------------------------------------------------------------------------*/
static void gdb_send_reply(int fd, char *reply)
{
	int cksum = 0;
	int bytes;

	static char buf[MAX_BUF];

	/* Save the reply to last reply so we can resend if need be. */
	gdb_last_reply(reply);
	if (gdb_debug_flg)
		ejtag_print(FOR_EJTAG, "Sent: $%s#", reply);

	if (*reply == '\0')
	{
		gdb_write(fd, "$#00", 4);
		if (gdb_debug_flg)
			ejtag_print(FOR_EJTAG, "%02x\n", cksum & 0xff);
	}
	else
	{
		memset(buf, '\0', sizeof (buf));

		buf[0] = '$';
		bytes = 1;

		while (*reply)
		{
			cksum += (unsigned char)*reply;
			buf[bytes] = *reply;
			bytes++;
			reply++;

			/* must account for "#cc" to be added */
			if (bytes == (MAX_BUF - 3))
			{
				ejtag_print(FOR_EJTAG, "buffer overflow");
			}
		}

		if (gdb_debug_flg)
			ejtag_print(FOR_EJTAG, "%02x\n", cksum & 0xff);

		buf[bytes++] = '#';
		buf[bytes++] = HEX_DIGIT[(cksum >> 4) & 0xf];
		buf[bytes++] = HEX_DIGIT[cksum & 0xf];

		gdb_write(fd, buf, bytes);
	}
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static void gdb_set_blocking_mode (int fd, int mode)
{
	if (mode)
	{
		if (fcntl(fd, F_SETFL, fcntl (fd, F_GETFL, 0) & ~O_NONBLOCK) < 0)
			ejtag_print(FOR_EJTAG, "fcntl failed: %s\n", strerror(errno));
	}
	else
	{
		/* turn non-blocking mode on */
		if (fcntl(fd, F_SETFL, fcntl (fd, F_GETFL, 0) | O_NONBLOCK) < 0)
			ejtag_print(FOR_EJTAG, "fcntl failed: %s\n", strerror(errno));
	}
}

/*----------------------------------------------------------------------------*/
static int gdb_pre_parse_packet(int fd, int blocking)
{
	int i, res;
	int c;
	char pkt_buf[MAX_BUF + 1];
	int cksum, pkt_cksum;
	static int block_on = GDB_BLOCKING_ON;

	if (block_on != blocking)
	{
		gdb_set_blocking_mode(fd, blocking);
		block_on = blocking;
	}

	c = gdb_read_byte(fd);

	switch (c)
	{
		case '$':              /* read a packet */
			/* insure that packet is null terminated. */
			memset(pkt_buf, 0, sizeof (pkt_buf));

			/* make sure we block on fd */
			gdb_set_blocking_mode(fd, GDB_BLOCKING_ON);

			pkt_cksum = i = 0;
			c = gdb_read_byte (fd);
			while ((c != '#') && (i < MAX_BUF))
			{
				pkt_buf[i++] = c;
				pkt_cksum += (unsigned char)c;
				c = gdb_read_byte(fd);
			}

			cksum = hex2nib(gdb_read_byte (fd)) << 4;
			cksum |= hex2nib(gdb_read_byte (fd));

			if ((pkt_cksum & 0xff) != cksum)
			{
				ejtag_print(FOR_EJTAG, "Bad checksum: sent 0x%x <--> computed 0x%x",
													cksum, pkt_cksum);

			}
			if (gdb_debug_flg)
				ejtag_print(FOR_EJTAG, "Recv: \"$%s#%02x\"\n", pkt_buf, cksum);

			/* always acknowledge a well formed packet immediately */
			gdb_send_ack (fd);

			res = gdb_parse_packet(fd, pkt_buf);
			if (res < 0)
				return res;

			break;

		case '-':
			if (gdb_debug_flg)
				ejtag_print(FOR_EJTAG, " gdb -> Nak\n");
			gdb_send_reply (fd, gdb_last_reply (NULL));
			break;

		case '+':
			if (gdb_debug_flg)
				ejtag_print(FOR_EJTAG, " gdb -> Ack\n");
			break;

		case 0x03:
			/* Gdb sends this when the user hits C-c. This is gdb's way of
			   telling the simulator to interrupt what it is doing and return
			   control back to gdb. */
			return GDB_RET_CTRL_C;

		case -1:
			/* fd is non-blocking and no data to read */
			break;

		default:
			ejtag_print(FOR_EJTAG, "Unknown request from gdb: %c (0x%02x)\n", c, c);
	}

	return GDB_RET_OK;
}


/*----------------------------------------------------------------------------*/
static void gdb_main_loop(int fd)
{
	int res;
	char reply[MAX_BUF];

	while (1)
	{
		res = gdb_pre_parse_packet(fd, GDB_BLOCKING_ON);
		switch (res)
		{
			case GDB_RET_KILL_REQUEST:
				return;

			case GDB_RET_CTRL_C:
				gdb_send_ack(fd);
				snprintf(reply, MAX_BUF, "S%02x", SIGINT);
				gdb_send_reply(fd, reply);
				break;

			default:
				break;
		}
	}
}


/*----------------------------------------------------------------------------*/
void *start_gdb_server(void *argv)
{
	s32 i = 1;
	s32 ret = 0;
	s32 sock, conn;
	struct sockaddr_in address[1];
	socklen_t addrLength[1];

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		ejtag_print(FOR_ALL, "gdb_server create socket failed\n");
		return (void *)CREATE_SOCK_FAILED;
	}
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &i, sizeof (i));

	address->sin_family = AF_INET;
	address->sin_port = htons(gdb_port);
	memset(&address->sin_addr, 0, sizeof (address->sin_addr));
	ret= bind(sock, (struct sockaddr *)address, sizeof (address));
	if (0 != ret)
	{
		ejtag_print(FOR_ALL, "gdb_server bind socket failed\n");
		return (void *)SOCK_BIND_FAILED; 
	}

	while (1)
	{
		if (listen(sock, 1))
		{
			ejtag_print(FOR_EJTAG, "gdb_server listen on socket failed\n");
			return (void *)SOCK_LISTEN_FAILED;					
		}
		ejtag_print(FOR_ALL, "gdb_server waiting on port[%d]\n", gdb_port);
		addrLength[0] = sizeof (struct sockaddr);
		conn = accept(sock, (struct sockaddr *)address, addrLength);
		if (conn < 0)
		{
			ejtag_print(FOR_ALL, "gdb_server accept connection failed\n");
			return (void *)SOCK_ACCEPT_FAILED; 					
		}

		setsockopt(conn, IPPROTO_TCP, TCP_NODELAY, &i, sizeof (i));
		ejtag_print(FOR_EJTAG, "Connection opened by host %s, port %hd.\n",
										inet_ntoa(address->sin_addr), 
										ntohs(address->sin_port));

		gdb_main_loop(conn);

		close(conn);
	}

	close(sock);
}


/*----------------------------------------------------------------------------*/
void gdb_server_init(void)
{
	s32 ret = 0;
	pthread_t gsvr_pid;
	
	ret = pthread_create(&gsvr_pid, NULL, start_gdb_server, NULL);
	if (0 != ret)
	{
		ejtag_print(FOR_EJTAG, "\ncreate gdb server pthread failed!\n");
	}

	return;
}

