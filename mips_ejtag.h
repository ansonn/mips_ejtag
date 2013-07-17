/*----------------------------------------------------------------------------*/
/*
** MIPS EJTAG regs define
**
** Date 	: 2012-10-12
** NOTES 	: Create By wangshuke<anson.wang@gmail.com>
**----------------------------------------------------------------------------*/

#ifndef _MIPS_EJTAG_REGS_H
#define _MIPS_EJTAG_REGS_H


#define EJTAG_BASE 	0xFF200000
#define DSU_BASE 	0xFF300000

/*----------------------------------------------------------------------------*/
#define ZERO_REGNUM 			0	/* read-only register, always 0 */
#define V0_REGNUM 				2	/* Function integer return value */
#define A0_REGNUM 				4	/* Loc of first arg during a subr call */
#if MIPS_EABI
#define MIPS_LAST_ARG_REGNUM 	11	/* EABI uses R4 through R11 for args */
#define MIPS_NUM_ARG_REGS 		8
#else
#define MIPS_LAST_ARG_REGNUM 	7	/* old ABI uses R4 through R7 for args */
#define MIPS_NUM_ARG_REGS 		4
#endif
#define T9_REGNUM 25		/* Contains address of callee in PIC */
#define SP_REGNUM 29		/* Contains address of top of stack */
#define RA_REGNUM 31		/* Contains return address value */
#define PS_REGNUM 32		/* Contains processor status */
#define HI_REGNUM 34		/* Multiple/divide temp */
#define LO_REGNUM 33		/* ... */
#define BADVADDR_REGNUM 35	/* bad vaddr for addressing exception */
#define CAUSE_REGNUM 36		/* describes last exception */
#define PC_REGNUM 37		/* Contains program counter */
#define FP0_REGNUM 38		/* Floating point register 0 (single float) */
#define EBASE_REGNUM 39
#define FPA0_REGNUM (FP0_REGNUM+12)	/* First float argument register */
#if MIPS_EABI			/* EABI uses F12 through F19 for args */
#define MIPS_LAST_FP_ARG_REGNUM (FP0_REGNUM+19)
#define MIPS_NUM_FP_ARG_REGS 8
#else /* old ABI uses F12 through F15 for args */
#define MIPS_LAST_FP_ARG_REGNUM (FP0_REGNUM+15)
#define MIPS_NUM_FP_ARG_REGS 4
#endif
#define FCRCS_REGNUM 70		/* FP control/status */
#define FCRIR_REGNUM 71		/* FP implementation/revision */
#define FP_REGNUM 72		/* Pseudo register that contains true address of executing stack frame */
#define	UNUSED_REGNUM 73	/* Never used, FIXME */
#define	FIRST_EMBED_REGNUM 74	/* First CP0 register for embedded use */
#define	PRID_REGNUM 89		/* Processor ID */
#define	LAST_EMBED_REGNUM 105	/* Last one */



#define ZERO	0	/* WIRED ZERO */
#define AT	1	/* ASSEMBLER TEMP */
#define V0	2	/* RETURN VALUE */
#define V1	3
#define A0	4	/* ARGUMENT REGISTERS */
#define A1	5
#define A2	6
#define A3	7
#define T0	8	/* CALLER SAVED */
#define T1	9
#define T2	10
#define T3	11
#define T4	12
#define T5	13
#define T6	14
#define T7	15
#define S0	16	/* CALLEE SAVED */
#define S1	17
#define S2	18
#define S3	19
#define S4	20
#define S5	21
#define S6	22
#define S7	23
#define T8	24	/* CODE GENERATOR */
#define T9	25
#define K0	26	/* KERNEL TEMPORARY */
#define K1	27
#define GP	28	/* GLOBAL POINTER */
#define SP	29	/* STACK POINTER */
#define FP	30	/* FRAME POINTER */
#define RA	31	/* return address */

#define SR      32
#define LO      33
#define HI      34
#define BAD     35
#define CAUSE   36
#define PC      37

/*----------------------------------------------------------------------------*/
/* coprocessor 0 definitions */
#define CP0_INDEX      0       
#define CP0_RANDOM     1       
#define CP0_ENTRYLO0   2       
#define CP0_ENTRYLO1   3       
#define CP0_CONTEXT    4       
#define CP0_PAGEMASK   5       
#define CP0_WIRED      6       
#define CP0_RESERVED7  7       
#define CP0_BADVADDR   8
#define CP0_COUNT1     9       
#define CP0_ENTRYHI    10      
#define CP0_COMPARE1   11      
#define CP0_STATUS     12      
#define CP0_CAUSE      13      
#define CP0_EPC        14      
#define CP0_PRID       15      
#define CP0_EBASE	   15
#define CP0_CONFIG     16      
#define CP0_LLADDR     17
#define CP0_COUNT2     18      
#define CP0_COMPARE2   19      
#define CP0_COUNT3     20      
#define CP0_COMPARE3   21      
#define CP0_DESAVE_2	22      
#define CP0_DEBUG      23      
#define CP0_DEPC       24      
#define CP0_Reserved25 25      
#define CP0_Reserved26 26      
#define CP0_Reserved27 27      
#define CP0_Reserved28 28      
#define CP0_Reserved29 29      
#define CP0_Reserved30 30      
#define CP0_DESAVE     31     

/*------------------------------------------------------------------------*/
/*  CP0 DEBUG BITS       						  */
/*------------------------------------------------------------------------*/
#define CP0_DEBUG_SS_EXC (1<<0)             /* single step */
#define CP0_DEBUG_BP_EXC (1<<1)             /* break point */
#define CP0_DEBUG_DBL_EXC (1<<2)            /* data address break load */
#define CP0_DEBUG_DBS_EXC (1<<3)            /* data address break store */
#define CP0_DEBUG_DIB_EXC (1<<4)            /* instruction addr break store */
#define CP0_DEBUG_DINT_EXC (1<<5)           /* processor/bus break */
#define CP0_DEBUG_JTAGRST (1<<7)            /* reset dsu and ejtag */
#define CP0_DEBUG_SST_EN (1<<8)             /* single step enable */
#define CP0_DEBUG_EXC_CODE_SHIFT 10
#define CP0_DEBUG_EXC_CODE_MASK  (0x1f<<CP0_DEBUG_EXC_CODE_SHIFT)
#define CP0_DEBUG_BSSF (1<<10)              /* bus error exception */
#define CP0_DEBUG_TLF (1<<11)               /* tlb exception */
#define CP0_DEBUG_OES (1<<12)               /* other exception status */
#define CP0_DEBUG_UMS (1<<13)               /* UTLB miss exception status */
#define CP0_DEBUG_NIS (1<<14)               /* non maskable interrupt status */
#define CP0_DEBUG_DDBL (1<<18)              /* dbg data-break load imprec. */
#define CP0_DEBUG_DDBS (1<<19)              /* dbg data-break store imprec. */
#define CP0_DEBUG_IEXI (1<<20)              /* imprec. error exc inhibit */
#define CP0_DEBUG_DBEP (1<<21)              /* dbus error exception pending */
#define CP0_DEBUG_CAEP (1<<22)              /* cache error exc pending */
#define CP0_DEBUG_MCEP (1<<23)              /* mach. check exc pending */
#define CP0_DEBUG_IFEP (1<<24)              /* instr fetch error pending */
#define CP0_DEBUG_CNT_DM (1<<25)            /* debug mode count reg runs */
#define CP0_DEBUG_HALT (1<<26)              /* system bus stopped */
#define CP0_DEBUG_LSDM (1<<28)              /* load-store DSEG to memory */
#define CP0_DEBUG_DM (1<<30)                /* debug mode status */
#define CP0_DEBUG_DBD (1<<31)               /* debug branch delay */

/*----------------------------------------------------------------------------*/
/* control reg bits define */
#define JTAGBRK			0x00001000  //Initiated a JTAG break -> Debugex.
#define PROBTRAP		0x00004000	//Controls debug exception vector
#define PROBEN			0x0000c000  //Probe enabled/present
#define PRACC			0x00040000  //Processor access, reset by software!
#define PRNW			0x00080000  //Processor access read not, write
#define PRRST			0x00010000  //Processor reset


/*----------------------------------------------------------------------------*/
/* assemble
*/
#define JR(RS)  ((RS)<<21|0x8)
#define MTC0(RT, RD, SEL) ((0x408<<20)|((RT)<<16)|((RD)<<11)|(SEL))
#define MFC0(RT, RD, SEL) ((0x400<<20)|((RT)<<16)|((RD)<<11)|(SEL))
#define MFLO(RD) (((RD)<<11)|0x12)
#define MFHI(RD) (((RD)<<11)|0x10)
//#define MTLO(RD) (((RD)<<11)|0x13)
#define MTLO(RD) (((RD)<<21)|0x13)
//#define MTHI(RD) (((RD)<<11)|0x11)
#define MTHI(RD) (((RD)<<21)|0x11)
#define SW(RT,OFF,BASE) ((0x2B<<26)|((BASE)<<21)|((RT)<<16)|(OFF))
#define SH(RT,OFF,BASE) ((0x29<<26)|((BASE)<<21)|((RT)<<16)|(OFF))
#define SB(RT,OFF,BASE) ((0x28<<26) | ((BASE)<<21) | ((RT)<<16) | (OFF))
#define LW(RT,OFF,BASE) ((0x23<<26)|((BASE)<<21)|((RT)<<16)|(OFF))
#define LHU(RT,OFF,BASE) ((0x25<<26)|((BASE)<<21)|((RT)<<16)|(OFF))
#define LBU(RT,OFF,BASE) ((0x20<<26) | ((BASE)<<21) | ((RT)<<16) | (OFF))
#define LUI(RT,IMM) ((0x0F<<26)|((RT)<<16)|(IMM))
#define OR(RD,RS,RT) ((RS)<<21|(RT)<<16|(RD)<<11|0x25)
#define ORI(RT,RS,IMM) ((0x0D<<26)|((RS)<<21)|((RT)<<16)|(IMM))
#define ADDIU(RT,RS,IMM) ((0x09<<26)|((RS)<<21)|((RT)<<16)|(IMM))
#define ADDI(RT,RS,IMM) ((0x08<<26)|((RS)<<21)|((RT)<<16)|(IMM))
#define XOR(RD,RS,RT) ((RS)<<21|(RT)<<16|(RD)<<11|0x26)
#define XORI(RT,RS,IMM) ((0x38<<24)|((RS)<<21)|((RT)<<16)|(IMM))
#define SDBBP(CODE) (0x7000003F|(((CODE)&0xFFFFF)<<6))
#define MIPS_SYNC    0x0000000f 
#define DERET    0x4200001F 
#define ERET	0x42000018
#define NOP      0x00000000 
#define SSNOP    0x00000040
#define CACHE(OP,OFF,BASE) (0xBC<<24|(BASE)<<21|(OP)<<16|(OFF))
#define ICACHE 0x0
//#define DCACHE 0x1
#define DCACHE 0x15


/*----------------------------------------------------------------------------*/
//DDR Regs
#define HE3250_DDR2_BASEADDR           0xBC113000
#define HE3250_DDR2_STAT                    HE3250_DDR2_BASEADDR+0x00000008
#define HE3250_DDR2_INTRSTAT                HE3250_DDR2_BASEADDR+0x0000000c
#define HE3250_DDR2_SCTL                    HE3250_DDR2_BASEADDR+0x00000004
#define HE3250_DDR2_SCFG                    HE3250_DDR2_BASEADDR+0x00000000
#define HE3250_DDR2_POWSTAT                 HE3250_DDR2_BASEADDR+0x00000048
#define HE3250_DDR2_MRRSTAT0                HE3250_DDR2_BASEADDR+0x00000064
#define HE3250_DDR2_CMDTSTAT                HE3250_DDR2_BASEADDR+0x0000004c
#define HE3250_DDR2_MCMD                    HE3250_DDR2_BASEADDR+0x00000040
#define HE3250_DDR2_MRRSTAT1                HE3250_DDR2_BASEADDR+0x00000068
#define HE3250_DDR2_MRRCFG0                 HE3250_DDR2_BASEADDR+0x00000060
#define HE3250_DDR2_CMDTSTATEN              HE3250_DDR2_BASEADDR+0x00000050
#define HE3250_DDR2_POWCTL                  HE3250_DDR2_BASEADDR+0x00000044
#define HE3250_DDR2_LPDDR2ZQCFG             HE3250_DDR2_BASEADDR+0x0000008c
#define HE3250_DDR2_PPCFG                   HE3250_DDR2_BASEADDR+0x00000084
#define HE3250_DDR2_MCFG1                   HE3250_DDR2_BASEADDR+0x0000007c
#define HE3250_DDR2_MSTAT                   HE3250_DDR2_BASEADDR+0x00000088
#define HE3250_DDR2_MCFG                    HE3250_DDR2_BASEADDR+0x00000080
#define HE3250_DDR2_DTUAWDT                 HE3250_DDR2_BASEADDR+0x000000b0
#define HE3250_DDR2_DTUPRD2                 HE3250_DDR2_BASEADDR+0x000000a8
#define HE3250_DDR2_DTUPRD3                 HE3250_DDR2_BASEADDR+0x000000ac
#define HE3250_DDR2_DTUNE                   HE3250_DDR2_BASEADDR+0x0000009c
#define HE3250_DDR2_DTUPDES                 HE3250_DDR2_BASEADDR+0x00000094
#define HE3250_DDR2_DTUNA                   HE3250_DDR2_BASEADDR+0x00000098
#define HE3250_DDR2_DTUPRD0                 HE3250_DDR2_BASEADDR+0x000000a0
#define HE3250_DDR2_DTUPRD1                 HE3250_DDR2_BASEADDR+0x000000a4
#define HE3250_DDR2_TCKSRE                  HE3250_DDR2_BASEADDR+0x00000124
#define HE3250_DDR2_TZQCSI                  HE3250_DDR2_BASEADDR+0x0000011c
#define HE3250_DDR2_TINIT                   HE3250_DDR2_BASEADDR+0x000000c4
#define HE3250_DDR2_TDPD                    HE3250_DDR2_BASEADDR+0x00000144
#define HE3250_DDR2_TOGCNT1U                HE3250_DDR2_BASEADDR+0x000000c0
#define HE3250_DDR2_TCKE                    HE3250_DDR2_BASEADDR+0x0000012c
#define HE3250_DDR2_TMOD                    HE3250_DDR2_BASEADDR+0x00000130
#define HE3250_DDR2_TEXSR                   HE3250_DDR2_BASEADDR+0x0000010c
#define HE3250_DDR2_TAL                     HE3250_DDR2_BASEADDR+0x000000e4
#define HE3250_DDR2_TRTP                    HE3250_DDR2_BASEADDR+0x00000100
#define HE3250_DDR2_TCKSRX                  HE3250_DDR2_BASEADDR+0x00000128
#define HE3250_DDR2_TRTW                    HE3250_DDR2_BASEADDR+0x000000e0
#define HE3250_DDR2_TCWL                    HE3250_DDR2_BASEADDR+0x000000ec
#define HE3250_DDR2_TWR                     HE3250_DDR2_BASEADDR+0x00000104
#define HE3250_DDR2_TCL                     HE3250_DDR2_BASEADDR+0x000000e8
#define HE3250_DDR2_TDQS                    HE3250_DDR2_BASEADDR+0x00000120
#define HE3250_DDR2_TRSTH                   HE3250_DDR2_BASEADDR+0x000000c8
#define HE3250_DDR2_TRCD                    HE3250_DDR2_BASEADDR+0x000000f8
#define HE3250_DDR2_TXP                     HE3250_DDR2_BASEADDR+0x00000110
#define HE3250_DDR2_TOGCNT100N              HE3250_DDR2_BASEADDR+0x000000cc
#define HE3250_DDR2_TMRD                    HE3250_DDR2_BASEADDR+0x000000d4
#define HE3250_DDR2_TRSTL                   HE3250_DDR2_BASEADDR+0x00000134
#define HE3250_DDR2_TREFI                   HE3250_DDR2_BASEADDR+0x000000d0
#define HE3250_DDR2_TRAS                    HE3250_DDR2_BASEADDR+0x000000f0
#define HE3250_DDR2_TWTR                    HE3250_DDR2_BASEADDR+0x00000108
#define HE3250_DDR2_TRC                     HE3250_DDR2_BASEADDR+0x000000f4
#define HE3250_DDR2_TRFC                    HE3250_DDR2_BASEADDR+0x000000d8
#define HE3250_DDR2_TMRR                    HE3250_DDR2_BASEADDR+0x0000013c
#define HE3250_DDR2_TCKESR                  HE3250_DDR2_BASEADDR+0x00000140
#define HE3250_DDR2_TZQCL                   HE3250_DDR2_BASEADDR+0x00000138
#define HE3250_DDR2_TRRD                    HE3250_DDR2_BASEADDR+0x000000fc
#define HE3250_DDR2_TRP                     HE3250_DDR2_BASEADDR+0x000000dc
#define HE3250_DDR2_TZQCS                   HE3250_DDR2_BASEADDR+0x00000118
#define HE3250_DDR2_TXPDLL                  HE3250_DDR2_BASEADDR+0x00000114
#define HE3250_DDR2_ECCCFG                  HE3250_DDR2_BASEADDR+0x00000180
#define HE3250_DDR2_ECCLOG                  HE3250_DDR2_BASEADDR+0x0000018c
#define HE3250_DDR2_ECCCLR                  HE3250_DDR2_BASEADDR+0x00000188
#define HE3250_DDR2_ECCTST                  HE3250_DDR2_BASEADDR+0x00000184
#define HE3250_DDR2_DTUWD0                  HE3250_DDR2_BASEADDR+0x00000210
#define HE3250_DDR2_DTUWD1                  HE3250_DDR2_BASEADDR+0x00000214
#define HE3250_DDR2_DTUWACTL                HE3250_DDR2_BASEADDR+0x00000200
#define HE3250_DDR2_DTULFSRRD               HE3250_DDR2_BASEADDR+0x00000238
#define HE3250_DDR2_DTUWD2                  HE3250_DDR2_BASEADDR+0x00000218
#define HE3250_DDR2_DTUWD3                  HE3250_DDR2_BASEADDR+0x0000021c
#define HE3250_DDR2_DTULFSRWD               HE3250_DDR2_BASEADDR+0x00000234
#define HE3250_DDR2_DTURACTL                HE3250_DDR2_BASEADDR+0x00000204
#define HE3250_DDR2_DTUWDM                  HE3250_DDR2_BASEADDR+0x00000220
#define HE3250_DDR2_DTURD0                  HE3250_DDR2_BASEADDR+0x00000224
#define HE3250_DDR2_DTURD1                  HE3250_DDR2_BASEADDR+0x00000228
#define HE3250_DDR2_DTURD2                  HE3250_DDR2_BASEADDR+0x0000022c
#define HE3250_DDR2_DTURD3                  HE3250_DDR2_BASEADDR+0x00000230
#define HE3250_DDR2_DTUCFG                  HE3250_DDR2_BASEADDR+0x00000208
#define HE3250_DDR2_DTUEAF                  HE3250_DDR2_BASEADDR+0x0000023c
#define HE3250_DDR2_DTUECTL                 HE3250_DDR2_BASEADDR+0x0000020c
#define HE3250_DDR2_DFIODTCFG1              HE3250_DDR2_BASEADDR+0x00000248
#define HE3250_DDR2_DFITCTRLDELAY           HE3250_DDR2_BASEADDR+0x00000240
#define HE3250_DDR2_DFIODTRANKMAP           HE3250_DDR2_BASEADDR+0x0000024c
#define HE3250_DDR2_DFIODTCFG               HE3250_DDR2_BASEADDR+0x00000244
#define HE3250_DDR2_DFITPHYWRLAT            HE3250_DDR2_BASEADDR+0x00000254
#define HE3250_DDR2_DFITPHYWRDATA           HE3250_DDR2_BASEADDR+0x00000250
#define HE3250_DDR2_DFITRDDATAEN            HE3250_DDR2_BASEADDR+0x00000260
#define HE3250_DDR2_DFITPHYRDLAT            HE3250_DDR2_BASEADDR+0x00000264
#define HE3250_DDR2_DFITREFMSKI             HE3250_DDR2_BASEADDR+0x00000294
#define HE3250_DDR2_DFITPHYUPDTYPE0         HE3250_DDR2_BASEADDR+0x00000270
#define HE3250_DDR2_DFITPHYUPDTYPE1         HE3250_DDR2_BASEADDR+0x00000274
#define HE3250_DDR2_DFITCTRLUPDDLY          HE3250_DDR2_BASEADDR+0x00000288
#define HE3250_DDR2_DFITPHYUPDTYPE2         HE3250_DDR2_BASEADDR+0x00000278
#define HE3250_DDR2_DFITCTRLUPDMIN          HE3250_DDR2_BASEADDR+0x00000280
#define HE3250_DDR2_DFITPHYUPDTYPE3         HE3250_DDR2_BASEADDR+0x0000027c
#define HE3250_DDR2_DFIUPDCFG               HE3250_DDR2_BASEADDR+0x00000290
#define HE3250_DDR2_DFITCTRLUPDMAX          HE3250_DDR2_BASEADDR+0x00000284
#define HE3250_DDR2_DFITCTRLUPDI            HE3250_DDR2_BASEADDR+0x00000298
#define HE3250_DDR2_DFITRRDLVLEN            HE3250_DDR2_BASEADDR+0x000002b8
#define HE3250_DDR2_DFITRSTAT0              HE3250_DDR2_BASEADDR+0x000002b0
#define HE3250_DDR2_DFITRWRLVLEN            HE3250_DDR2_BASEADDR+0x000002b4
#define HE3250_DDR2_DFITRCFG0               HE3250_DDR2_BASEADDR+0x000002ac
#define HE3250_DDR2_DFITRRDLVLGATEEN        HE3250_DDR2_BASEADDR+0x000002bc
#define HE3250_DDR2_DFISTSTAT0              HE3250_DDR2_BASEADDR+0x000002c0
#define HE3250_DDR2_DFISTPARLOG             HE3250_DDR2_BASEADDR+0x000002e0
#define HE3250_DDR2_DFITDRAMCLKEN           HE3250_DDR2_BASEADDR+0x000002d0
#define HE3250_DDR2_DFISTPARCLR             HE3250_DDR2_BASEADDR+0x000002dc
#define HE3250_DDR2_DFISTCFG0               HE3250_DDR2_BASEADDR+0x000002c4
#define HE3250_DDR2_DFISTCFG1               HE3250_DDR2_BASEADDR+0x000002c8
#define HE3250_DDR2_DFISTCFG2               HE3250_DDR2_BASEADDR+0x000002d8
#define HE3250_DDR2_DFITDRAMCLKDIS          HE3250_DDR2_BASEADDR+0x000002d4
#define HE3250_DDR2_DFILPCFG0               HE3250_DDR2_BASEADDR+0x000002f0
#define HE3250_DDR2_DFITRWRLVLDELAY0        HE3250_DDR2_BASEADDR+0x00000318
#define HE3250_DDR2_DFITRWRLVLDELAY1        HE3250_DDR2_BASEADDR+0x0000031c
#define HE3250_DDR2_DFITRWRLVLDELAY2        HE3250_DDR2_BASEADDR+0x00000320
#define HE3250_DDR2_DFITRRDLVLRESP0         HE3250_DDR2_BASEADDR+0x0000030c
#define HE3250_DDR2_DFITRRDLVLRESP1         HE3250_DDR2_BASEADDR+0x00000310
#define HE3250_DDR2_DFITRRDLVLRESP2         HE3250_DDR2_BASEADDR+0x00000314
#define HE3250_DDR2_DFITRWRLVLRESP0         HE3250_DDR2_BASEADDR+0x00000300
#define HE3250_DDR2_DFITRRDLVLDELAY0        HE3250_DDR2_BASEADDR+0x00000324
#define HE3250_DDR2_DFITRRDLVLDELAY1        HE3250_DDR2_BASEADDR+0x00000328
#define HE3250_DDR2_DFITRWRLVLRESP1         HE3250_DDR2_BASEADDR+0x00000304
#define HE3250_DDR2_DFITRRDLVLDELAY2        HE3250_DDR2_BASEADDR+0x0000032c
#define HE3250_DDR2_DFITRWRLVLRESP2         HE3250_DDR2_BASEADDR+0x00000308
#define HE3250_DDR2_DFITRRDLVLGATEDELAY0    HE3250_DDR2_BASEADDR+0x00000330
#define HE3250_DDR2_DFITRCMD                HE3250_DDR2_BASEADDR+0x0000033c
#define HE3250_DDR2_DFITRRDLVLGATEDELAY1    HE3250_DDR2_BASEADDR+0x00000334
#define HE3250_DDR2_DFITRRDLVLGATEDELAY2    HE3250_DDR2_BASEADDR+0x00000338
#define HE3250_DDR2_IPTR                    HE3250_DDR2_BASEADDR+0x000003fc
#define HE3250_DDR2_IPVR                    HE3250_DDR2_BASEADDR+0x000003f8
#define HE3250_DDR2_CSTAT                   HE3250_DDR2_BASEADDR+0x00000488
#define HE3250_DDR2_PCFG_0                  HE3250_DDR2_BASEADDR+0x00000400
#define HE3250_DDR2_PCFG_1                  HE3250_DDR2_BASEADDR+0x00000404
#define HE3250_DDR2_CCFG                    HE3250_DDR2_BASEADDR+0x00000480
#define HE3250_DDR2_PCFG_2                  HE3250_DDR2_BASEADDR+0x00000408
#define HE3250_DDR2_PCFG_3                  HE3250_DDR2_BASEADDR+0x0000040c
#define HE3250_DDR2_PCFG_4                  HE3250_DDR2_BASEADDR+0x00000410
#define HE3250_DDR2_PCFG_5                  HE3250_DDR2_BASEADDR+0x00000414
#define HE3250_DDR2_PCFG_6                  HE3250_DDR2_BASEADDR+0x00000418
#define HE3250_DDR2_PCFG_10                 HE3250_DDR2_BASEADDR+0x00000428
#define HE3250_DDR2_PCFG_7                  HE3250_DDR2_BASEADDR+0x0000041c
#define HE3250_DDR2_PCFG_11                 HE3250_DDR2_BASEADDR+0x0000042c
#define HE3250_DDR2_PCFG_8                  HE3250_DDR2_BASEADDR+0x00000420
#define HE3250_DDR2_PCFG_12                 HE3250_DDR2_BASEADDR+0x00000430
#define HE3250_DDR2_PCFG_9                  HE3250_DDR2_BASEADDR+0x00000424
#define HE3250_DDR2_PCFG_13                 HE3250_DDR2_BASEADDR+0x00000434
#define HE3250_DDR2_PCFG_14                 HE3250_DDR2_BASEADDR+0x00000438
#define HE3250_DDR2_PCFG_15                 HE3250_DDR2_BASEADDR+0x0000043c
#define HE3250_DDR2_PCFG_20                 HE3250_DDR2_BASEADDR+0x00000450
#define HE3250_DDR2_CCFG1                   HE3250_DDR2_BASEADDR+0x0000048c
#define HE3250_DDR2_PCFG_16                 HE3250_DDR2_BASEADDR+0x00000440
#define HE3250_DDR2_PCFG_21                 HE3250_DDR2_BASEADDR+0x00000454
#define HE3250_DDR2_PCFG_17                 HE3250_DDR2_BASEADDR+0x00000444
#define HE3250_DDR2_PCFG_22                 HE3250_DDR2_BASEADDR+0x00000458
#define HE3250_DDR2_PCFG_23                 HE3250_DDR2_BASEADDR+0x0000045c
#define HE3250_DDR2_PCFG_18                 HE3250_DDR2_BASEADDR+0x00000448
#define HE3250_DDR2_PCFG_24                 HE3250_DDR2_BASEADDR+0x00000460
#define HE3250_DDR2_PCFG_19                 HE3250_DDR2_BASEADDR+0x0000044c
#define HE3250_DDR2_PCFG_30                 HE3250_DDR2_BASEADDR+0x00000478
#define HE3250_DDR2_PCFG_25                 HE3250_DDR2_BASEADDR+0x00000464
#define HE3250_DDR2_PCFG_31                 HE3250_DDR2_BASEADDR+0x0000047c
#define HE3250_DDR2_PCFG_26                 HE3250_DDR2_BASEADDR+0x00000468
#define HE3250_DDR2_PCFG_27                 HE3250_DDR2_BASEADDR+0x0000046c
#define HE3250_DDR2_PCFG_28                 HE3250_DDR2_BASEADDR+0x00000470
#define HE3250_DDR2_PCFG_29                 HE3250_DDR2_BASEADDR+0x00000474
#define HE3250_DDR2_DCFG                    HE3250_DDR2_BASEADDR+0x00000484

#define HE3250_DDR2_PHY_BASEADDR       0xBC116000


/*----------------------------------------------------------------------------*/
extern u32 ejtag_get_reg(u32 reg_num);
extern void show_mips_regs(void);
extern void ejtag_release(void);
extern void ejtag_set_reg(u32 reg_idx, u32 reg_val);
extern void ejtag_mem_display(u32 start_addr, u32 count, u32 data_size);
extern void ejtag_mem_read(u32 start_addr, u32 count, u32 data_size);
extern void ejtag_mem_write(u32 addr, u32 data, u32 data_size);
extern void ejtag_go_address(u32 addr);
extern void ejtag_del_bp(u32 addr);
extern void ejtag_add_bp(u32 addr);
extern void ejtag_init_step_mode(MIPS_STEP_MODE step_mode);
extern void ejtag_fill_mem(u32 start_addr, s32 val, u32 count);
extern void ejtag_test(void);
extern void ejtag_boot(void);
extern void ejtag_ddr_init(void);
extern void ejtag_sigint_exit(s32 signo);
extern void ejtag_show_cp0_debug(void);


/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
extern u32 gdb_get_reg(u32 reg_idx);
extern u32 gdb_read_mem(u32 address);
extern void gdb_set_reg(u32 reg_idx, u32 reg_val);
extern void gdb_release(void);
extern void gdb_add_break_point(u32 addr);
extern void gdb_del_break_point(u32 addr);
extern BREAK_POINT *gdb_find_break_point(int addr);
extern u32 gdb_get_ejtag_ctrl(void);
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/



#endif



