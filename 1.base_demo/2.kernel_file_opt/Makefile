obj-m += file_opt.o 
 
KDIR ?= /lib/modules/$(shell uname -r)/build  
# KDIR := /home/chorm/workspace/my_android_src/kernel
PWD ?= $(shell pwd)
 
all:
	make -C $(KDIR) M=$(PWD) modules
        
clean:
