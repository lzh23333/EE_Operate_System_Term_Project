#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "string.h"

int main(int agrc, char ** argv){
    int fd;
    int buffer_size = 1024;
    char data[1024];
    printf("writer begin\n");
    fd = open("/dev/my_pipe",O_WRONLY);
    printf("%d \n",fd);

    if(fd < 0){
        printf("can't open\n");
        return 0;
    }
    

    while(1){
        
        scanf("%s", data);
        write(fd,data,strlen(data));
    }

    close(fd);
    return 0;

}
