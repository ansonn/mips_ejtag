/*----------------------------------------------------------------------------*/
/*
** MIPS EJTAG main
**
** Date 	: 2012-10-12
** NOTES 	: Create By wangshuke<anson.wang@gmail.com>
**----------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <pthread.h>

#include "command_def.h"
#include "command.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
extern u32 gdb_port;
/*----------------------------------------------------------------------------*/
extern void gdb_del_all_bp(void);
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
extern s32 ejtag_init();
extern void init_readline();
extern void ejtag_usb_exit(void);
extern s32 cmd_continue(char *argv[]);
extern void gdb_server_init(void);
extern void *start_gdb_server(void *argv);


/*----------------------------------------------------------------------------*/
void init_pthread_mutex(void)
{
	pthread_mutex_init(&gst_mips_ejtag.tap_mutex, NULL);
	pthread_mutex_init(&gst_mips_debug.list_mutex, NULL);
}

/*----------------------------------------------------------------------------*/
void destroy_pthread_mutex(void)
{
	pthread_mutex_destroy(&gst_mips_ejtag.tap_mutex);
	pthread_mutex_destroy(&gst_mips_debug.list_mutex);
}

/*----------------------------------------------------------------------------*/
void ejtag_exit(u32 exit_code)
{
	ejtag_print(FOR_ALL, "EJTAG exit_code[%d]\n", exit_code);
	exit(exit_code);
}

/*----------------------------------------------------------------------------*/
void ejtag_sigint_exit(s32 signo)
{
	if (CMD_RUN == gst_mips_ejtag.cmd_run_state)
	{
		ejtag_print(FOR_ALL, "\n\nHave Command Run Now, Dont Ctrl+C!" 
						" Wait Command Done\n\n");
		return;
	}
	else if (CMD_DDRINIT == gst_mips_ejtag.cmd_run_state)
	{
		ejtag_exit(0);
	}
	gdb_del_all_bp();
	cmd_continue(NULL);
	do_usb_cmd(EMU_CMD_HW_TRST0);
	usleep(1000);
	do_usb_cmd(EMU_CMD_HW_TRST1);
	usleep(1000);
	ejtag_usb_exit();
	destroy_pthread_mutex();
	ejtag_exit(0);
}

/*----------------------------------------------------------------------------*/
s32 ejtag_blx_download(s8 argc, s8 *argv[])
{
	s32 ret = SUCCESS;

	if (4 > argc)
	{
		ejtag_print(FOR_ECLIPSE, "blx_download param error\n");
		return LOAD_PARAM_ERR;
	}

	if (!strcmp("mem", argv[1]))
	{
		if (5 != argc)
		{
			ejtag_print(FOR_ECLIPSE, "blx_download mem param error\n");
			return LOAD_MEM_PARAM_ERR;
		}
	
		gst_mips_ejtag.cmd_run_state = CMD_RUN;
		ret = cmd_ubootload(argv + 1);
		gst_mips_ejtag.cmd_run_state = NO_CMD_RUN;
	}
	else if (!strcmp("nand", argv[1]))
	{
		gst_mips_ejtag.cmd_run_state = CMD_RUN;
		ret = cmd_nand(argv);
		gst_mips_ejtag.cmd_run_state = NO_CMD_RUN;
	}
	else if (!strcmp("nor", argv[1]))
	{
		ejtag_print(FOR_ECLIPSE, "blx_download nor dont support\n");
		gst_mips_ejtag.cmd_run_state = CMD_RUN;
		ret = OPT_NO_SUPPORT;
		gst_mips_ejtag.cmd_run_state = NO_CMD_RUN;
	}
	else
		ret = LOAD_PARAM_ERR;
	
	return ret;
}

/*----------------------------------------------------------------------------*/
s32 ejtag_blx_debug(s8 argc, s8 *argv[])
{
	u32 ret = 0;
	s8 *err_pos = NULL;

	if (2 != argc)
	{
		ejtag_print(FOR_ECLIPSE, "blx_debug param error\n");
		return DEBUG_PARAM_ERR;
	}
	gdb_port = strtoul(argv[1], &err_pos, 10);
	if (NULL == err_pos)
	{
		ejtag_print(FOR_ECLIPSE, "blx_debug gdb_port error");
		return DEBUG_PARAM_ERR;
	}
	ret = (u32)start_gdb_server(argv);

	return ret;
}

/*----------------------------------------------------------------------------*/
s32 ejtag_blx_command(s8 argc, s8 *argv[])
{
	s8 *line = NULL;
	s8 *s = NULL;
	HIST_ENTRY *hent = NULL;
    s32 history_offset;
    char exe_line[128] = { 0 };

	init_readline();
	while (1) 
	{
		line = readline("mips_ejtag: ");
		if (!line) 
		{
			/* ctrl + D */
			ejtag_release();
			ejtag_print(FOR_EJTAG, "\n");
			break;
		} 
		else if (line[0] == '\0') 
		{
			history_offset = where_history();
			hent = history_get(history_offset);
			if (hent) 
			{
				//ejtag_print(FOR_EJTAG, "\n hent->line = %s", hent->line);
				COMMAND *cmd = find_command_from_line(hent->line);
				if (cmd && cmd->repeatable) 
				{
					memset(exe_line, 0, sizeof(exe_line));
					strncpy(exe_line, hent->line, sizeof(exe_line));
					s = exe_line;
					goto REPEATCMD;
				}
			}
			else
			{
				//ejtag_print(FOR_EJTAG, "\n hent == NULL");
			}
		}

		/* remove both end whitespace */
		s = strip_white(line);
		
REPEATCMD:
		if (*s) 
		{
			add_history(s);
			execute_line(s);
		} 

		free(line);
	}
	
	return SUCCESS;
}

/*----------------------------------------------------------------------------*/
s32 ejtag_blx_gdb(s8 argc, s8 *argv[])
{
	s8 *err_pos = NULL;

	if (2 <= argc)
	{
		gdb_port = strtoul(argv[1], &err_pos, 10);
		if (NULL == err_pos)
		{
			ejtag_print(FOR_EJTAG, "blx_debug gdb_port error");
			return DEBUG_PARAM_ERR;
		}	
	}

	gdb_server_init();
	ejtag_blx_command(argc, argv);
	
	return SUCCESS;
}

/*----------------------------------------------------------------------------*/
void ejtag_usage(void)
{
	ejtag_print(FOR_ALL, "EJTAG Usage:\n");
	ejtag_print(FOR_ALL, "	./ejtag blx_download mem/nand/nor file address:\n");
	ejtag_print(FOR_ALL, "	./ejtag blx_debug gdb_port:\n");
	ejtag_print(FOR_ALL, "	./ejtag blx_gdb gdb_port:\n");
	ejtag_print(FOR_ALL, "	./ejtag blx_command:\n");
	ejtag_print(FOR_ALL, "Notes:\n");
	ejtag_print(FOR_ALL, "	blx_download\t: do download file\n");
	ejtag_print(FOR_ALL, "	blx_debug\t: start gdb server only\n");
	ejtag_print(FOR_ALL, "	blx_gdb\t\t: start gdb server and command line\n");
	ejtag_print(FOR_ALL, "	blx_command\t: start command func only\n");
	ejtag_print(FOR_ALL, "\n");

}

/*----------------------------------------------------------------------------*/
s32 main(s8 argc, s8 *argv[])
{
	s32 ret = 0;

	ejtag_print(FOR_ALL, "Welcom MIPS EJTAG!\n");
	init_pthread_mutex();
	ret = ejtag_init();
	if (ret) 
		ejtag_exit(ret);
	signal(SIGINT, ejtag_sigint_exit);

	if (2 > argc)
	{
		gst_mips_debug.run_mode = EJTAG_MODE;
		ejtag_print(FOR_EJTAG, "\nblx_command! enter ejtag mode\n");
		ejtag_print(FOR_EJTAG, "	U can use command only\n");
		ret = ejtag_blx_command(argc - 1, argv + 1);
		ejtag_exit(ret);
	}

	if (!strcmp("blx_download", argv[1]))
	{
		gst_mips_debug.run_mode = ECLIPSE_MODE;
		ejtag_print(FOR_ECLIPSE, "\nblx_download! enter eclipse mode\n");
		ret = ejtag_blx_download(argc - 1, argv + 1);
		ejtag_exit(ret);
	}
	else if (!strcmp("blx_debug", argv[1]))
	{
		gst_mips_debug.run_mode = ECLIPSE_MODE;
		ejtag_print(FOR_ECLIPSE, "\nblx_debug! enter eclipse mode\n");
		ret = ejtag_blx_debug(argc - 1, argv + 1);
		ejtag_exit(ret);
	}
	else if (!strcmp("blx_gdb", argv[1]))
	{
		gst_mips_debug.run_mode = EJTAG_MODE;
		ejtag_print(FOR_EJTAG, "\nblx_gdb! enter ejtag mode\n");
		ejtag_print(FOR_EJTAG, "	U can use gdb and command\n");
		ret = ejtag_blx_gdb(argc - 1, argv + 1);
		ejtag_exit(ret);
	}
	else if (!strcmp("blx_command", argv[1]))
	{
		gst_mips_debug.run_mode = EJTAG_MODE;
		ejtag_print(FOR_EJTAG, "\nblx_command! enter ejtag mode\n");
		ejtag_print(FOR_EJTAG, "	U can use command only\n");
		ret = ejtag_blx_command(argc - 1, argv + 1);
		ejtag_exit(ret);
	}
	else
	{
		ejtag_usage();
		ejtag_exit(CMD_PARAM_ERR);
	}

	return 0;
}
/*----------------------------------------------------------------------------*/










