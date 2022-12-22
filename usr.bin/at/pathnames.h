/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#ifndef AT_DEFSHELL
#define AT_DEFSHELL	_PATH_BSHELL
#endif

#ifndef AT_DIR
#define AT_DIR		"/var/spool/at/"
#endif

/*
 * subdirectory of AT_DIR for spool jobs and output file
 */
#ifndef AT_LOCKDIR
#define AT_LOCKDIR	"past"
#endif

#ifndef AT_LOCK
#define AT_LOCK		"at.counter"
#endif

#ifndef AT_RUNLOCK
#define AT_RUNLOCK	"atrun.lock"
#endif

/*
 * access control, argv[1] = username, return 0 if access allowed
 */
#ifndef AT_ALLOWED
#define AT_ALLOWED	"/usr/libexec/at_allowed"
#endif

/*
 * Where commands are exec'ed from.  Must be readable by world
 * or shell init will fail.
 *
 * If you are trusting you could just make it AT_DIR/AT_LOCKDIR and
 * make AT_LOCKDIR readable by world.
 * 
 * If all goes we'll the script will chdir() to the original directory
 * anyway.
 */
#ifndef AT_TMP
#define AT_TMP		"/tmp"
#endif
