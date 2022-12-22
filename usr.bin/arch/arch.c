
#include <stdio.h> 

#ifdef i386
char *arch = "i386";
#else
char *arch = "unknown";
#endif

main(argc, argv)
	int argc;
	char *argv[];
{
	if (argc == 1) {
		printf("%s\n", arch);
		exit(0);
	} 
	
	if (argc == 2) {
		exit(strcmp(arch, argv[1]) ? 1 : 0);
	}

	printf("usage: arch\n       arch archname\n"); 
}
