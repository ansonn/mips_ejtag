/*----------------------------------------------------------------------------*/
/*
** EJTAG Command
**
** Date 	: 2012-10-12
** NOTES 	: Create By wangshuke<anson.wang@gmail.com>
**----------------------------------------------------------------------------*/





/*----------------------------------------------------------------------------*/
#ifndef __COMMAND_H__
#define __COMMAND_H__
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <readline/readline.h>
#include "command_def.h"


extern u32 jtag_break_valid;
/*----------------------------------------------------------------------------*/
#define EJTAG_INIT_WAIT_PENDING_LOAD() \
	do {\
		u32 ret = ejtag_init_wait_pending();\
		jtag_break_valid = 0;\
	  	if ((0 == ret) || ((~(ret & PRNW)) && (ret & PRACC))) {\
			jtag_break_valid = 1;\
	    	break;\
	  	}\
		ejtag_print(FOR_EJTAG, "Active PRNW Mode Failed!\n");\
	} while (0)

/*----------------------------------------------------------------------------*/
#define EJTAG_INIT_WAIT_PENDING_STORE() \
	do {\
		jtag_break_valid = 0;\
	  	if (0 != (ejtag_init_wait_pending() & PRNW)) {\
			jtag_break_valid = 1;\
	    	break;\
	  	}\
		ejtag_print(FOR_EJTAG, "Active PRNW Mode Failed!\n");\
	} while (0)

/*----------------------------------------------------------------------------*/
typedef struct 
{
  char			*name;			/* User printable name of the function. */
  rl_icppfunc_t	*func;			/* Function to call to do the job. */
  const char	*format;		/* Documentation for this function.  */
  const char	*usage;			/* description for how to use this command */
  int			repeatable;		/* the later command can be invoked by "RETURN" */
  int 			alias;			/* Alias command node should NOT be display in help */
  
} COMMAND;

/*----------------------------------------------------------------------------*/
extern int execute_line(char *line);
extern COMMAND *find_command_from_line(char *line);
extern char *strip_white(char *string);
/*----------------------------------------------------------------------------*/
#define EJTAG_COMMAND_SPECIAL_CHAR_NEG 	'!'
#define EJTAT_COMMAND_HISTORY 			"./.ejtag_history"


#endif






