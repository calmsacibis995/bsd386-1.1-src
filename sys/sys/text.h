/*
 * Copyright (c) 1991, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: text.h,v 1.16 1993/11/12 01:58:44 karels Exp $
 */

/*
 * Information about an executable file.
 * Some fields are used only during an exec
 * (x_flags, x_path, x_strings, x_string_size, x_arg_count, x_args).
 * XXX Text caching is not implemented yet.
 * The 'x_*' member name template comes from the (free) Reno text.h.
 */
struct text {
	int	x_flags;		/* X_PATH_SYSSPACE */
	time_t	x_timestamp;		/* used for cache invalidation */
	char	*x_path;		/* pathname of exec file */
	struct	vnode *x_vnode;		/* vnode of exec file */
	struct	vattr x_vattr;		/* attributes of exec file */
#define	x_size	x_vattr.va_size
	char	*x_header;		/* first page of exec file */
	char	*x_stack;		/* address of base of user stack */
	char	*x_stack_top;		/* address of top of user stack */
	char	*x_strings;		/* copies of arguments, environment */
	int	x_string_size;		/* space used by strings */
	u_long	x_entry;		/* initial pc */
	uid_t	x_uid;			/* new uid if file is setuid */
	gid_t	x_gid;			/* new gid if file is setgid */
	struct	proc *x_proc;		/* cached proc pointer for VOP_*() */
	int	x_arg_count;		/* used in space estimations */
	struct	exec_arg *x_args;	/* chain of string pointers */
	struct	exec_load *x_load_commands;	/* chain of load requests */
	/* XXX vmspace parameters: at some point, these should go away */
	segsz_t x_tsize;
	segsz_t x_dsize;
	caddr_t	x_taddr;
	caddr_t	x_daddr;
};

#define	X_PATH_SYSSPACE	0x01	/* x_path is system space pointer, not user */
#define	X_ENTRY		0x02	/* x_entry is valid */
#define	X_SET_UID	0x04	/* x_uid is valid */
#define	X_SET_GID	0x08	/* x_gid is valid */

/*
 * A chain of exec_arg structures is used to build
 * the argument and environment arrays.
 * The linked list form makes it simple for exec
 * to allocate a variable number of arguments,
 * and for exec handlers (e.g. exec_interpreter)
 * to manipulate the argument list.
 */
struct exec_arg {
	char	*ea_string;
	struct	exec_arg *ea_next;
};

/*
 * A chain of exec_load structures is used to build
 * a process's address space after an exec.
 * If the data is mapped from a vnode,
 * we keep a reference to the vnode.
 * The vnode here is not necessarily the one
 * in the associated text structure.
 */
enum exec_op { EXEC_ZERO, EXEC_MAP, EXEC_READ, EXEC_CLEAR };

struct exec_load {
	enum	exec_op el_op;		/* the actual command */
	struct	vnode *el_vnode;	/* where to get data */
	off_t	el_offset;		/* offset of the data in the vnode */
	vm_offset_t	el_address;	/* address of segment to be loaded */
	vm_size_t	el_length;	/* length of segment */
	vm_prot_t	el_prot;	/* protection of segment */
	int	el_attr;		/* shared, COW, private */
	struct	exec_load *el_next;	/* chain of load commands */
};

#ifdef KERNEL
/*
 * Function declarations.
 */
int exec_interpreter __P((struct text *));
int exec_compact_demand_load_binary __P((struct text *));
int exec_demand_load_binary __P((struct text *));
int exec_shared_binary __P((struct text *));
int exec_unshared_binary __P((struct text *));

int exec_map_segment __P((struct proc *, struct exec_load *));
int exec_read_segment __P((struct proc *, struct exec_load *));
int exec_zero_segment __P((struct proc *, struct exec_load *));

int analyze_exec_header __P((struct text *));
void delete_text __P((struct text *));
void exec_close_files __P((struct proc *));
int exec_gather_arguments __P((struct text *, char **, char **));
int exec_install_arguments __P((struct text *));
int exec_lookup __P((struct text *));
void exec_new_pcomm __P((struct proc *, char *));
void exec_set_entry __P((struct text *, u_long));
void exec_set_state __P((struct text *));
#endif
