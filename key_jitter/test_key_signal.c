#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>

static int fd;
static unsigned int key_val;

void signal_fun(int signum)
{
	read(fd, &key_val, sizeof(key_val));
	printf("signal function, key val:%x\n", key_val);
	return;
}

int main(int argc, char *argv[])
{
	int ret;
	int oflags;

	signal(SIGIO, &signal_fun);

	fd = open("/dev/my_key_irq", O_RDWR);
	if (fd < 0) {
		printf("open my_key error\n");
		return -1;
	}

	printf("fcntl 0\n");
	fcntl(fd, F_SETOWN, getpid());
	printf("fcntl 1\n");
	oflags = fcntl(fd, F_GETFL);
	printf("fcntl 2\n");
	fcntl(fd, F_SETFL, oflags | FASYNC);
	printf("fcntl 3\n");

	while (1) {
		sleep(10);
	}
}
