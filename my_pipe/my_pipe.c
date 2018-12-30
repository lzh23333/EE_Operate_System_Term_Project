#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>


static const char* dev_name = "my_pipe";
static unsigned int dev_major = 55;

static int my_pipe_open(struct inode* inode, struct file* file){
    printk("open\n");
    return 0;
}

static ssize_t my_pipe_write(struct file *file, const char* __user *buffer){
    printk("write\n");
    return 0;
}

static struct file_operations my_pipe_fo = {
    .owner = THIS_MODULE,
    .open  = my_pipe_open,
    .write = my_pipe_write,
};

static int my_pipe_init(void){
    int dev_id = register_chrdev(dev_major, dev_name, &my_pipe_fo);
    printk("init\n");
    if(dev_id < 0){
        printk("error\n");
    }
    return 0;
}

static void my_pipe_exit(void){
    printk("exit\n");
    unregister_chrdev(dev_major,dev_name);
}

module_init(my_pipe_init);
module_exit(my_pipe_exit);

MODULE_AUTHOR("Robin");
MODULE_LICENSE("GPL");

