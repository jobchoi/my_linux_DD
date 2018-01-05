#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h> 

void off_signal_handler (int signum)                        /* 시그널 처리를 위한 핸들러 */
{
	static int count = 0;
	count++;
    if(signum == SIGUSR1) {                                     /* SIGIO이면 애플리케이션을 종료한다. */
		printf("off signal : %d \n",count );
		printf("off Signal is Catched!!!\n");
		if(count >= 5)
			exit(1);
    }

    return;
}

void on_signal_handler (int signum)                        /* 시그널 처리를 위한 핸들러 */
{
	static int count = 0;
	count++;
    if(signum == SIGUSR2) {                                     /* SIGIO이면 애플리케이션을 종료한다. */
		printf("on signal : %d \n",count );
		printf("on Signal is Catched!!!\n");
		if(count >= 5)
			exit(1);
    }

    return;
}

int main(int argc, char** argv)
{
    char buf[BUFSIZ];
    char i = 0;
    int fd = -1;
    memset(buf, 0, BUFSIZ);

    signal(SIGUSR1, off_signal_handler);    /* 시그널 처리를 위한 핸들러를 등록한다. */
    signal(SIGUSR2, on_signal_handler);    /* 시그널 처리를 위한 핸들러를 등록한다. */

    printf("GPIO Set : %s\n", argv[1]);
    fd = open("/dev/gpioled", O_RDWR);
	printf("---%d\n" ,fd);
    sprintf(buf, "%s:%d", argv[1], getpid());

    write(fd, buf, strlen(buf));

    if(read(fd, buf, strlen(buf)) != 0)
        printf("Success : read( )\n");

    printf("Read Data : %s\n", buf);

    printf("My PID is %d.\n", getpid());

    while(1);                                                         /* pause(); */

    close(fd);

    return 0;
}

