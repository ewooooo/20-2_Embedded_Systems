#include <stdio.h>
#include <stdlib.h> // atoi str->int 변환
#include <unistd.h> // file
#include <fcntl.h>  // file
#include <sys/ioctl.h> //ioctl

#define DEV_MAJOR_NUMBER 275
#define CH_READ_BUFFER_SIZE _IOW(DEV_MAJOR_NUMBER,1,int)

int main(int argc, char* argv[])
{
    int dev;
    dev = open("/dev/BufferedMem", O_RDWR);
    if (dev < 0)
    {
        printf("driver open failed!\n");
        return -1;
    }
    if (argc<1){
        printf("error arg [modify buffer size]");
        return -1;
    }
    int buf_size = atoi(argv[1]);
    ioctl(dev, CH_READ_BUFFER_SIZE, &buf_size);
    printf("success modify Read buffer size = %d\n",buf_size);
    close(dev);
    return 0;
}