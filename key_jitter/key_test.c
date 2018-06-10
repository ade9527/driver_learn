#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

int main(int argc, char *argv[])
{
	int fd;
	unsigned int key_val;
	int ret;

	struct pollfd poll_fds[1];

	fd = open("/dev/my_key_irq", O_RDWR);
	if (fd < 0) {
		printf("open my_key error\n");
		return -1;
	}
	poll_fds[0].fd = fd;
	poll_fds[0].events = POLLIN;

	while (1) {
		ret = poll(poll_fds, 1, 10000);
		if (ret == 0) {
			//printf("key_test:time out\n");
		} else {
			ret = read(fd, &key_val, sizeof(key_val));
			printf("key :0x%x\n", key_val);

		}
	}
}
