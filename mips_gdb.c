/*----------------------------------------------------------------------------*/
/*
** EJTAG Command
**
** Date 	: 2012-10-20
** NOTES 	: Create By wangshuke<anson.wang@gmail.com>
**----------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "command.h"
#include "command_def.h"
#include "mips_ejtag.h"

extern void ejtag_del_all_bp(void);
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
u32 gdb_get_reg(u32 reg_idx)
{
	u32 val = 0;
	
	LOCK_MIPS_TAP();
	EJTAG_INIT_WAIT_PENDING_LOAD();
	if (jtag_break_valid)
	{
		val =  ejtag_get_reg(reg_idx);
	}
	UNLOCK_MIPS_TAP();

	return val;
}
/*----------------------------------------------------------------------------*/
u32 gdb_read_mem(u32 address)
{
	u32 val = 0;
	
	LOCK_MIPS_TAP();
	EJTAG_INIT_WAIT_PENDING_LOAD();
	if (jtag_break_valid)
	{
		val =  ejtag_read_one_data(address, 4);
	}
	UNLOCK_MIPS_TAP();

	return val;
}

/*----------------------------------------------------------------------------*/
u32 gdb_writeb_mem(u32 address, u32 data)
{
	u32 val = 0;
	
	LOCK_MIPS_TAP();
	EJTAG_INIT_WAIT_PENDING_LOAD();
	if (jtag_break_valid)
	{
		val =  ejtag_write_one_data(address, data, 1);
	}
	UNLOCK_MIPS_TAP();

	return val;
}

/*----------------------------------------------------------------------------*/
void gdb_set_reg(u32 reg_idx, u32 reg_val)
{
	LOCK_MIPS_TAP();
	EJTAG_INIT_WAIT_PENDING_LOAD();
	if (jtag_break_valid)
	{
		ejtag_set_reg(reg_idx, reg_val);
	}
	UNLOCK_MIPS_TAP();

}

/*----------------------------------------------------------------------------*/
void gdb_release(void)
{
	LOCK_MIPS_TAP();
	ejtag_release();
	UNLOCK_MIPS_TAP();
}
/*----------------------------------------------------------------------------*/
void gdb_add_break_point(u32 addr)
{
	LOCK_MIPS_TAP();
	EJTAG_INIT_WAIT_PENDING_LOAD();
	if (jtag_break_valid)
	{
		ejtag_add_bp(addr);
	}
	UNLOCK_MIPS_TAP();

}

/*----------------------------------------------------------------------------*/
void gdb_del_break_point(u32 addr)
{
	LOCK_MIPS_TAP();
	EJTAG_INIT_WAIT_PENDING_LOAD();
	if (jtag_break_valid)
	{
		ejtag_del_bp(addr);
	}
	UNLOCK_MIPS_TAP();
}

BREAK_POINT *gdb_find_break_point(int addr)
{
	return (BREAK_POINT *)list_find_break_point(addr);
}

/*----------------------------------------------------------------------------*/
u32 gdb_get_ejtag_ctrl(void)
{
    u32 ctrl = 0;
	
	LOCK_MIPS_TAP();
    /* ensure that w0 are 1 and w1's are 0 */
	tap_do_instruction(EJG_CONTROL);
	ctrl = tap_wr_data(0x8000c000);
	//fprintf(stderr, "gdb_get_ejtag_ctrl  ctrl[%#x]\n", ctrl);
	UNLOCK_MIPS_TAP();

	return ctrl;
}

void gdb_del_all_bp(void)
{
	LOCK_MIPS_TAP();
	ejtag_del_all_bp();
	UNLOCK_MIPS_TAP();
}

/*----------------------------------------------------------------------------*/
