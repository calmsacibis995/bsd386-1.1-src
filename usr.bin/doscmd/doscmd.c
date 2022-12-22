/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: doscmd.c,v 1.4 1994/01/31 00:50:46 polk Exp $ */

#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <signal.h>
#include <machine/psl.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <errno.h>
#include <limits.h>

#include <machine/param.h>
#include <machine/vmlayout.h>

#include "doscmd.h"

FILE *debugf;
int  intnum;
int  xmode = 0;
int  dosmode = 0;
int  use_fork = 0;
int  tflag = 0;
int  ecnt = 0;
int booting = 0;
char *envs[256];
unsigned long *ivec = (unsigned long *)0;

void trap (struct sigframe sf, struct trapframe tf);
void sigtrap (struct sigframe sf, struct trapframe tf);
void sigill (struct sigframe sf, struct trapframe tf);
void floating (struct sigframe sf, struct trapframe tf);
void random_sig_handler(struct sigframe sf, struct trapframe tf);
void breakpoint(struct sigframe sf, struct trapframe tf);

static void setsignal(int signum, void *handler)
{
  struct sigaction sa;

  sa.sa_handler = handler;
  sa.sa_mask = sigmask(SIGIO) | sigmask(SIGALRM);
  sa.sa_flags = SA_ONSTACK;
  sigaction(signum, &sa, NULL);
}

char signal_stack[16 * 1024];

void
usage ()
{
	fprintf (stderr, "usage: doscmd cmd args...\n");
	exit (1);
}

#define	BPW	(sizeof(u_long) << 3)
u_long debug_ints[256/BPW];

inline void
debug_set(int x)
{
    x &= 0xff;
    debug_ints[x/BPW] |= 1 << (x & (BPW - 1));
}

inline void
debug_unset(int x)
{
    x &= 0xff;
    debug_ints[x/BPW] &= ~(1 << (x & (BPW - 1)));
}

inline u_long
debug_isset(int x)
{
    x &= 0xff;
    return(debug_ints[x/BPW] & (1 << (x & (BPW - 1))));
}

char *dos_path = 0;
char cmdname[256];

char *memfile = "/tmp/doscmd.XXXXXX";

main (int argc, char **argv)
{
	struct sigstack sstack;
	struct trapframe tf;
	struct sigframe sf;
	int fd = -1;
	int i;
	int c;
	extern int optind;
	char prog[1024];
    	char buffer[4096];
	char *ptr;
	int zflag = 0;
    	int mfd;
	FILE *fp;

	exec_level = 0;

	debug_flags = D_ALWAYS;
	debugf = stderr;

	fd = open ("/dev/null", O_RDWR);
	if (fd != 3)
		dup2 (fd, 3); /* stdaux */
	if (fd != 4)
		dup2 (fd, 4); /* stdprt */
	if (fd != 3 && fd != 4)
		close (fd);

    	fd = -1;

    	debug_set(0);		/* debug any D_TRAPS without intnum */

	while ((c = getopt (argc, argv, "IEMPRAU:S:HDtzvVxfb")) != EOF) {
		switch (c) {
    	    	case 'I':
			debug_flags |= D_ITRAPS;
			for (c = 0; c < 256; ++c)
				debug_set(c);
			break;
    	    	case 'E':
			debug_flags |= D_EXEC;
			break;
    	    	case 'M':
			debug_flags |= D_MEMORY;
			break;
		case 'P':
			debug_flags |= D_PORT;
			break;
		case 'R':
			debug_flags |= D_REDIR;
			break;
		case 'A':
			debug_flags |= D_TRAPS|D_ITRAPS;
			for (c = 0; c < 256; ++c)
				debug_set(c);
			break;
		case 'U':
			debug_unset(strtol(optarg, 0, 0));
			break;
		case 'S':
			debug_flags |= D_TRAPS|D_ITRAPS;
			debug_set(strtol(optarg, 0, 0));
			break;
		case 'H':
			debug_flags |= D_HALF;
			break;
		case 'x':
			xmode = 1;
			break;
    	    	case 'f':
			use_fork = 1;
			break;
		case 't':
			tflag = 1;
			break;
		case 'z':
			zflag = 1;
			break;
    	    	case 'D':
			debug_flags |= D_DISK | D_FILE_OPS;
			break;
		case 'v':
			debug_flags |= D_TRAPS | D_ITRAPS | D_HALF | 0xff;
			break;
		case 'V':
			vflag = 1;
			debug_flags = -1;
			break;
		case 'b':
			booting = 1;
	    	    	xmode = 1;
			break;
		default:
			usage ();
		}
	}

    	if (xmode && use_fork) {
		fprintf(stderr, "Cannot both enable fork and X\n");
		exit(1);
    	}

    	if (dosmode && use_fork) {
		fprintf(stderr, "Cannot both enable fork and dos\n");
		exit(1);
    	}

	if (vflag) {
		debugf = stdout;
		setbuf (stdout, NULL);
	}

	mfd = mkstemp(memfile);

	if (mfd < 0) {
		fprintf(stderr, "memfile: %s\n", strerror(errno));
		fprintf(stderr, "High memory will not be mapped\n");
	} else {
	    caddr_t add;

	    unlink(memfile);

    	    mfd = squirrel_fd(mfd);

	    lseek(mfd, 64 * 1024 - 1, 0);
	    write(mfd, "", 1);
	    add = mmap((caddr_t)0x000000, 64 * 1024,
			PROT_EXEC | PROT_READ | PROT_WRITE,
			MAP_FILE | MAP_FIXED | MAP_INHERIT | MAP_SHARED,
			mfd, 0);
	    add = mmap((caddr_t)0x100000, 64 * 1024,
			PROT_EXEC | PROT_READ | PROT_WRITE,
			MAP_FILE | MAP_FIXED | MAP_INHERIT | MAP_SHARED,
			mfd, 0);
	}
	
	mem_init();
	init_devinit_handlers();

	/*
	 * initialize all port handlers for INB, OUTB, ...
	 */
	init_io_port_handlers();

	if ((fp = fopen(".doscmdrc", "r")) == NULL) {
	    struct passwd *pwd = getpwuid(geteuid());
	    if (pwd) {
		sprintf(buffer, "%s/.doscmdrc", pwd->pw_dir);
		fp = fopen(buffer, "r");
	    }
	    if (!fp)
		fp = fopen("/etc/doscmdrc", "r");
	}

    	/*
	 * With no arguments we will assume we must boot dos
	 */
	if (optind >= argc) {
#if defined(VER11)
    	    if (!booting) {
		fprintf(stderr, "Usage: doscmd cmd [args]\n");
		exit(1);
	    }
#endif
	    booting = 1;
	    xmode = 1;		/* Needs to be true for now */
    	}
#if defined(VER11)
    	if (booting || xmode) {
	    fprintf(stderr, "Warning: You are using a not yet supported or documented feature of doscmd.\n");
	    fprintf(stderr, "       : Proceed at your own risk.\n");
	    fprintf(stderr, "       : Do not cry out for help.\n");
	}
#endif

    	if (fp) {
	    if (booting) {
		booting = read_config(fp);
    	    	if (booting < 0) {
		    if ((fd = disk_fd(booting = 0)) < 0)	/* A drive */
			fd = disk_fd(booting = 2);		/* C drive */
    	    	} else {
		    fd = disk_fd(booting);
    	    	}

    	    	if (fd < 0) {
		    fprintf(stderr, "Cannot boot from %c: (can't open)\n",
				     booting + 'A');
		    exit(1);
    	    	}
    	    } else
		read_config(fp);
    	} else if (booting) {
	    fprintf(stderr, "You must have a doscmdrc to boot\n");
	    exit(1);
	}

    	if (fd >= 0) {
	    if (read(fd, (char *)0x7c00, 512) != 512) {
		fprintf(stderr, "Short read on boot block from %c:\n",
			        booting + 'A');
		exit(1);
	    }
	    dosmode = 1;
	    init_cs = 0x0000;
	    init_ip = 0x7c00;

	    init_ds = 0x0000;
	    init_es = 0x0000;
	    init_ss = 0x9800;
	    init_sp = 0x8000 - 2;
	} else {
	    if (optind >= argc)
		    usage ();

	    for (i = 0; i < ecnt; ++i) {
		if (!strncmp(envs[i], "COMSPEC=", 8))
		    break;
	    }
	    if (i >= ecnt)
	    	put_dosenv("COMSPEC=C:\\COMMAND.COM");

	    for (i = 0; i < ecnt; ++i) {
		if (!strncmp(envs[i], "PATH=", 5)) {
		    dos_path = envs[i] + 5;
		    break;
		}
	    }
	    if (i >= ecnt) {
	    	put_dosenv("PATH=C:\\");
		dos_path = envs[ecnt-1] + 5;
	    }

	    for (i = 0; i < ecnt; ++i) {
		if (!strncmp(envs[i], "PROMPT=", 7))
		    break;
	    }
	    if (i >= ecnt)
	    	put_dosenv("PROMPT=DOS> ");

	    envs[ecnt] = 0;

	    if (dos_getcwd(2) == NULL) {
		char *p;

		p = getcwd(buffer, sizeof(buffer));
		if (!p || !*p) p = getenv("PWD");
		if (!p || !*p) p = "/";
		init_path(2, (u_char *)"/", (u_char *)p);
	    }

	    if (dos_getcwd('R' - 'A') == NULL)
		init_path('R' - 'A', (u_char *)"/", 0);

	    strncpy(prog, argv[optind++], sizeof(prog) -1);
	    prog[sizeof(prog) -1] = '\0';

	    if ((fd = open_prog (prog)) < 0) {
		    fprintf (stderr, "%s: command not found\n", prog);
		    exit (1);
	    }
	    load_command (fd, cmdname, argv + optind, envs);
	}

	sstack.ss_sp = signal_stack + sizeof signal_stack;
	sstack.ss_onstack = 0;
	sigstack (&sstack, NULL);

	setsignal (SIGSEGV, random_sig_handler);
	setsignal (SIGFPE, floating);		/**/

	setsignal (SIGBUS, trap);		/**/
	setsignal (SIGILL, sigill);		/**/
	setsignal (SIGILL, trap);		/**/
	setsignal (SIGTRAP, sigtrap);		/**/

	video_init();
	video_bios_init();
	cmos_init();
    	bios_init();

	init_optional_devices();

if (0) {
    char *memfile = "/tmp/bios.XXXXXX";
    mfd = mkstemp(memfile);

    if (mfd < 0) {
	    fprintf(stderr, "memfile: %s\n", strerror(errno));
	    fprintf(stderr, "BIOS memory will be mapped\n");
    } else {
	caddr_t add;
    	caddr_t start = (caddr_t)(0xF0000 + 4 * 1024);
	size_t size = 56 * 1024;

	unlink(memfile);

    	mfd = squirrel_fd(mfd);

	add = mmap(start, size,
		    PROT_EXEC | PROT_READ | PROT_WRITE,
		    MAP_FILE | MAP_FIXED | MAP_INHERIT | MAP_SHARED,
		    mfd, 0);
	if (add != start)
	    fprintf(stderr, "Did not unmap upper %04x bytes from %05x\n", size, start);
	else
	    fprintf(stderr, "Unmapped upper %04x bytes from %05x\n", size, start);
	add = *(caddr_t *)0x100000;
    }
}

	gettimeofday(&boot_time, 0);

	if (zflag) for (;;) pause();

	tf.tf_ax = 0;
	tf.tf_bx = 0;
	tf.tf_cx = 0;
	tf.tf_dx = 0;
	tf.tf_si = 0;
	tf.tf_di = 0;
	tf.tf_bp = 0;
	

	tf.tf_eflags = 0x20202;
	tf.tf_cs = init_cs;
	tf.tf_ip = init_ip;
	tf.tf_ss = init_ss;
	tf.tf_sp = init_sp;

	tf.tf_ds = init_ds;
	tf.tf_es = init_es;

	sf.sf_eax = -1;
	sf.sf_scp = &sf.sf_sc;
	sf.sf_sc.sc_ps = PSL_VM;
	sf.sf_sc.sc_mask = 0;
	sf.sf_sc.sc_onstack = 0;

    	if (tflag) {
		tmode = 1;
		tracetrap(&tf);
    	}
#if 0
        __asm__ volatile("mov 0x8e64, %eax");		/* Location to trap */
	__asm__ volatile(".byte 0x0f");			/* MOV DR0,EAX */
	__asm__ volatile(".byte 0x21");
	__asm__ volatile(".byte 0x03");
        __asm__ volatile("mov 0x00070002, %eax");	/* word,rw,global */
	__asm__ volatile(".byte 0x0f");			/* MOV DR7,EAX */
	__asm__ volatile(".byte 0x21");
	__asm__ volatile(".byte 0x1f");
#endif
#if 0
	*(long *)(USRSTACK - 20) = 0x12345678L;
	*(long *)(USRSTACK - 24) = (0x0107 << 4) + 0x7f74;
	*(u_short *)(USRSTACK - 28) = *(u_short *)(*(long *)(USRSTACK - 24));
#endif

	switch_vm86 (sf, tf);

	if (vflag) dump_regs(&tf);
	fatal ("vm86 returned\n");
}

int
open_name(char *name, char *ext)
{
    int fd;
    char *p = name;
    while (*p)
	++p;

    *ext = 0;
    if (!strstr(name, ".exe") && !strstr(name, ".com")) {
	    strcpy(ext, ".exe");
	    strcpy(p, ".exe");
    }
    if ((fd = open (name, O_RDONLY)) >= 0) {
	    return (fd);
    }

    *p = 0;
    if (!strstr(name, ".exe") && !strstr(name, ".com")) {
	    strcpy(ext, ".com");
	    strcpy(p, ".com");
    }
    if ((fd = open (name, O_RDONLY)) >= 0) {
	    return (fd);
    }
    return(-1);
}

open_prog (name)
char *name;
{
	int fd;
	char *fullname;
	char *dirs;
	char *p;
	char *e;
	int need_slash;
	int end_flag;
    	char ext[5];


	dirs = dos_path;

    	if (name[1] == ':' || name[0] == '/' || name[0] == '\\'
			   || name[0] == '.') {
	    fullname = translate_filename(name);
	    fd = open_name(fullname, ext);

	    strcpy(cmdname, name);
    	    if (*ext)
		strcat(cmdname, ext);
	    return(fd);
	}

	fullname = alloca (strlen (dos_path) + strlen (name) + 10);

	end_flag = 0;

	while (*dirs) {
	    p = dirs;
	    while (*p && *p != ';')
		++p;

	    strncpy(fullname, dirs, p - dirs);
	    fullname[p-dirs] = '\0';
	    e = fullname + (p - dirs);
	    dirs = *p ? p + 1 : p;

	    *e++ = '\\';
	    strcpy(e, name);

	    e = translate_filename(fullname);

	    fd = open_name(e, ext);
	    if (fd >= 0) {
		strcpy(cmdname, fullname);
		if (*ext)
		    strcat(cmdname, ext);
		return(fd);
	    }
	}

	return (-1);
}

void
dump_regs (tf)
struct trapframe *tf;
{
	u_char *addr;
	int i;
	char buf[100];
	debug (D_ALWAYS, "\n");
	debug (D_ALWAYS, "ax=%04x bx=%04x cx=%04x dx=%04x\n",
	       tf->tf_ax, tf->tf_bx, tf->tf_cx, tf->tf_dx);
	debug (D_ALWAYS, "si=%04x di=%04x sp=%04x bp=%04x\n",
	       tf->tf_si, tf->tf_di, tf->tf_sp, tf->tf_bp);
	debug (D_ALWAYS, "cs=%04x ss=%04x ds=%04x es=%04x\n",
		 tf->tf_cs, tf->tf_ss, tf->tf_ds, tf->tf_es);
	debug (D_ALWAYS, "ip=%x flags=%x\n", tf->tf_ip, tf->tf_eflags);

	addr = (u_char *)MAKE_ADDR (tf->tf_cs, tf->tf_ip);

	for (i = 0; i < 16; i++)
		debug (D_ALWAYS, "%02x ", addr[i]);
	debug (D_ALWAYS, "\n");

	disassemble (tf, buf);
	debug (D_ALWAYS, "%s\n", buf);
}

#include <stdarg.h>

void
debug (int flags, char *fmt, ...)
{
	va_list args;

	if (flags & (debug_flags & ~0xff)) {
		if ((debug_flags & 0xff) == 0
		    && (flags & (D_ITRAPS|D_TRAPS))
		    && !debug_isset(flags & 0xff))
			return;
		va_start (args, fmt);
		vfprintf (debugf, fmt, args);
		va_end (args);
	}
}

void
fatal (char *fmt, ...)
{
	va_list args;


    	if (xmode) {
		char buf[1024];
		char buf2[1024];
		char *m;

		va_start (args, fmt);
		vsprintf (buf, fmt, args);
		va_end (args);

		tty_move(23, 0);
		for (m = buf; *m; ++m)
			tty_write(*m, 0x0400);

		tty_move(24, 0);
		for (m = "(PRESS <CTRL-ALT> ANY MOUSE BUTTON TO exit)"; *m; ++m)
			tty_write(*m, 0x0900);
		tty_move(-1, -1);
		for (;;)
			tty_pause();
	}

	va_start (args, fmt);
	fprintf (debugf, "doscmd: fatal error ");
	vfprintf (debugf, fmt, args);
	va_end (args);
	exit (1);
}

done (tf, val)
struct trapframe *tf;
int val;
{
    	if (curpsp < 2  && xmode) {
	    char *m;

	    tty_move(24, 0);
	    for (m = "END OF PROGRAM"; *m; ++m)
		    tty_write(*m, 0x8400);

	    for (m = "(PRESS <CTRL-ALT> ANY MOUSE BUTTON TO exit)"; *m; ++m)
		    tty_write(*m, 0x0900);
	    tty_move(-1, -1);
	    for (;;)
		    tty_pause();
	}
	if (!use_fork)
	    exec_return(tf, val);
    	if (use_fork || !curpsp)
	    exit (val);
}

/* returns instruction length */
disassemble (tf, buf)
struct trapframe *tf;
char *buf;
{
	int addr;

	addr = (int)MAKE_ADDR (tf->tf_cs, tf->tf_ip);
	return (i386dis (tf->tf_cs, tf->tf_ip, addr, buf, 0));
}

void
put_dosenv(char *value)
{
    if (ecnt < sizeof(envs)/sizeof(envs[0])) {
    	if ((envs[ecnt++] = strdup(value)) == NULL) {
	    perror("put_dosenv");
	    exit(1);
	}
    } else {
	fprintf(stderr, "Environment full, ignoring %s\n", value);
    }
}

int
squirrel_fd(int fd)
{
    int sfd = OPEN_MAX;
    struct stat sb;

    do {
	errno = 0;
    	fstat(--sfd, &sb);
    } while (sfd > 0 && errno != EBADF);

    if (errno == EBADF && dup2(fd, sfd) >= 0) {
	close(fd);
	return(sfd);
    }
    return(fd);
}

int
booted()
{
	return(booting);
}

void
unknown_int2(int maj, int min, struct trapframe *tf)
{
    if (vflag) dump_regs(tf);
    printf("Unknown interrupt %02x function %02x\n", maj, min);
    tf->tf_eflags |= PSL_C;
#if defined(VER11)
    if (!booting)
	exit(1);
#endif
}

void
unknown_int3(int maj, int min, int sub, struct trapframe *tf)
{
    if (vflag) dump_regs(tf);
    printf("Unknown interrupt %02x function %02x subfunction %02x\n",
	   maj, min, sub);
    tf->tf_eflags |= PSL_C;
#if defined(VER11)
    if (!booting)
	exit(1);
#endif
}

void
unknown_int4(int maj, int min, int sub, int ss, struct trapframe *tf)
{
    if (vflag) dump_regs(tf);
    printf("Unknown interrupt %02x function %02x subfunction %02x %02x\n",
	   maj, min, sub, ss);
    tf->tf_eflags |= PSL_C;
#if defined(VER11)
    if (!booting)
	exit(1);
#endif
}
