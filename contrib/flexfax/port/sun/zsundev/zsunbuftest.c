/* quick-and-dirty test program
 * just run it and watch the console messages
 * or use adb -k /vmunix /dev/mem
 *    and watch *zsaline/X (ttya) or *zsaline+150/X (ttyb)
 */

#include <stdio.h>
#include <fcntl.h>
#include <stropts.h>

void
snooze(message)
char *message;
{
	fprintf(stderr, message);
	(void)getc(stdin);
}

main(argc,argv)
char *argv[];
{
	int tty = open("/dev/tty", O_RDWR);
	int dev;

	if (argc !=2) {
		fprintf(stderr, "usage: %s ttydevice\n", argv[0]);
		exit(1);
	}

	snooze("device not yet open\n");

        if ((dev = open(argv[1], O_RDWR)) < 0) {
		perror("error opening device");
		exit(1);
	}

        snooze("device now open\n");

        if (ioctl(dev, I_PUSH, "zsunbuf") < 0) {
		perror("error pushing zsunbuf");
		exit(1);
	}

	snooze("zsunbuf now pushed\n");

	if (ioctl(dev, I_POP, 0) < 0) {
		perror("error popping zsunbuf");
		exit(1);
	}

	snooze("zsunbuf now popped\n");

	close(dev);

	snooze("device now closed\n");

	exit(0);
}
