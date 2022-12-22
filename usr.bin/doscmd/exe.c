/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
 
/*	Krystal $Id: exe.c,v 1.2 1994/01/14 23:28:31 polk Exp $ */

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include "doscmd.h"

int	pspseg;
int	curpsp = 0;
int	psp_s[10] = { 0 };
int	env_s[10];
struct trapframe frames[10];

char *env_block;

int
make_environment (char *cmdname, char **env)
{
	int i;
	int total;
	int len;
	int envseg;
	char *p;
	char *env_block;

	total = 0;
	for (i = 0; env[i]; i++) {
	    debug (D_EXEC,"env: %s\n", env[i]);
	    len = strlen (env[i]);
	    if (total + len >= 32 * 1024)
		break;
	    total += len + 1;
	}

	total++; /* terminating null */
	total += 2; /* word count */
	total += strlen (cmdname) + 1;
	total += 4; /* some more zeros, just in case */

	if ((envseg = mem_alloc(total/16 + 1, 1, NULL)) == 0)
		fatal("out of memory for env\n");

	env_block = MAKE_ADDR(envseg, 0);
	memset (env_block, 0, total);

	p = env_block;
	total = 0;
	for (i = 0; env[i]; i++) {
		len = strlen (env[i]);
		if (total + len >= 32 * 1024)
			break;
		total += len + 1;
		strcpy (p, env[i]);
		p += strlen (p) + 1;
	}
	*p++ = 0;
	*(short *)p = strlen(cmdname);
	p += 2;
	strcpy (p, cmdname);
	while(*p) {
		if (*p == '/')
			*p = '\\';
		else if (islower(*p))
		    	*p = toupper(*p);
		p++;
	}
	*p = '\0';
	return envseg;
}

struct exehdr {
	u_short magic;
	u_short bytes_on_last_page;
	u_short size; /* 512 byte blocks */
	u_short nreloc;
	u_short hdr_size; /* paragraphs */
	u_short min_memory; /* paragraphs */
	u_short max_memory; /* pargraphs */
	u_short init_ss;
	u_short init_sp;
	u_short checksum;
	u_short init_ip;
	u_short init_cs;
	u_short reloc_offset;
	u_short overlay_num;
};

struct reloc_entry {
	u_short off;
	u_short seg;
};

load_command (int fd, char *cmdname, char **argv, char **envs)
{
	struct exehdr hdr;
	int min_memory;
	int max_memory;
	int biggest;
	int envseg;
	char *psp;
	int text_size;
	int reloc_size;
	struct reloc_entry *reloc_tbl, *rp;
	int i;
	char *start_addr;
	int start_segment;
	u_short *segp;
	int exe_file;
	char *p;
	int used, n;

	if (envs)
		envseg = make_environment (cmdname, envs);
	else
		envseg = env_s[curpsp];

	/* read exe header */
	if (read (fd, &hdr, sizeof hdr) != sizeof hdr)
		fatal ("can't read header\n");

	/* proper header ? */
	if (hdr.magic == 0x5a4d) {
		exe_file = 1;
		text_size = (hdr.size - 1) * 512 + hdr.bytes_on_last_page
			- hdr.hdr_size * 16;

		min_memory = hdr.min_memory + (text_size + 15)/16;
		max_memory = hdr.max_memory + (text_size + 15)/16;
	} else {
		exe_file = 0;
		min_memory = 64 * (1024/16);
		max_memory = 0xffff;
	}

	/* alloc mem block */
	pspseg = mem_alloc(max_memory, 1, &biggest);
	if (pspseg == 0) {
		if (biggest < min_memory ||
		   (pspseg = mem_alloc(biggest, 1, NULL)) == 0)
			fatal("not enough memory: needed %d have %d\n",
			      min_memory, biggest);

		max_memory = biggest;
	}

	mem_change_owner(pspseg, pspseg);
	mem_change_owner(envseg, pspseg);

	/* create psp */
	psp_s[++curpsp] = pspseg;
	env_s[curpsp] = envseg;

	psp = MAKE_ADDR(pspseg, 0);
	memset(psp, 0, 256);

	psp[0] = 0xcd;
	psp[1] = 0x20;

	*(u_short *)&psp[2] = pspseg + max_memory;

	/*
	 * this is supposed to be a long call to dos ... try to fake it
	 */
	psp[5] = 0xcd;
	psp[6] = 0x99;
	psp[7] = 0xc3;

	*(u_short *)&psp[0x16] = psp_s[curpsp-1];
	psp[0x18] = 1;
	psp[0x19] = 1;
	psp[0x1a] = 1;
	psp[0x1b] = 0;
	psp[0x1c] = 2;
	memset(&psp[0x1d], 0xff, 15);

	*(u_short *)&psp[0x2c] = envseg;

	*(u_short *)&psp[0x32] = 20;
	*(u_long *)&psp[0x34] = (pspseg << 16) | 0x18;
	*(u_long *)&psp[0x38] = 0xffffffff;

	psp[0x50] = 0xcd;
	psp[0x51] = 0x98;
	psp[0x52] = 0xc3;

	memset(psp + 0x5d, 0x20, 11);
	memset(psp + 0x6d, 0x20, 11);

	p = psp + 0x81;
    	*p = 0;
	used = 0;

	for (i = 0; argv[i]; i++) {
		n = strlen (argv[i]);
		if (used + 1 + n > 0x7d)
			break;
		*p++ = ' ';
		strcpy (p, argv[i]);
		p += strlen (p);
	}

	psp[0x80] = strlen (psp + 0x81);
	psp[0x81 + psp[0x80]] = 0x0d;
	psp[0x82 + psp[0x80]] = 0;

	disk_transfer_addr = psp + 0x80;
	disk_transfer_seg = ((int)disk_transfer_addr) >> 4;
	disk_transfer_offset = ((int)disk_transfer_addr) & 0x0f;

	start_addr = psp + 0x100;
	start_segment = pspseg + 0x10;

	if (!exe_file) {
		lseek (fd, 0, 0);
		i = read (fd, start_addr, max_memory * 16);
		close (fd);

		debug(D_EXEC, "Read %05x into %04x\n",
			       i, (long)start_addr >> 4);

		init_cs = pspseg;
		init_ip = 0x100;
		init_ss = init_cs;
		init_sp = 0xfffe;
		init_ds = init_cs;
		init_es = init_cs;

		return;
	}

	lseek (fd, hdr.hdr_size * 16, 0);
	if (read (fd, start_addr, text_size) != text_size)
		fatal ("error reading program text\n");
	debug(D_EXEC, "Read %05x into %04x\n",
		       text_size, (long)start_addr >> 4);

    	if (hdr.nreloc) {
	    reloc_size = hdr.nreloc * sizeof (struct reloc_entry);

	    if ((reloc_tbl = (struct reloc_entry *)malloc (reloc_size)) == NULL)
		    fatal ("out of memory for program\n");

	    lseek (fd, hdr.reloc_offset, 0);
	    if (read (fd, reloc_tbl, reloc_size) != reloc_size)
		    fatal ("error reading reloc table\n");

	    for (i = 0, rp = reloc_tbl; i < hdr.nreloc; i++, rp++) {
		    segp = (u_short *)(start_addr + (int)MAKE_ADDR (rp->seg, rp->off));
		    *segp += start_segment;
	    }
	    free((char *)reloc_tbl);
    	}

	close (fd);

	init_cs = hdr.init_cs + start_segment;
	init_ip = hdr.init_ip;
	init_ss = hdr.init_ss + start_segment;
	init_sp = hdr.init_sp;

	init_ds = pspseg;
	init_es = init_ds;
}

int
get_psp(void)
{
	return(psp_s[curpsp]);
}

int
get_env(void)
{
	return(env_s[curpsp]);
}

int
exec_command(int fd, char *prog, u_short *param, struct trapframe *tf)
{
    char *arg;
    char *env;
    char *argv[2];
    char *envs[100];

    env = MAKE_ADDR(param[0], 0);
    arg = MAKE_ADDR(param[2], param[1]);

    if (arg) {
	int nbytes = *arg++;
	arg[nbytes] = 0;
	if (!*arg)
	    arg = NULL;
    }
    argv[0] = arg;
    argv[1] = NULL;

    debug (D_EXEC, "exec_command: prog = %s\n"
		   "env = 0x0%x, arg = %04x:%04x(%s)\n",
    	prog, param[0], param[2], param[1], arg);

    if (env) {
	int i;
	for ( i=0; i < 99 && *env; ++i ) {
	    envs[i] = env;
	    env += strlen(env)+1;
	}
	envs[i] = NULL;
	load_command(fd, prog, argv, envs);
    } else
	load_command(fd, prog, argv, NULL);

    frames[curpsp] = *tf;

    {
	char *fcb;
	char *psp = MAKE_ADDR(psp_s[curpsp], 0);

	if (param[4]) {
	    fcb = MAKE_ADDR(param[4],param[3]);
	    memcpy(psp+0x5c, fcb, 16);
	}
	if (param[6]) {
	    fcb = MAKE_ADDR(param[6],param[5]);
	    memcpy(psp+0x6c, fcb, 16);
	}
    }
}

void
exec_return(struct trapframe *tf, int code)
{
    debug(D_EXEC, "Returning from exec\n");
    mem_free_owner(psp_s[curpsp]);
    *tf = frames[curpsp--];
    tf->tf_ax = code;
    tf->tf_eflags &= ~PSL_C;	/* It must have worked */
}
