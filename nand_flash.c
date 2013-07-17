/*----------------------------------------------------------------------------*/
/*
** EJTAG Command
**
** Date 	: 2013-07-02
** Mark		: Modify By wangshuke<anson.wang@gmail.com>
**----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>
#include "nand.h"
#include "command_def.h"

/*he3250 nand controller regs*/
#define HE3250_NAND_BASE	0xBC044000
//#define HE3250_NAND_BASE	0xBC009300
#define HE3250_NAND_FLCONF	0x1300 //0x00
#define HE3250_NAND_FLCTRL	0x1304 //0x04
#define HE3250_NAND_FLCOMM	0x1308 //0x08
#define HE3250_NAND_FLADDR0L	0x130c //0x0c
#define HE3250_NAND_FLADDR1L	0x1310 //0x10
#define HE3250_NAND_FLDATA	0x1314 //0x14
#define HE3250_NAND_FLPB0	0x1318 //0x18
#define HE3250_NAND_FLSTATE	0x131c //0x1c
/*......lookup regs [0:7]&lookup enable.........from 0x1320  to  0x133c*/
#define HE3250_NAND_FLECCSTATUS	0x1340 //0x40
#define HE3250_NAND_FLADDR0H	0x1344 //0x44
#define HE3250_NAND_FLADDR1H	0x1348 //0x48
//#define HE3250_NAND_FLLOOKUPEN 0x134c //0x4c
/*DMA REGS: address reg ,control reg ,configuration reg*/
#define HE3250_NAND_FLDMAADDR	0x1380 //0x80
#define HE3250_NAND_FLDMACTRL	0x1384 //0x84
#define HE3250_NAND_FLDMACNTR	0x1388 //0x88

//model 0
#define PSM	0x1
#define	TWB	0x13
#define	TWHR 	0x13
#define	TWP 	0x9
#define	TRP 	0x9
#define	TWH 	0x4


/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
struct nand_flash {
	int page_size;
	int page_shift;
	int page_mask;
	int block_size;
	int block_shift;
	int oob_size;
	int chip_size;
	struct nand_flash_dev *dev;
	struct nand_manufacturers *manf;
};

/*----------------------------------------------------------------------------*/
static struct nand_flash flash = {0};

extern struct nand_flash_dev *get_nand_flash_dev_by_id(int dev_id);
extern struct nand_manufacturers *get_nand_manuf_by_id(int maf_id);

int nand_erase(unsigned long offset);

extern void ejtag_write_one_data(u32 addr, u32 data, u32 data_size);
extern s32 ejtag_read_one_data(u32 addr, u32 data_size);

/*----------------------------------------------------------------------------*/
static s32 ffbs(s32 x)
{
	s32 r = 1;

	if (!x)
		return 0;
	
	if (!(x & 0xffff)) 
	{
	    x >>= 16;
	    r += 16;
	}
	if (!(x & 0xff)) 
	{
	    x >>= 8;
	    r += 8;
	}
	if (!(x & 0xf)) 
	{
	    x >>= 4;
	    r += 4;
	}
	if (!(x & 3)) 
	{
	    x >>= 2;
	    r += 2;
	}
	if (!(x & 1)) 
	{
	    x >>= 1;
	    r += 1;
	}

	return r;
}

/*----------------------------------------------------------------------------*/
s32 nand_init(void)
{
	u32 ret;
	u32 conf,ctrl;
	int maf_id = 0;
	int dev_id = 0;
	int id_4th = 0;

	ejtag_write(0xbc04a00c,0xdc);
	ejtag_write(0xbc04a010,0x40000000);//soft reset nfc
	ejtag_write(0xbc04a010,0x0);
	ejtag_write(0xbc04a0b0,0x4);
	ejtag_write(0xbc04a008,0x40000000);

	conf = PSM << 31 | TWB << 23 | TWHR << 17 
								 | TWP << 11 | TRP << 5 | TWH;

	printf("Init Nand\n");
	printf("Set Conf[0x%x]\n", conf);
	ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLCONF, conf);
	
	ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLCOMM, 0xFF);
	ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLCTRL, ctrl);
	ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLADDR0L, 0x00);
	ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLADDR0H, 0x00);
	ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLCOMM, 0x90);
	ret = ejtag_read(HE3250_NAND_BASE|HE3250_NAND_FLDATA);
	maf_id = ret & 0xff;
	dev_id =(ret >> 8) & 0xff;
	id_4th = (ret >> 24) & 0xff;

	flash.dev = get_nand_flash_dev_by_id(dev_id);
	if (flash.dev == NULL) 
	{
		printf("NO NAND flash found\n");
		return -1;
	}
	flash.manf = get_nand_manuf_by_id(maf_id);
	if (flash.manf == NULL) 
	{
		printf("unknow nand flash manufacture");
		return -1;
	}

	flash.chip_size = flash.dev->chipsize << 20;

	flash.page_size = (1 << (id_4th&0x03)) << 10;
	flash.page_shift = ffbs(flash.page_size) - 1;
	flash.page_mask = (flash.chip_size >> flash.page_shift) - 1;
	id_4th >>= 2;
	flash.oob_size = (8 << (id_4th & 0x01)) * (flash.page_size >> 9);//:bytes/512bytes
	id_4th >>= 2;
	flash.block_size = (64 << (id_4th&0x03)) << 10;
	flash.block_shift = ffbs(flash.block_size) - 1;

	ctrl=ejtag_read(HE3250_NAND_BASE|HE3250_NAND_FLCTRL);
	ctrl |= 0x1; //5 addr cycles
	if(flash.page_size == 0x800)
		ctrl |= (1<<11); //2k
	if(flash.block_size/flash.page_size==128)
		ctrl |= (1<<12); //128pages/block
	ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLCTRL, ctrl);

	printf("ManfName :%s;	DevName  :%s\n", flash.manf->name, flash.dev->name);
	printf("ManfId   :0x%x;		DevId    :0x%x\n", flash.manf->id, flash.dev->id);
	printf("PageSize :0x%x;	PageShift:0x%x;		PageMask   :0x%x\n", 
								flash.page_size, flash.page_shift, flash.page_mask);
	printf("OobSize  :0x%x;		BlockSize:0x%x;	EraseShift :0x%x\n", flash.oob_size, flash.block_size, flash.block_shift);

	return 0;
}

/*----------------------------------------------------------------------------*/
s32 nand_dma_write(u32 memaddr, u32 offset, u32 size)
{
	u32 destaddr = 0;
	u32 i,state;
    u32 tempsize, write_bytes = 0;
	u32 pagecnt;
	u32 ret = SUCCESS;

	if (!flash.chip_size)
	{
		ret = nand_init();
		if (SUCCESS != ret)
		{
			printf("init nand flash failed\n");
			return INIT_NAND_FAILED;
		}
	}
	
	if ((offset & (flash.page_size - 1)) != 0) 
	{
		printf("write non page aligned data\n");
		return WRITE_NAND_NO_ALIGNED;
	}
	if((offset + size) > (u32)flash.chip_size)
	{
		 printf("Attempt to write outside the flash area\n");
		 return WRITE_NAND_ADDR_FAILED;
	}

	printf("\nWrite Nand\n");
	printf("Address :0x%08x;	Offset :0x%08x;	Size :%d\n",
									destaddr, offset, size);

	memaddr = memaddr - 0xa0000000;
	destaddr = ((offset >> flash.page_shift) & flash.page_mask) 
										<< (flash.page_shift+1);

	printf("Starting\n");
	while (write_bytes < size)
	{

		if ((size - write_bytes) >= (u32)flash.block_size)
			tempsize = flash.block_size;
		else
	        tempsize = size - write_bytes;

		ret = nand_erase(offset);
		if (ret)
			return NAND_ERASE_FAILED;

		printf("Write From MemAddr[%#x] To NandAddr[%#x]\n", memaddr,destaddr);
		pagecnt = tempsize / flash.page_size;
		pagecnt = (tempsize % flash.page_size) ? (pagecnt + 1) : pagecnt;
		
		for(i = 0; i < pagecnt; i++)
		{
			ejtag_write(0xbc045380, memaddr);
			ejtag_write(0xbc045388, (flash.page_size>>2)<<16);
			ejtag_write(0xbc045384, 0x0100d000);
			do {
				state = ejtag_read(0xbc04531c);
			} while(state&0x80);
			ejtag_write(0xbc045310, destaddr);
			ejtag_write(0xbc045308, 0x0080);	
			do {
				state=ejtag_read(0xbc04531c);
			} while(!(state&0x03));

			memaddr += flash.page_size;
			destaddr += (1<<(flash.page_shift+1));
		}
		offset += tempsize;
		write_bytes += tempsize;
	}	
	printf("Nand Write Success!\n\n");
	
	return SUCCESS;
}

/*----------------------------------------------------------------------------*/
int nand_dma_read( unsigned long memaddr,unsigned long offset,int size)
{

	unsigned long destaddr;
	int i,state;
    unsigned int tempsize, read_bytes = 0;
	int pagecnt;

	if(!flash.chip_size)
		nand_init();
	if ((offset & (flash.page_size - 1)) != 0) 
	{
		printf("Attempt to write non page aligned data\n");
		return -1;
	}
	if( offset > (unsigned long)flash.chip_size 
					|| (offset+size) > (unsigned long)flash.chip_size)
	{
		 printf("Attempt to write outside the flash area\n");
		 return -1;
	}

	memaddr = memaddr - 0xa0000000;
	destaddr = ((offset >> flash.page_shift) & flash.page_mask) 
											<< (flash.page_shift + 1);
	printf("Nand Read, address[%lx] ,offset[0x%08lx], size[%d(0x%x)]\n",
												destaddr, offset, size, size);
	while (read_bytes < (unsigned int)size)
	{
		if ((size-read_bytes) >= (unsigned int)flash.block_size)
		{
			tempsize=flash.block_size;
		}
       	else
        	tempsize=size-read_bytes;

		printf("mem=%lx;dest=%lx\n",memaddr,destaddr);
		pagecnt = tempsize/flash.page_size;
		
		pagecnt = (tempsize%flash.page_size)?(pagecnt+1):pagecnt;
		for (i = 0; i < pagecnt; i++)
		{
			ejtag_write(0xbc04530c,destaddr);
			ejtag_write(0xbc045308,0x0030);	
			do{
				state=ejtag_read(0xbc04531c);
			}while(!(state&0x03));

			ejtag_write(0xbc045380,memaddr);
			ejtag_write(0xbc045388,(flash.page_size>>2)<<16);
			ejtag_write(0xbc045384,0x0101d000);
			do{
				state=ejtag_read(0xbc04531c);
			}while(state&0x80);

			memaddr+= flash.page_size;
			destaddr+= (1<<(flash.page_shift+1));
		}
		offset += tempsize;
		read_bytes += tempsize;
	}	
	printf("read ok!\n");
	
	return 0;
}


/*----------------------------------------------------------------------------*/
int nand_read_reg(unsigned long memaddr, unsigned long offset, unsigned int len)
{
	unsigned int ret;
	unsigned int i,j;
	unsigned int size = 0,tsize=len,subsize;
	int page_num = size/flash.page_size;
	unsigned long addr;

    for (j = 0; j < len; j += size)
	{
    	//addr=offset;//((offset>>flash.page_shift)&flash.page_mask)<<(flash.page_shift+1)|(offset&(~(flash.page_mask<<flash.page_shift)));
		addr = ((offset >> flash.page_shift) & flash.page_mask)
						<< (flash.page_shift + 1) | (offset % flash.page_size);

		subsize = flash.page_size - (addr%flash.page_size);
		size = subsize>tsize?tsize:subsize;

		printf("\naddressing %lx ,nf offset 0x%08lx page(%ld), dump size %x bytes:\n",addr ,offset,(offset>>flash.page_shift)&flash.page_mask,size);
	
//page read       
        ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLADDR0L, addr&0xffffffff);
        //ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLADDR0H, (addr>>32)&0x3F);
		ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLADDR0H, 0);
        ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLCOMM, 0x0130);

        for (i = 0; i < size; i+=4) 
		{
	        ret = ejtag_read(HE3250_NAND_BASE|HE3250_NAND_FLDATA);
	        printf("%08x  ", ret);
	        if(!(i&0x2f))
                printf("\n");
        }

        printf("\nOOB:\n");
        for (i = 0; i < (unsigned int)flash.oob_size; i += 4) 
		{
            ret = ejtag_read(HE3250_NAND_BASE|HE3250_NAND_FLDATA);
            printf("%08x  ",ret);
            if(!(i&0x2f))
                printf("\n");
        }
        offset +=size;
		tsize -=size;
        printf("\n");
    }
		
    return 0;
}

/*----------------------------------------------------------------------------*/
int nand_read(unsigned long memaddr ,unsigned  long offset,unsigned int size)
{
	if(!flash.chip_size)
	        nand_init();

	if(offset > (unsigned long)flash.chip_size)
	{
         printf("Attempt to read outside the flash area\n");
         return -1;
	}

	nand_read_reg(memaddr,offset,size);

	return 0;
}

/*----------------------------------------------------------------------------*/
int nand_write_reg( unsigned long memaddr, unsigned long offset,unsigned int size)
{
	unsigned int  i;
	unsigned int tempsize, write_bytes = 0;
	int block = 0;

	int state = 1;
	unsigned long addr;

	while (write_bytes < size) 
	{
		addr = ((offset >> flash.page_shift) & flash.page_mask)
						<<(flash.page_shift + 1) | (offset&(flash.page_size - 1));

		//addr=offset;//((offset>>flash.page_shift)&flash.page_mask)<<(flash.page_shift+1)|(offset&(~flash.page_mask));
        if((size - write_bytes) >= (unsigned int)flash.page_size)
		{
	        tempsize = flash.page_size;
        }
        else
            tempsize = size-write_bytes;

		printf("begin write nandflash,addressing %lx ,offset:0x%08lx,size:%d(0x%x)bytes\n",addr,offset,tempsize,tempsize);
		///page program
		ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLADDR1L, addr&0xffffffff);
		//ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLADDR1H, (addr>>32)&0x3F);
		ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLADDR1H, 0);
		ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLCOMM, 0x0180);

        for (i = 0; i < tempsize; i += 4, memaddr += 4) 
		{
            ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLDATA, ejtag_read(memaddr));
        }


        ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLCOMM, 0x0010);
        do {
            ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLCOMM, 0x0070);
            state = ejtag_read(HE3250_NAND_BASE|HE3250_NAND_FLDATA);
        }while (!(state & 0x40));

        ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLCOMM, 0x0070);
        state = ejtag_read(HE3250_NAND_BASE|HE3250_NAND_FLDATA);
        if (state & 0x01)
        {
            printf("write error!\n");
            return -1;
        }
        write_bytes += tempsize;
        offset += tempsize;

	}
	printf("\n");
	
	return 0;
}

/*----------------------------------------------------------------------------*/
int nand_write( unsigned long memaddr, unsigned long offset,unsigned int size)
{

	if(!flash.chip_size)
        nand_init();
	
	if ((offset & (flash.page_size - 1)) != 0) 
	{
		printf("Attempt to write non page aligned data\n");
		return -1;
	}
	
	if(offset > (unsigned long)flash.chip_size 
				|| (offset+size) > (unsigned long)flash.chip_size)
	{
		printf("Attempt to write outside the flash area\n");
		return -1;
	}


	nand_write_reg(memaddr, offset, size);

	return 0;
}



/*----------------------------------------------------------------------------*/
int nand_erase(unsigned long offset)
{
	int state = 1;
	unsigned long addr;
	
	if(!flash.chip_size)
		nand_init();
	
	if( offset > (unsigned long)flash.chip_size)
	{
		printf("Attempt to erase  outside the flash area\n");
		return -1;
	}

	addr=((offset >> flash.page_shift) & flash.page_mask)
				  <<(flash.page_shift + 1) | (offset & (flash.page_size - 1));
	//addr=offset;//((offset>>flash.page_shift)&flash.page_mask)<<(flash.page_shift+1);

	ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLADDR1L, addr&0xffffffff);
	//ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLADDR1H, (addr>>32)&0x3f);
	ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLADDR1H, 0);
	ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLCOMM, 0x0060);

	state = ejtag_read(HE3250_NAND_BASE|HE3250_NAND_FLSTATE);
	if (state & 0x08)
	{
		printf("error:address 0x%lx,where the block is write-protected!\n",offset);
		return -1;
	}
	else if (state & 0x04)
	{
		printf("acess error!\n");
		return -1;
	}
	else{
		do {
			state = ejtag_read(HE3250_NAND_BASE|HE3250_NAND_FLSTATE);
		}while (!(state & 0x03));
	}
	
	ejtag_write(HE3250_NAND_BASE|HE3250_NAND_FLCOMM, 0x0070);
	state = ejtag_read(HE3250_NAND_BASE|HE3250_NAND_FLDATA);
	if (state & 0x01)
	{

		printf("state=0x%x,erase error!\n", state);
		return -1;
	}
	else
		printf("Erase Offset 0x%lx 		OK!\n",offset);
	
	return 0;
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/


