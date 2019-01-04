#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define BUFF_SIZE  1024   //buffer size of char device

struct pipe_struct{
    char buffer[BUFF_SIZE];
    struct semaphore sem;
    int wr;
    int rd;
    int data_size;

};

struct pipe_struct my_pipe_device;  


static const char* my_dev_name = "my_pipe";
static unsigned int pipe_major = 55;    //主设备号
static unsigned int pipe_minor = 0;     //次设备号
static unsigned int cdev_count = 1;              //连续两个设备
static dev_t pipe_dev;


static ssize_t my_pipe_read(struct file* f, char* , size_t size, loff_t* ppos);
static ssize_t my_pipe_write(struct file* f, const char*, size_t size, loff_t* ppos);
static int my_pipe_open(struct inode* inode, struct file* f);
static int my_pipe_release(struct inode* inode, struct file *f);


static struct file_operations my_pipe_fo = {    //文件操作结构
    .owner = THIS_MODULE,
    .open  = my_pipe_open,
    .write = my_pipe_write,
    .read  = my_pipe_read,
    .release = my_pipe_release,
    
};

static int __init my_pipe_init(void){   //字符设备注册
    /*
    int res;
    printk(KERN_INFO "my_pipe init \n");
    if(pipe_major){
        res = register_chrdev_region(MKDEV(pipe_major,pipe_minor),cdev_count,my_dev_name);
    }
    else{
        res = alloc_chrdev_region(&pipe_dev, pipe_minor, cdev_count, my_dev_name);
        pipe_major = MAJOR(pipe_dev);
    }

    //若没有分配到主设备号
    if(res < 0){
        printk(KERN_WARNING"cannot register my_pipe major!\n");
        return res;
    }
    
    my_pipe_device = kmalloc(sizeof(struct pipe_struct),GFP_KERNEL);
    //字符设备结构初始化
    my_pipe_device->cdev = *cdev_alloc();
    cdev_init(&(my_pipe_device->cdev), &my_pipe_fo);
    my_pipe_device->cdev.owner = THIS_MODULE;
    my_pipe_device->cdev.ops = &my_pipe_fo;

    my_pipe_device->wr = 0;
    my_pipe_device->rd = 0;
    my_pipe_device->data_size = 0;
    sema_init(&my_pipe_device->sem,1);

    res = cdev_add(&(my_pipe_device->cdev),pipe_dev,cdev_count);
    

    if(res){
        printk(KERN_WARNING"cannot register my_pipe chrdev!\n");
        cdev_del(&(my_pipe_device->cdev));
        return -ENOMEM;
    }

    */
    int dev_id = register_chrdev(pipe_major,my_dev_name,&my_pipe_fo);
    my_pipe_device.wr = 0;
    my_pipe_device.rd = 0;
    my_pipe_device.data_size = 0;
    sema_init(&my_pipe_device.sem,1);
    return 0;
}

static void my_pipe_exit(void){ //字符设备删除
    printk(KERN_INFO"my_pipe exit\n");
    
    /*cdev_del(&(my_pipe_device->cdev));
    unregister_chrdev_region(MKDEV(pipe_major,pipe_minor),cdev_count);
    */
   unregister_chrdev(pipe_major,my_dev_name);
}



static int my_pipe_open(struct inode* inode, struct file* filp){
    
    /*struct pipe_struct* p_s;
    p_s = container_of(inode->i_cdev,struct pipe_struct,cdev);
    
    */
    
    printk(KERN_INFO"open\n");

    return 0;
}

static int my_pipe_release(struct inode* inode, struct file* f){
    printk(KERN_INFO "my_pipe release\n");
    return 0;
}

static ssize_t my_pipe_write(struct file *filp, const char* __user data,size_t size, loff_t * ppos){
    struct pipe_struct* pipe_device = &my_pipe_device;//filp->private_data;
    int count = 0;                          //写入数据个数
    int left_space = BUFF_SIZE - pipe_device->data_size; //剩余可写空间
    int fir;
    int sec;
    
    printk(KERN_INFO"write\n");

    down(&pipe_device->sem);
    count = (size <= left_space) ? size : left_space;   //计算需要写入的字节数
    
    if(pipe_device->wr + count < BUFF_SIZE){
        if(copy_from_user(pipe_device->buffer + pipe_device->wr,data,count) != 0){      //将数据写入
            printk(KERN_WARNING"copy from user error!\n");
            up(&pipe_device->sem);
            return -EFAULT;
        }
        else{
            printk(KERN_INFO"copy from user successfully!\n");
        }
        
    }
    else{//需要写入两次
        fir = BUFF_SIZE - pipe_device->wr;           //第一次写入个数
        sec = count - fir;              //第二次

        if(copy_from_user(pipe_device->buffer+pipe_device->wr,data,fir) != 0){
            printk(KERN_WARNING"copy from user first error!\n");
            up(&pipe_device->sem);
            return -EFAULT;
        }

        if(copy_from_user(pipe_device->buffer,data+fir,sec) != 0){   //若第二次写入失败，返回已写入数据量
            printk(KERN_WARNING"copy from user second error!\n");
            pipe_device->wr = 0;
            pipe_device->data_size += fir;
            up(&pipe_device->sem);
            return fir;
        }
    }

    pipe_device->wr = (pipe_device->wr + count) % BUFF_SIZE;                        //移动wr位置
    pipe_device->data_size += count;
    up(&pipe_device->sem);                               
    return count;
}

static ssize_t my_pipe_read(struct file *f, char* __user data, size_t size, loff_t* ppos){
    struct pipe_struct* pipe_device = &my_pipe_device;//f->private_data;
    int count;
    int fir;
    int sec;
    
    printk(KERN_INFO "read.\n");

    down(&pipe_device->sem);
    count = (size <= pipe_device->data_size) ? size : pipe_device->data_size;     //需要读取的字节数
    if(pipe_device->rd + count <= BUFF_SIZE){
        if(copy_to_user(data,pipe_device->buffer+pipe_device->rd,count) != 0){
            printk(KERN_WARNING "copy to user error.\n");
            up(&pipe_device->sem);
            return -EFAULT;
        }
    }
    else {
        int fir = BUFF_SIZE - pipe_device->rd;
        int sec = count - fir;

        if(copy_to_user(data, pipe_device->buffer+pipe_device->rd, fir) != 0){
            printk(KERN_WARNING "copy to user 1st error\n");
            up(&pipe_device->sem);
            return -EFAULT;
        }
        
        if(copy_to_user(data+fir,pipe_device->buffer,sec) != 0){ //若第二次失败，返回已读出的数据量
            printk(KERN_WARNING "copy to user 2rd error.\n");
            pipe_device->data_size -= fir;
            pipe_device->rd = 0;
            up(&pipe_device->sem);
            return fir;
        }
    }

    pipe_device->data_size -= count;
    pipe_device->rd = (pipe_device->rd + count) % BUFF_SIZE;
    up(&pipe_device->sem);
    return count;
}

module_init(my_pipe_init);
module_exit(my_pipe_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Robin");




