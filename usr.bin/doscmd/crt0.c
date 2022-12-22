/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: crt0.c,v 1.1 1994/01/14 23:24:57 polk Exp $ */
char **environ;

start(int argc, char **argv, char **env)
{
  environ = env;
  exit(main(argc, argv, environ));
}
