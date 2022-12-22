#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mkdev.h>

int
main(int ac, char* av[])
{
    struct stat sb;

    if (stat(av[1], &sb) >= 0 && S_ISCHR(sb.st_mode)) {
	printf("LK.%03d.%03d.%03d\n",
	    major(sb.st_dev),
	    major(sb.st_rdev),
	    minor(sb.st_rdev));
	return (0);
    } else
	return (-1);
}
