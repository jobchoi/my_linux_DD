#include <stdio.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char** argv)
{
    char buf[BUFSIZ];
    char i = 0;
    int fd = -1;
    memset(buf, 0, BUFSIZ);

    printf("GPIO Set : %s\n", argv[1]);
	// Open GPIO 
	fd = open("/dev/gpioled", O_RDWR);

    if(fd<0)
		printf("can not open /dev/gpioled\n"); 

	// Write to GPIO
    write(fd, argv[1], strlen(argv[1]), NULL);

	// Read from Kernel (출력모드로 설정되어 있기에 Kernel로부터 읽는다.)
    read(fd, buf, strlen(argv[1]), NULL);

	printf("Read data : %s\n", buf);
    close(fd);

    return 0;
}

