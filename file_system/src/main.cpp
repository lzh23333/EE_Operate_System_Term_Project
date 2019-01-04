#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <linux/fs.h>
#include<linux/hdreg.h>
#include <iostream>


int get_sector_size(int fd){
    //get sector size of fd
    int size;
    ioctl(fd,BLKSSZGET,&size);
    return size;
}
void sectorDump(char *p, int size){
    int line_num = 16;
    int i = 0;
    while( i < size ){
        unsigned char a = (*(p+i));
        if( line_num == 0 ){
            std::cout << std::endl;
            line_num = 16;
        }
        else{
            unsigned short l = a & 0x0f;
            unsigned short h = (a & 0xf0)>>4;
            std::cout.setf(std::ios_base::right, std::ios_base::adjustfield);
            std::cout.fill(' ');
            std::cout.width(2);
            std::cout <<std::hex<< h << l ;

            line_num --;
            if(line_num % 4 ==0 )
                std::cout << " ";
        }
        i++;
    }
    std::cout << std::endl;
}

int sectorRead(int fd, unsigned long sectorID, char **p){
    int sector_size = get_sector_size(fd);

    lseek(fd,0,SEEK_SET);
    if(lseek(fd,sectorID * sector_size,SEEK_CUR) == -1){
        std::cerr << "no such sector" << std::endl;
    }
    *p = new char[sector_size];

    return read(fd,*p,sector_size);
}



void physicalDisk(int fd){
    //get basic infomation about disk
    int size;
    ioctl(fd,BLKSSZGET,&size);
    long long disk_size;
    ioctl(fd,BLKGETSIZE64,&disk_size);
    hd_geometry hdio;
    ioctl(fd,HDIO_GETGEO,&hdio);
    std::cout << "sector size: " << size << " B " << std::endl
             <<"sectors: " << disk_size/size  << std::endl
            <<"磁头:" << int(hdio.heads) <<std::endl
           <<"柱面:"<<hdio.cylinders << std::endl
          <<"扇区:"<< int(hdio.sectors) << std::endl
         <<"起始:"<< hdio.start << std::endl;
}


int main(int argc, char *argv[])
{
    int fd = open("/dev/sda",O_RDONLY);
    std::cout << fd << std::endl;
   
    physicalDisk(fd);
    char * p;

    int a = sectorRead(fd,0,&p);
    if(a>=0){
        std::cout << "read success " << a << " bytes " << std::endl;
        sectorDump(p,a);
    }
    else{
        std::cout << "reading error" << std::endl;
    }



}

