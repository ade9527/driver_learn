#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

void print_usage(char *file)
{
	printf("Operation fail:\n");
	printf("Usage:\n");
	printf("%s <file_name> <r/w> <num/string>\n", file);
}

/* addr 参数没做 */
int main(int argc, char *argv[])
{
	int fd;
	char *buf = NULL;
	int len = 0;
	int ret = 0;

	if (argc != 4 || (strcmp(argv[2], "r") != 0 &&
					strcmp(argv[2], "w") != 0)) {
		print_usage(argv[0]);
		return -1;
	}

	if (*argv[2] == 'r') {
		len = atoi(argv[3]);
		printf("read %d byte\n", len);
		if (len == 0)
			return 0;
		buf = malloc(len);
		if (!buf) {
			printf("alloc memory failed\n");
			return -1;
		}
	}

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		printf("Can't open %s\n", argv[1]);
		ret = -1;
		goto mem_free;
	}

	if (*argv[2] == 'r') {
		ret = read(fd, buf, len);
		if (ret < 0) {
			printf("read error\n");
			goto file_close;
		}
		printf("actual read %d byte\n", ret);
		for (int i = 0; i < ret; i++) {
			printf("%c,", buf[i]);
		}
		printf("\n");
		ret = 0;
	} else { /* write */
		len = strlen(argv[3]) + 1;
		printf("write %d byte\n", len);
		ret = write(fd, argv[3], len);
		if (ret < 0) {
			printf("write error\n");
			goto file_close;
		}
		printf("actual write %d byte\n", ret);
		ret = 0;
	}


file_close:
	close(fd);
mem_free:
	if (buf != NULL)
		free(buf);
	return ret;
}
