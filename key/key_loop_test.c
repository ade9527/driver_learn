#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	int fd;
	unsigned char key_val[4];
	int ret;

	fd = open("/dev/my_key", O_RDWR);
	if (fd < 0) {
		printf("open my_key error\n");
	}

	while (1) {
		ret = read(fd, key_val, sizeof(key_val));
			printf("key :%d,%d,%d,%d\n", key_val[0], key_val[1], key_val[2], key_val[3]);
			usleep(100);
	}
}
