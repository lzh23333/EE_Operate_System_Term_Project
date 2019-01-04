#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(int agrc, char ** argv){
    int fd ;
    int buffer_size = 1024;
    char data[1024];
    printf("reader begin\n");
    
    fd = open("/dev/my_pipe",O_RDONLY);
    printf("%d \n",fd);
    if(fd < 0){
        printf("can't open\n");
    }
    
    while(1){
        int count = read(fd,data,buffer_size);
        if(count > 0){
            for(int i = 0; i < count; i++){
                printf("%c",data[i]);
            }
            printf("\n");
        }
    }
    close(fd);
    return 0;
}