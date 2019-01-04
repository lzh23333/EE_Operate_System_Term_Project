
all:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
		rm -f modules.order

obj-m += my_pipe.o #加载到module的编译链中，内核会编译生成出来ko文件，作为一个模块
