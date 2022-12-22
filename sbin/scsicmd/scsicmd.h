/*
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: scsicmd.h,v 1.2 1992/11/09 18:47:18 trent Exp $
 */

typedef struct parameter {
	u_char	pm_bufno;	/* one of several buffers (0 => SCSI CDB) */
	u_char	pm_byte;	/* byte offset of high bit */
	u_char	pm_bit;		/* bit offset within byte */
	u_char	pm_len;		/* length of field in bits */
	char	*pm_name;	/* short name for parameter arguments */
	char	*pm_desc;	/* long name for printed description */
	u_long	pm_value;	/* value set by user or returned by SCSI */
	char	*pm_string;	/* like pm_value but for string parameters */
	const	char *pm_code;	/* translated status code for pm_value */
} pm;

typedef struct buffer {
	u_char	*bf_buf;	/* pointer to buffer space */
	u_long	bf_len;		/* length of buffer space */
	int	bf_overlap;	/* if bf_len==0, overlaps with this bufno */
	char	*bf_name;	/* short name for parameter arguments */
	char	*bf_desc;	/* long name for printed description */
	u_long	bf_return_len;	/* returned length */
} bf;

typedef struct command {
	bf	*cmd_bf;	/* address of buffer table */
	pm	*cmd_pm;	/* address of parameter table */
	void	(*cmd_ex) __P((void));		/* execute function */
	char	*cmd_name;	/* name of command */
	char	*cmd_desc;	/* description of command */
	void	(*cmd_code) __P((pm *));	/* code translation function */
	int	cmd_flag;	/* initialization state */
} cmd;

#define	CMD_INIT	0x1	/* committed initializations to buffers */

extern cmd *current_cmd;

typedef struct hosttype {
	cmd	*ht_cmd;	/* address of command table */
	char	*ht_name;	/* name (e.g. SCSI, aha, etc.) */
	char	*ht_desc;	/* description of host type */
} ht;

extern ht *current_ht;
extern ht httab[];

typedef u_char	ucdb[12];

extern int scsi;
extern char *scsiname;

void execute_command __P((void));
pm *extract_bf __P((char *, int));
void inquiry_code __P((pm *));
void mode_select_cmd __P((void));
void mode_sense_cmd __P((void));
void print_parameters __P((char *));
void read_cmd __P((void));
void read_data_cmd __P((void));
void read_var_cmd __P((void));
void request_sense_code __P((pm *));
void scsi_sense __P((void));
cmd *search_command __P((char *));
void set_parameters __P((char *));
char *sprintasc __P((unsigned, unsigned));
void start_command __P((char *));
void store_pm __P((pm *));
void write_cmd __P((void));

#define	BUFFER_SIZE	256
