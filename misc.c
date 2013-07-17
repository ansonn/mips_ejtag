/*----------------------------------------------------------------------------*/
/*
** MIPS EJTAG common function
**
** Date 	: 2012-10-12
** NOTES 	: Create By wangshuke<anson.wang@gmail.com>
**----------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>

#include "command.h"
#include "command_def.h"
#include "mips_ejtag.h"


extern s8 *reg_names[];
extern u32 gdb_get_ejtag_ctrl(void);

void ejtag_mem_write_multi_w(u32 addr, s32 *data, u32 len);
u32 ejtag_del_all_bp(void);

/*----------------------------------------------------------------------------*/
s32 load_file(s8 *filename, s32 addr, u32 *w_size)
{
	s32 i = 0;
	s32 fd = 0;
	u32 len = 0;
	u32 w_len = 0;
	u32 size = 0;
	u8 buffer[1024];
	u32 percent = 0;
	struct stat fd_state;

	fd = open(filename, O_RDONLY);
	if (0 > fd)
	{
		ejtag_print(FOR_ALL, "Opend file[%s] Failed!\n", filename);
		return OPEN_FILE_FAILED;
	}
	fstat(fd, &fd_state);
	size = fd_state.st_size;
	ejtag_print(FOR_ALL, "Load File[%s] to Mem Size[%d]\n", filename, size);

	for (i = 0; i < size; i += 1024)
	{
		len = read(fd, buffer, 1024);
		if (0 > len)
			return SYS_CALL_FAIL;
		
		ejtag_mem_write_multi_w(addr + i, (s32 *)buffer, len);

		w_len += len;
		if (0 == percent)
			ejtag_print(FOR_ECLIPSE, "load-mem %%%d\n", ++percent);
		else if ((w_len*100)/size > percent && (w_len*100)/size - percent > 8)
		{
			percent = (w_len*100)/size;
			ejtag_print(FOR_ECLIPSE, "load-mem %%%d\n", percent);
		}
		
		ejtag_print(FOR_EJTAG, ".");
	}

	close(fd);
	*w_size = size;
	ejtag_print(FOR_EJTAG, "\n");
	
	return SUCCESS;
}


/*----------------------------------------------------------------------------*/
void ejtag_run_assembly(u32 code)
{
	ejtag_init_buffer();
	ejtag_add_assembly_buffer(code);
	ejtag_send_buffer(EJG_NO_RET);
}
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
u32 ejtag_forward_word(void)
{
	u32 word = 0;

    tap_do_instruction(EJG_DATA);
    word = tap_wr_data(DONT_CARE);
    tap_do_instruction(EJG_CONTROL);    
	tap_wr_data(PROBEN | PROBTRAP);

    return word;
}

/*----------------------------------------------------------------------------*/
void ejtag_write_general_reg(u32 reg_idx, u32 reg_val)
{
    ejtag_run_assembly(LUI(reg_idx, (reg_val >> 16) & 0xffff));
    ejtag_run_assembly(ORI(reg_idx, reg_idx, reg_val & 0xFFFF));
	
	return;
}

/*----------------------------------------------------------------------------*/
u32 ejtag_read_general_reg(u32 reg_num)
{
	u32 val = 0;
	u32 bak_reg = 0;

    if (!(reg_num >= ZERO && reg_num <= RA))
		return;
		
	/* choose either v0 or a0 as transit register */
	bak_reg = (reg_num != V0 ? V0 : AT);

	/* save current v0/a0 value */
	ejtag_run_assembly(MTC0(bak_reg, CP0_DESAVE, 0));
	ejtag_run_assembly(LUI(bak_reg, (EJTAG_BASE >> 16) & 0xFFFF));
	ejtag_run_assembly(SW(reg_num, 0, bak_reg));
	ejtag_run_assembly(NOP);
	val = ejtag_forward_word();
	
	/* restore current v0/AT value */
	ejtag_run_assembly(MFC0(bak_reg, CP0_DESAVE, 0));
	
	return val;
}

/*----------------------------------------------------------------------------*/
void ejtag_write_cp0_reg(int cp0_regnum, int sel, int val)
{
    if (cp0_regnum < CP0_INDEX 
		|| (cp0_regnum >= CP0_Reserved25 && cp0_regnum <= CP0_Reserved30)
		|| cp0_regnum > CP0_DESAVE )
        return;
	
    /* save current v0 value */
    ejtag_run_assembly(MTC0(V0, CP0_DESAVE, 0));
    /* load v0 with value we want to set cp0 register to */ 
    ejtag_run_assembly(LUI(V0, (val >> 16) & 0xFFFF));
    ejtag_run_assembly(ORI(V0, V0, val & 0xFFFF));

    ejtag_run_assembly(MTC0(V0, cp0_regnum, sel));
	
    /* restore original v0 value */
    ejtag_run_assembly(MFC0(V0, CP0_DESAVE, 0));

	return;
}

/*----------------------------------------------------------------------------*/
u32 ejtag_read_cp0_reg(int cp0_regnum, int cp0_sel)
{
	u32 reg_val = 0;

    if ( cp0_regnum < CP0_INDEX 
		|| (cp0_regnum >= CP0_Reserved25 && cp0_regnum <= CP0_Reserved30)
		|| cp0_regnum > CP0_DESAVE)
        return 0;

    ejtag_run_assembly(MTC0(V0, CP0_DESAVE, 0));
    ejtag_run_assembly(LUI(V0, (EJTAG_BASE >> 16) & 0xFFFF));
    ejtag_run_assembly(MFC0(AT, cp0_regnum, cp0_sel));
    ejtag_run_assembly(SW(AT, 0, V0));
    ejtag_run_assembly(NOP);
    reg_val = ejtag_forward_word();

    /* restore original v0 value */
    ejtag_run_assembly(MFC0(V0, CP0_DESAVE, 0));

	return reg_val;
}

/*----------------------------------------------------------------------------*/
void ejtag_write_HILO_reg(s32 reg_idx, u32 reg_val)
{
	ejtag_run_assembly(MTC0(V0, CP0_DESAVE, 0));
	ejtag_run_assembly(LUI(V0, (reg_val >> 16) & 0xFFFF));
	ejtag_run_assembly(ORI(V0, V0, reg_val & 0xFFFF));

	if (reg_idx == HI) 
		ejtag_run_assembly(MTHI(V0));
	else 
		ejtag_run_assembly(MTLO(V0));

	ejtag_run_assembly(MFC0(V0, CP0_DESAVE, 0));
	
	return;
}

/*----------------------------------------------------------------------------*/
u32 ejtag_read_HILO_reg(s32 reg_num)
{
	u32 val = 0;

	if (reg_num != HI && reg_num != LO)
		return 0;

	ejtag_run_assembly(MTC0(V0, CP0_DESAVE, 0));
	ejtag_run_assembly(LUI(V0, (EJTAG_BASE >> 16) & 0xFFFF));

	if (reg_num == HI_REGNUM ) 
	    ejtag_run_assembly(MFHI(AT));
	else 
	    ejtag_run_assembly(MFLO(AT));
	
	ejtag_run_assembly(SW(AT, 0, V0));
	ejtag_run_assembly(NOP);
	val = ejtag_forward_word();

	/* restore original v0 value */
	ejtag_run_assembly(MFC0(V0, CP0_DESAVE, 0));

	return val;
}

/*----------------------------------------------------------------------------*/
void byte2strbuf(s8 *ptr, u8 binary)
{
	u8 bdata;
	if (isprint(binary)) {
		bdata = binary;
	} else {
		bdata = '.';
	}

	*ptr = bdata;

	return;
}

/*----------------------------------------------------------------------------*/
void write_to_strbuf(s8 *ptr, void *data, s32 datasize)
{
	int i;
    u32 ldata, *lptr;
	u16 wdata, *wptr;
	u8 bdata, *bptr;
	u8 *cptr;

	if (datasize == 4) 
	{
		lptr = data;
		ldata = le32toh(*lptr);
		cptr = (u8 *)&ldata;
		byte2strbuf(ptr, *cptr);
		cptr++;
		byte2strbuf(ptr + 1, *cptr);
		cptr++;
		byte2strbuf(ptr + 2, *cptr);
		cptr++;
		byte2strbuf(ptr + 3, *cptr);
	} 
	else if (datasize == 2) 
	{
		wptr = data;
		wdata = le16toh(*wptr);
		cptr = (u8 *)&wdata;
		byte2strbuf(ptr, *cptr);
		cptr++;
		byte2strbuf(ptr + 1, *cptr);
		cptr++;
	} 
	else if (datasize == 1) 
	{
		bptr = data;
		bdata = *bptr;
		cptr = (u8 *)&bdata;
		byte2strbuf(ptr, *cptr);
	} else {
		ejtag_print(FOR_EJTAG, "%s: datasize wrong\n", __FUNCTION__);
	}

	return;
}

/*----------------------------------------------------------------------------*/
void print_strbuf(s8 str_buf[], u32 strbuf_size)
{
	int i = 0;
	
	for (; i < strbuf_size; i++) 
		ejtag_print(FOR_EJTAG, "%c", str_buf[i]);

	return;
}

/*----------------------------------------------------------------------------*/
s32 ejtag_read_one_data(u32 addr, u32 data_size)
{
	u32 val = 0;

	if (!(0x01 == data_size || 0x02 == data_size || 0x04 == data_size))
	{
		ejtag_print(FOR_EJTAG, "ejtag_read_one_data data_size error = %d\n", 	
														data_size);
		return 0;
	}

    /* store current v0 value in debug reg */
    ejtag_run_assembly(MTC0(V0, CP0_DESAVE, 0));

    /* load v0 with address */ 
    ejtag_run_assembly(LUI(V0, (addr >> 16) & 0xffff));
    ejtag_run_assembly(ORI(V0, V0, addr & 0xffff));

    /* load at */
	if (1 == data_size)
		ejtag_run_assembly(LBU(AT, 0, V0));
	else if (2 == data_size)
		ejtag_run_assembly(LHU(AT, 0, V0));
	else
		ejtag_run_assembly(LW(AT, 0, V0));
      
    /* load v0 with data */
    ejtag_run_assembly(LUI(V0, (EJTAG_BASE >> 16) & 0xFFFF));
    ejtag_run_assembly(ORI(V0, V0, EJTAG_BASE & 0xFFFF));

    /* store at */
    ejtag_run_assembly(SW(AT, 0, V0));
    ejtag_run_assembly(NOP);
    val = ejtag_forward_word();

	ejtag_run_assembly(MFC0(V0, CP0_DESAVE, 0));

    return val ;
}
/*----------------------------------------------------------------------------*/
s32 ejtag_write_one_data(u32 addr, u32 data, u32 data_size)
{
	if (!(0x01 == data_size || 0x02 == data_size || 0x04 == data_size))
	{
		ejtag_print(FOR_EJTAG, "ejtag_write_one_data data_size error = %d\n", 
														data_size);
		return 0;
	}
    /* store current v0 value in debug reg */
    ejtag_run_assembly(MTC0(V0, CP0_DESAVE, 0));

    /* load v0 with address */ 
    ejtag_run_assembly(LUI(V0, (addr >> 16) & 0xffff));
    ejtag_run_assembly(ORI(V0, V0, addr & 0xffff));

    /* load at with data */ 
    ejtag_run_assembly(LUI(AT, (data >> 16) & 0xffff));
    ejtag_run_assembly(ORI(AT, AT, data & 0xffff));

    /* load at */
	if (1 == data_size)
		ejtag_run_assembly(SB(AT, 0, V0));
	else if (2 == data_size)
		ejtag_run_assembly(SH(AT, 0, V0));
	else
		ejtag_run_assembly(SW(AT, 0, V0));
	ejtag_run_assembly(MIPS_SYNC);

	/* restore v0 */
	ejtag_run_assembly(MFC0(V0, CP0_DESAVE, 0));

	return data;
}
/*----------------------------------------------------------------------------*/
s32 cmd_get_data_size(s8 *arg)
{
	/* Check for a size specification .b, .h or .w. */
	s32 len = strlen(arg);
	
	if (len > 2 && arg[len-2] == '.') 
	{
		switch(arg[len-1]) 
		{
			case 'b':
				return 1;
			case 'h':
				return 2;
			case 'w':
				return 4;
			default:
				break;
		}
	}
	return DEFAULT_DATA_SIZE;
}
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
void ejtag_set_reg(u32 reg_idx, u32 reg_val)
{

    if (reg_idx < ZERO_REGNUM || reg_idx > LAST_EMBED_REGNUM)
        return;

    if ( reg_idx >= ZERO_REGNUM && reg_idx <= RA_REGNUM) 
		ejtag_write_general_reg(reg_idx, reg_val);
	else if (reg_idx > RA_REGNUM && reg_idx < FP0_REGNUM) 
	{
        switch (reg_idx)
		{
			case PS_REGNUM:
				ejtag_write_cp0_reg(CP0_STATUS, 0, reg_val);
				break ;
			case HI_REGNUM:
			case LO_REGNUM:
				ejtag_write_HILO_reg(reg_idx, reg_val);           
				break ;
			case BADVADDR_REGNUM:
				ejtag_write_cp0_reg(CP0_BADVADDR, 0, reg_val);
				break ;
			case CAUSE_REGNUM:
				ejtag_write_cp0_reg(CP0_CAUSE, 0, reg_val);
				break ;
			case PC_REGNUM:
				ejtag_write_cp0_reg(CP0_DEPC, 0, reg_val);
				break ;
			case EBASE_REGNUM:
				ejtag_write_cp0_reg(CP0_EBASE, 0, reg_val);
				break;
        }
    } 
	else if (reg_idx >= FP0_REGNUM && reg_idx <= FP_REGNUM) 
	{
		switch (reg_idx) 
		{
			case EBASE_REGNUM:
				ejtag_write_cp0_reg(CP0_EBASE, 1, reg_val);
				break;
		}
    } 
	else 
        ejtag_write_cp0_reg(reg_idx - FIRST_EMBED_REGNUM, 0, reg_val);

	return;
}

/*----------------------------------------------------------------------------*/
u32 ejtag_get_reg(u32 reg_num)
{
	u32 a0 = 0;
    u32 val = 0;

    if (reg_num < ZERO_REGNUM ||  reg_num > LAST_EMBED_REGNUM)
    {
		ejtag_print(FOR_EJTAG, "\nreg index wrong reg_num[%d]", reg_num);
		return 0;
    }
	
    if (reg_num >= ZERO_REGNUM && reg_num <= RA_REGNUM) 
	{
		val = ejtag_read_general_reg(reg_num);
	}
	else if (reg_num > RA_REGNUM && reg_num < FP0_REGNUM)
	{ 
        switch(reg_num)
		{
        	case PS_REGNUM:
            	val = ejtag_read_cp0_reg(CP0_STATUS, 0);
            	break ;
        	case HI_REGNUM:
        	case LO_REGNUM:  
				val = ejtag_read_HILO_reg(reg_num);
            	break;
        	case BADVADDR_REGNUM:
            	val = ejtag_read_cp0_reg(CP0_BADVADDR, 0);
            	break;
        	case CAUSE_REGNUM:
            	val = ejtag_read_cp0_reg(CP0_CAUSE, 0);
            	break;
        	case PC_REGNUM:
            	val = ejtag_read_cp0_reg(CP0_DEPC, 0);
            	break;
        }
    }
	else if (reg_num >= FP0_REGNUM && reg_num <= FP_REGNUM)
	{
		switch (reg_num)
		{
			case EBASE_REGNUM:
				val = ejtag_read_cp0_reg(CP0_EBASE, 1);
				break;
			default:
				val = 0;
		}
	}
	else
	{
		val = ejtag_read_cp0_reg(reg_num - FIRST_EMBED_REGNUM, 0);
	}

    return val;
}

/*----------------------------------------------------------------------------*/
void ejtag_release(void)
{	
    ejtag_run_assembly(MIPS_SYNC);
    ejtag_run_assembly(NOP);
    ejtag_run_assembly(DERET);

	gst_mips_ejtag.ejtag_mode = NON_DEBUG;

	return;
}

/*----------------------------------------------------------------------------*/
void show_mips_regs(void)
{
    int i = 0;
    int reg = 0;


    for(i = 0; i <= PC_REGNUM; i++) 
	{
        if (i && !(i % 4)) 
			ejtag_print(FOR_EJTAG, "\n");
        reg = ejtag_get_reg(i);
        ejtag_print(FOR_EJTAG, "%5s(%3d):%08x ", reg_names[i], i, reg);
    }
    ejtag_print(FOR_EJTAG, "\n");

    for (i = FIRST_EMBED_REGNUM; i <= LAST_EMBED_REGNUM; i++) 
	{
        if (i - FIRST_EMBED_REGNUM && !((i - FIRST_EMBED_REGNUM) % 4)) 
			ejtag_print(FOR_EJTAG, "\n");
        reg = ejtag_get_reg(i);
		
        ejtag_print(FOR_EJTAG, "%5s(%3d):%08x ", reg_names[i], i, reg);
    }

    ejtag_print(FOR_EJTAG, "\n");
}

/*----------------------------------------------------------------------------*/
void ejtag_mem_display(u32 start_addr, u32 count, u32 data_size)
{
	u32 step = 0;
	s8 str_buf[16] = { 0 };
	u32 addr = start_addr & ~0x0F;
	u32 end_addr = start_addr + count;
	u32 data_t = 0;
	u32 data_w = 0;
	u16 data_h = 0;
	u8 data_b = 0;
	s8 *ptr = str_buf;


	if (!(0x01 == data_size || 0x02 == data_size || 0x04 == data_size))
		data_size = 4;
	step = data_size;

	/* each 16 bytes form a line. */
    for (; addr < end_addr; addr += step) 
	{
        if ((addr & 0xF) == 0) 
		{
			ptr = str_buf;
			memset(str_buf, 0x00, sizeof(str_buf));
			ejtag_print(FOR_EJTAG, "0x%08x: ", addr);
		}

		data_t = ejtag_read_one_data(addr, data_size);
		if (data_size == 4) {
			data_w = (u32)data_t;
			write_to_strbuf(ptr, &data_w, 4);
			ptr += 4;
			ejtag_print(FOR_EJTAG, "%08x ", data_w);

		} else if (data_size == 2) {
			data_h = (u16)(data_t & 0xffff);
			write_to_strbuf(ptr, &data_h, 2);
			ptr += 2;
			ejtag_print(FOR_EJTAG, "%04x ", data_h);

		} else if (data_size == 1) {
			data_b = (u8)(data_t & 0xff);
			write_to_strbuf(ptr, &data_b, 1);
			ptr += 1;
			ejtag_print(FOR_EJTAG, "%02x ", data_b);
		}
        
		// end of the data or end of a line
		if (((addr + step) & 0xf) == 0 || (addr + step) == end_addr) 
		{
			ejtag_print(FOR_EJTAG, "\t");
			print_strbuf(str_buf, sizeof(str_buf));
			ejtag_print(FOR_EJTAG, "\n");
		}
    }
	ejtag_print(FOR_EJTAG, "\n");
	
	return;
}

/*----------------------------------------------------------------------------*/
void ejtag_mem_read(u32 start_addr, u32 count, u32 data_size)
{
	u32 end_addr;
	u32 addr;
	u32 data;


	addr = start_addr;
	end_addr = addr + count;
	
	while (addr < end_addr) 
	{
		ejtag_print(FOR_EJTAG, "0x%08x: ", addr);
		data = ejtag_read_one_data(addr, data_size);
		if (4 == data_size) {
			u32 ldata = le32toh(data);
			ejtag_print(FOR_EJTAG, "%08x\n", ldata);
		} else if (2 == data_size) {
			u16 wdata = (u16)(data & 0xffff);
			ejtag_print(FOR_EJTAG, "%04x\n", wdata);
		} else if (1 == data_size) {
			u8 bdata = (u8)(data & 0xff);
			ejtag_print(FOR_EJTAG, "%02x\n", bdata);
		}
		addr += data_size;
	}

	return;
}
/*----------------------------------------------------------------------------*/
void ejtag_mem_write(u32 addr, u32 data, u32 data_size)
{

	if (4 == data_size)
		ejtag_write_one_data(addr, data, data_size);
	else if (2 == data_size)
		ejtag_write_one_data(addr, data & 0xFFFF, data_size);
	else if (1 == data_size) 
		ejtag_write_one_data(addr, data & 0xFF, data_size);

	return;
}

/*----------------------------------------------------------------------------*/
void ejtag_fill_mem(u32 addr, s32 val, u32 count)
{
	u32 i = 0;
	u32 offset = 0x00;
	
	for (offset = 0; offset < count; offset += 4) 
	{
		ejtag_write_one_data(addr + offset, val, 4);
		if (offset%64 == 0) 
		{
			if (i%32 == 0)
				ejtag_print(FOR_EJTAG,"\n");
			ejtag_print(FOR_EJTAG, ".");
			i++;
		}
	}
	ejtag_print(FOR_EJTAG,"\n");

	return;
}

/*----------------------------------------------------------------------------*/
void ejtag_mem_write_multi_w(u32 addr, s32 *buffer, u32 size)
{
	int i,j;
	int data;

	if (size > 0xFFFF)
	{
		ejtag_print(FOR_EJTAG, "Write Mem Size out range\n ");
	    return;
	}

	ejtag_run_assembly(MTC0(V0, CP0_DESAVE, 0));
	ejtag_run_assembly(LUI(V0, (addr >> 16) & 0xFFFF));
	ejtag_run_assembly(ORI(V0, V0, addr & 0xFFFF));

	for (i = 0; i < size; )
	{
		ejtag_init_buffer();
		for (j = 0; j < 16; j++)
		{
			data = *buffer;
			buffer++;
			ejtag_add_assembly_buffer(LUI(AT, (data >> 16) & 0xFFFF));
			ejtag_add_assembly_buffer(ORI(AT, AT, data & 0xFFFF));
			ejtag_add_assembly_buffer(SW(AT, i, V0));
			i += 4;
		}
		ejtag_send_buffer(EJG_NO_RET);

	}
	ejtag_run_assembly(MFC0(V0, CP0_DESAVE, 0));

	return;
}

/*----------------------------------------------------------------------------*/
void ejtag_go_address(u32 addr)
{
	ejtag_run_assembly(LUI (V0, (addr >> 16) & 0xFFFF));
	ejtag_run_assembly(ORI (V0, V0, addr & 0xFFFF));
	ejtag_run_assembly(MTC0 (V0, CP0_DEPC,0));
	ejtag_run_assembly(DERET);

	gst_mips_ejtag.ejtag_mode = NON_DEBUG;
}

/*----------------------------------------------------------------------------*/
BREAK_POINT *list_find_break_point(int addr)
{
    BREAK_POINT *b_point = NULL;

    b_point = gst_mips_debug.list_bp_head.next;
	
    LOCK_BP_LIST();
	while(b_point != NULL )
    {	
    	if (b_point->address == addr)
    	{
    		UNLOCK_BP_LIST();
        	return b_point;
    	}		
		b_point = b_point->next;
	}

	UNLOCK_BP_LIST();	
    return NULL;
}
void list_add_break_point(BREAK_POINT *b_point)
{
	BREAK_POINT *t_pos = &gst_mips_debug.list_bp_head;
	b_point->next = NULL;

	LOCK_BP_LIST();
	while (NULL != t_pos->next)
	{
		if (t_pos->next->address == b_point->address)
		{
			ejtag_print(FOR_EJTAG, "Address[0x%08x] have already bp\n",
											b_point->address);
			goto unlock;
		}
		t_pos = t_pos->next;
	}
	t_pos->next = b_point;
	gst_mips_debug.bp_num++;
	ejtag_print(FOR_EJTAG, " bp_num [%d] addr[0x%08x], code[0x%08x]\n", 
										gst_mips_debug.bp_num,
										b_point->address,
										b_point->code);

unlock:
	UNLOCK_BP_LIST();
	
	return;
}
void list_del_break_point(BREAK_POINT *b_point)
{
	BREAK_POINT *t_pos = &gst_mips_debug.list_bp_head;

	LOCK_BP_LIST();
	while (NULL != t_pos->next)
	{
		if (b_point == t_pos->next)
			break;
		t_pos = t_pos->next;
	}
	
	if (NULL == t_pos->next)
	{
		ejtag_print(FOR_EJTAG, "Address[0x08%x], Is Not A breadpoint!\n", 
										b_point->address);
		goto unlock;
	}
	t_pos->next = t_pos->next->next;
	gst_mips_debug.bp_num--;
	ejtag_print(FOR_EJTAG, " bp_num [%d] addr[0x%08x], code[0x%08x]\n", 
										gst_mips_debug.bp_num,
										b_point->address,
										b_point->code);
	free(b_point);
unlock:
	UNLOCK_BP_LIST();

	return;
}
/*----------------------------------------------------------------------------*/
void ejtag_flush_mips_cache(u32 addr, MIPS_CACHE_MODE cache_type)
{
    /* save current v0 value */
    ejtag_run_assembly(MTC0(V0, CP0_DESAVE, 0));
    /* load v0 with value we want to set cp0 register to */ 
    ejtag_run_assembly(LUI(V0, (addr >> 16) & 0xFFFF));
    ejtag_run_assembly(ORI(V0, V0, addr & 0xFFFF));

    ejtag_run_assembly(CACHE((0x14 | DCACHE),0,V0));
	
    /* restore original v0 value */
    ejtag_run_assembly(MFC0(V0, CP0_DESAVE, 0));
}

/*----------------------------------------------------------------------------*/
void ejtag_del_bp(u32 addr)
{
	BREAK_POINT *b_point = NULL;

	b_point = list_find_break_point(addr);
	if (NULL == b_point)
	{
		ejtag_print(FOR_EJTAG, "\nAddress[0x%08x], Is Not Breakpoint", addr);
		return;
	}

	if (b_point->bp_time <= 0)
	{
		/* write back mips code */
		ejtag_write_one_data(addr, b_point->code, 4);
		// flush the caches
		ejtag_flush_mips_cache(addr, DATA_CACHE);
		ejtag_run_assembly(MIPS_SYNC);
		ejtag_flush_mips_cache(addr, CODE_CACHE);
		ejtag_run_assembly(MIPS_SYNC);

		list_del_break_point(b_point);
	}
	b_point->bp_time--;
	
	return;
}

/*----------------------------------------------------------------------------*/
void ejtag_add_bp(u32 addr)
{
	BREAK_POINT *b_point = NULL;
	BREAK_POINT *new_point = NULL;

	// allocate space for breakpoint
	new_point = malloc(sizeof(BREAK_POINT));
	assert(new_point);
	memset(new_point, 0x00, sizeof(BREAK_POINT));
	
	b_point = list_find_break_point(addr);
	if (NULL == b_point)
	{
		// update BREAK_POINT data structure
		new_point->address = addr;
		new_point->code = ejtag_read_one_data(addr, 4);
		// write breakpoint instruction into memory
		ejtag_write_one_data(addr, SDBBP(0), 4);
		
		// flush the caches
		ejtag_flush_mips_cache(addr, DATA_CACHE);
		ejtag_run_assembly(MIPS_SYNC);
		ejtag_flush_mips_cache(addr, CODE_CACHE);
		ejtag_run_assembly(MIPS_SYNC);

		// add bp to list
		list_add_break_point(new_point);
	}
	else
	{
		memcpy(new_point, b_point, sizeof(BREAK_POINT));
		new_point->bp_time++;
	}

	return;
}

/*----------------------------------------------------------------------------*/
void ejtag_show_bp_info(void)
{
	u32 i = 0;
	BREAK_POINT *t_bp = gst_mips_debug.list_bp_head.next;
	
	ejtag_print(FOR_EJTAG, "BreakPoint Num : %d\n", gst_mips_debug.bp_num);
	for (i = 0; i < gst_mips_debug.bp_num; i++)
	{
		ejtag_print(FOR_EJTAG, 
				" bp_%02d : addr[0x%08x], code[0x%08x], bp_time[%d]\n", 
											i + 1,
											t_bp->address,
											t_bp->code,
											t_bp->bp_time);
		t_bp = t_bp->next;
	}

	return;
}

/*----------------------------------------------------------------------------*/
u32 ejtag_del_all_bp(void)
{
	u32 i = 0;
	u32 bp_num = gst_mips_debug.bp_num;
	BREAK_POINT *t_bp = gst_mips_debug.list_bp_head.next;
	MIPS_EJTAG_MODE record = gst_mips_ejtag.ejtag_mode;
	
	ejtag_print(FOR_EJTAG, "BreakPoint Num : %d\n", gst_mips_debug.bp_num);
	for (i = 0; i < bp_num; i++)
	{
		//ejtag_print(FOR_EJTAG, " delete bp_%02d : addr[0x%08x], mips_code[0x%08x]\n", 
		//									i + 1,
		//									t_bp->address,
		//									t_bp->code);
		EJTAG_INIT_WAIT_PENDING_LOAD();	
		if (jtag_break_valid)
		{
			ejtag_del_bp(t_bp->address);
		}
		t_bp = t_bp->next;
	}
	if (NON_DEBUG == record && DEBUG == gst_mips_ejtag.ejtag_mode)
		ejtag_release();
	
	return 0;
}

/*----------------------------------------------------------------------------*/
void ejtag_init_step_mode(MIPS_STEP_MODE step_mode)
{
    u32 cp0_debug = 0;

    cp0_debug = ejtag_read_cp0_reg(CP0_DEBUG, 0);
    if (USE_STEP == step_mode)
        cp0_debug |= CP0_DEBUG_SST_EN;
    else
        cp0_debug &= ~CP0_DEBUG_SST_EN;
    ejtag_write_cp0_reg(CP0_DEBUG, 0, cp0_debug);

	return;
}

/*----------------------------------------------------------------------------*/
void ejtag_boot(void)
{
	u32 ctrl_reg ;
	tap_do_instruction(EJG_CONTROL);
	ctrl_reg = tap_wr_data(0x8000c000);

	ctrl_reg = ctrl_reg | PRRST;
	tap_do_instruction(EJG_CONTROL);
	tap_wr_data(ctrl_reg);
	ejtag_print(FOR_EJTAG, "Set PRRST bit, Proccessor Reset! --->pls enter");
    getchar();
	
    ctrl_reg = ctrl_reg | PROBEN;
	tap_do_instruction(EJG_CONTROL);
	tap_wr_data(ctrl_reg);
	ejtag_print(FOR_EJTAG, "Set PROBEN bit!");

	tap_do_instruction(EJG_EJTAGBOOT);
    ejtag_print(FOR_EJTAG, "Do EJG_EJTAGBOOT; pls reset board --->enter");
    getchar();
	
	ctrl_reg = ctrl_reg & (~PRRST);
	tap_do_instruction(EJG_CONTROL);
	ejtag_print(FOR_EJTAG, "ctrl_reg[0x%08x]\n", tap_wr_data(ctrl_reg));

	return;
}

/*----------------------------------------------------------------------------*/
void ejtag_ddr_init(void)
{
	ejtag_print(FOR_EJTAG, "Starting DDR2 PCTL && PHY initialization.\n");
	while (!(ejtag_read(HE3250_DDR2_DFISTSTAT0) & 0x01)) {
	}
	ejtag_print(FOR_EJTAG, "------>DDR2 PHY initialization complete.\n");


	ejtag_write(HE3250_DDR2_TOGCNT1U, 		0x37);
	ejtag_write(HE3250_DDR2_TINIT, 			0xc8);
	ejtag_write(HE3250_DDR2_TOGCNT100N, 	0x06);
	ejtag_write(HE3250_DDR2_TREFI, 			0x4e);
	ejtag_write(HE3250_DDR2_TMRD, 			0x02);
	ejtag_write(HE3250_DDR2_TRFC, 			0x1a);
	ejtag_write(HE3250_DDR2_TAL, 			0x00);
	ejtag_write(HE3250_DDR2_TCL, 			0x04);
	ejtag_write(HE3250_DDR2_TCWL, 			0x03);
	ejtag_write(HE3250_DDR2_TRCD, 			0x04);
	ejtag_write(HE3250_DDR2_TEXSR, 			0xc8);


	ejtag_print(FOR_EJTAG, 
				"Programming DDR2 PCTL to execute memory init sequence.\n");
	ejtag_write(HE3250_DDR2_MCMD, 			0x80f00000);
	while (ejtag_read(HE3250_DDR2_MCMD) & 0x80000000) {
	}

	ejtag_write(HE3250_DDR2_MCMD, 			0x80f00001);
	while (ejtag_read(HE3250_DDR2_MCMD) & 0x80000000) {
		;
	}

	ejtag_write(HE3250_DDR2_MCMD, 			0x80f40003);
	while (ejtag_read(HE3250_DDR2_MCMD) & 0x80000000) {
		;
	}

	ejtag_write(HE3250_DDR2_MCMD, 			0x80f60003);
	while (ejtag_read(HE3250_DDR2_MCMD) & 0x80000000) {
		;
	}

	ejtag_write(HE3250_DDR2_MCMD, 			0x80f20023);
	while (ejtag_read(HE3250_DDR2_MCMD) & 0x80000000) {
		;
	}

	ejtag_write(HE3250_DDR2_MCMD, 			0x80f07433);
	while (ejtag_read(HE3250_DDR2_MCMD) & 0x80000000) {
		;
	}

	ejtag_write(HE3250_DDR2_MCMD, 			0x80f00001);
	while (ejtag_read(HE3250_DDR2_MCMD) & 0x80000000) {
		;
	}

	ejtag_write(HE3250_DDR2_MCMD, 			0x80f00002);
	while (ejtag_read(HE3250_DDR2_MCMD) & 0x80000000) {
		;
	}

	ejtag_write(HE3250_DDR2_MCMD, 			0x80f00002);
	while (ejtag_read(HE3250_DDR2_MCMD) & 0x80000000) {
		;
	}

	ejtag_write(HE3250_DDR2_MCMD, 			0x80f06433);
	while (ejtag_read(HE3250_DDR2_MCMD) & 0x80000000) {
		;
	}

	ejtag_write(HE3250_DDR2_MCMD, 			0x80f23823);
	while (ejtag_read(HE3250_DDR2_MCMD) & 0x80000000) {
		;
	}

	ejtag_write(HE3250_DDR2_MCMD, 			0x80f20023);
	while (ejtag_read(HE3250_DDR2_MCMD) & 0x80000000) {
		;
	}
	ejtag_print(FOR_EJTAG, "----->Memory init sequence complete.\n");

	ejtag_write(HE3250_DDR2_SCTL, 			0x01);

	//
	while (0x01 != (ejtag_read(HE3250_DDR2_STAT) & 0x7)) {
		;
	}
	ejtag_write(HE3250_DDR2_DFITPHYWRLAT, 	0x02);
	ejtag_write(HE3250_DDR2_DFITRDDATAEN, 	0x02);
	ejtag_write(HE3250_DDR2_DFITCTRLDELAY, 	0x04);
	ejtag_write(HE3250_DDR2_DFITPHYRDLAT, 	0x0b);
	ejtag_write(HE3250_DDR2_DCFG, 			0x112);

	ejtag_write(HE3250_DDR2_MCFG, 			0x40001);
	ejtag_write(HE3250_DDR2_PCFG_1, 		0x30);
	ejtag_write(HE3250_DDR2_SCTL, 			0x02);

	while (0x03 != (ejtag_read(HE3250_DDR2_STAT) & 0x7)) {
		;
	}
	ejtag_print(FOR_EJTAG, "----->DDR2 PCTL and PHY init complete.\n");


	return;	
}


/*----------------------------------------------------------------------------*/















/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/


#if 0
//for show reg
#endif


/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
void ejtag_show_cp0_debug(void)
{
    s32 dbgreg;
  
    dbgreg = ejtag_read_cp0_reg(CP0_DEBUG, 0);

    ejtag_print(FOR_EJTAG, "\nDEBUG REGISTER: %08x\n",dbgreg);
    ejtag_print(FOR_EJTAG, "   Single-step exc:       %d(%s)\n", 
			(dbgreg & CP0_DEBUG_SS_EXC) ? 1 : 0,
			(dbgreg & CP0_DEBUG_SS_EXC) ? "happened" : "not happen");
    ejtag_print(FOR_EJTAG, "   Break-instr exc:       %d(%s)\n", 
			(dbgreg & CP0_DEBUG_BP_EXC) ? 1 : 0,
			(dbgreg & CP0_DEBUG_BP_EXC) ? "happened" : "not happen");
    ejtag_print(FOR_EJTAG, "   Data-addr load exc:    %d(%s)\n", 
			(dbgreg & CP0_DEBUG_DBL_EXC) ? 1 : 0,
			(dbgreg&CP0_DEBUG_DBL_EXC) ? "happened" : "not happen");
    ejtag_print(FOR_EJTAG, "   Data-addr store exc:   %d(%s)\n", 
			(dbgreg & CP0_DEBUG_DBS_EXC) ? 1 : 0,
			(dbgreg & CP0_DEBUG_DBS_EXC) ? "happened" : "not happen");
    ejtag_print(FOR_EJTAG, "   Inst-addr load exc:    %d(%s)\n", 
			(dbgreg & CP0_DEBUG_DIB_EXC) ? 1 : 0,
			(dbgreg & CP0_DEBUG_DIB_EXC) ? "happened" : "not happen");
    ejtag_print(FOR_EJTAG, "   Debug intr exc:        %d(%s)\n", 
			(dbgreg & CP0_DEBUG_DINT_EXC) ? 1 : 0,
			(dbgreg & CP0_DEBUG_DINT_EXC) ? "happened" : "not happen");
    ejtag_print(FOR_EJTAG, "   Single-step enable:    %d(%s)\n", 
			(dbgreg & CP0_DEBUG_SST_EN) ? 1 : 0,
			(dbgreg & CP0_DEBUG_SST_EN) ? "enable" : "disable");
    ejtag_print(FOR_EJTAG, "   DEXC code (see CAUSE): %x\n", 
			(dbgreg & CP0_DEBUG_EXC_CODE_MASK) >> CP0_DEBUG_EXC_CODE_SHIFT);
    ejtag_print(FOR_EJTAG, "   Data break load impr.  %d(%s)\n", 
			(dbgreg & CP0_DEBUG_DDBL) ? 1 : 0,
			(dbgreg & CP0_DEBUG_DDBL) ? "match" : "dismatch");
    ejtag_print(FOR_EJTAG, "   Data break store impr. %d(%s)\n", 
			(dbgreg & CP0_DEBUG_DDBS) ? 1 : 0,
			(dbgreg & CP0_DEBUG_DDBS) ? "match" : "dismatch");
    ejtag_print(FOR_EJTAG, "   Imprec. err exc inhib. %d(%s)\n", 
			(dbgreg & CP0_DEBUG_IEXI) ? 1 : 0,
			(dbgreg & CP0_DEBUG_IEXI) ? "happened" : "not happen");
    ejtag_print(FOR_EJTAG, "   Dbus err exc pending   %d(%s)\n", 
			(dbgreg & CP0_DEBUG_DBEP) ? 1 : 0,
			(dbgreg & CP0_DEBUG_DBEP) ? "happened" : "not happen");
    ejtag_print(FOR_EJTAG, "   Cache err exc pending  %d(%s)\n", 
			(dbgreg & CP0_DEBUG_CAEP) ? 1 : 0, 
			(dbgreg & CP0_DEBUG_CAEP) ? "happened" : "not happen");
    ejtag_print(FOR_EJTAG, "   Mach check exc pending %d(%s)\n", 
			(dbgreg & CP0_DEBUG_MCEP) ? 1 : 0,
			(dbgreg & CP0_DEBUG_MCEP) ? "happened" : "not happen");
    ejtag_print(FOR_EJTAG, "   Instr fetch exc pend.  %d(%s)\n", 
			(dbgreg & CP0_DEBUG_IFEP) ? 1 : 0,
			(dbgreg & CP0_DEBUG_IFEP) ? "happened" : "not happen");
    ejtag_print(FOR_EJTAG, "   Count runs in debug    %d(%s)\n", 
			(dbgreg & CP0_DEBUG_CNT_DM) ? 1 : 0,
			(dbgreg & CP0_DEBUG_CNT_DM) ? "run" : "stop");
    ejtag_print(FOR_EJTAG, "   System bus halted      %d(%s)\n", 
			(dbgreg & CP0_DEBUG_HALT) ? 1 : 0,
			(dbgreg & CP0_DEBUG_HALT) ? "stop" : "run");
    ejtag_print(FOR_EJTAG, "   DSEG addr to main mem  %d(%s)\n", 
			(dbgreg & CP0_DEBUG_LSDM) ? 1 : 0,
			(dbgreg & CP0_DEBUG_LSDM) ? "no" : "yes");
    ejtag_print(FOR_EJTAG, "   Debug Mode             %d(%s)\n", 
			(dbgreg & CP0_DEBUG_DM) ? 1 : 0,
			(dbgreg & CP0_DEBUG_DM) ? "happened" : "not happened");
    ejtag_print(FOR_EJTAG, "   Debug exc branch dly   %d(%s)\n", 
			(dbgreg & CP0_DEBUG_DBD) ? 1 : 0,
			(dbgreg & CP0_DEBUG_DBD) ? "in" : "not in");
}

/*----------------------------------------------------------------------------*/
void ejtag_show_reg(s8 *argv[])
{
	s8 *endp;
	u32 regnum;

	if (NULL == argv[2]) 
	{
		ejtag_print(FOR_EJTAG, "U Should Give reg index for show_reg\n");
		return;
	}
	regnum = strtoul(argv[2], &endp, 10);
	if (NULL == endp)
	{
		ejtag_print(FOR_EJTAG, "U Should Give reg index[%s] error\n", argv[2]);
		return;
	}
	ejtag_print(FOR_EJTAG, "%s(%d): 0x%08x\n", reg_names[regnum], regnum,
							   ejtag_get_reg(regnum));

	return;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/







#if 0
#endif


void ejtag_test(void)
{
	ejtag_print(FOR_EJTAG, "For Test\n");
}
/*----------------------------------------------------------------------------*/















