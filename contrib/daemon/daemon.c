#include <stdio.h>

extern int daemon();

main(int argc, char **argv)
{
    int nochdir = 1;
    int noclose = 1;

    ++argv;
    if (**argv == '-') { noclose = 0; nochdir = 0; argv++; }
    if (daemon(nochdir, noclose) == -1)
	exit(-1);
    if (**argv == '\0') {
    	fprintf(stderr, "daemon: null command!\n");
	exit(-1);
    }
    return execvp(argv[0], argv);
}
