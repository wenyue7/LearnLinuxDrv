#==========================================================================================================================
# make 定义了很多默认变量，${MAKE} 就是预设的 make 这个命令的名称（或者路径）。make -p 可以查看所有预定义的变量的当前值。
#==========================================================================================================================
# $(MAKE) -C $(KERNELDIR) M=$(PWD) modules
# Use make M=dir to specify directory of external module to build
#==========================================================================================================================
# 注意:最关键的几个变量:CC,obj-m,KERNELDIR  例如:CC=arm-none-linux-gnueabi-gcc
#                                            obj-m:=smodule.o
#                                            KERNELDIR ?= /root/fgi/linux-4.5.2
#     其中:CC是编译器,obj-m为需要编译的目标模块,KERNELDIR为内核路径.注意在编写可加载模块前要有一个内核代码目录树.KERNEL的内核版本必须与运行的内
#     核版本一致,否则编译出的模块往往无法加载.
#==========================================================================================================================
# KERNELDIR 指的是内核库文件的路径，你的代码中使用的是内核提供的函数，而这些函数也是有具体实现的，在连接成一个内核模块时要说明这些库文件在哪里，方便
# 链接程序把它们连接成一个完成的模块。
# “？=”：如果这个KERNELDIR为空说明你没有指定内核库文件的路径，那么它就会给KERNELDIR赋值，因为顶层Makefile通过这个环境变量知道内核库文件在哪里。
# KERNELDIR=/lib/modules/$(shell uname -r)/build;
#     其中 shell uname -r 说的是调用shell里头的uname指令 你可以uname -r看看呢是什么，他表示的是内核版本号。一般来说我们构造内核树时，它把内核
#     库统一保存在/lib/modules/内核版本号/build目下。
#==========================================================================================================================
# KERNELRELEASE 是在内核源码的顶层 Makefile 中定义的一个变量，在第一次读取执行模块中的Makefile时，KERNELRELEASE 没有被定义，所以make将读取
# else之后的内容，如果make的目标是clean，直接执行clean操作，然后结束。当make的目标为all时，-C $(KERNELDIR)指明跳转到内核源码目录下读取那里的
# Makefile, M=$(PWD)表明然后返回到当前目录继续读入、执行模块中的的Makefile。当从内核源码目录返回时，KERNELRELEASE已被定义，kbuild也被启动去解析
# kbuild语法的语句，make将继续读取else之前的内容。else之前的内容为kbuild语法的语句，指明模块源码中各文件的依赖关系，以及要生成的目标模块名。
# param-objs := file1.o file2.o 表示param.o由file1.o与file2.o 连接生成，obj-m := param.o表示编译连接后将生成param.o模块。
#==========================================================================================================================
# 一个经典的编译一个linux设备驱动的Makefile代码，此Makefile可以完成对大部分驱动的编译，使用时只需要稍加修改就可以了。
# $(warning KERNELRELEASE=$(KERNELRELEASE))
# ifneq ($(KERNELRELEASE),)
# MYMOD := hello
# obj-m := $(MYMOD).o
#  
# else
# #KERNELDIR ?= /home/linux/linux3.14/
#  
# KERNELDIR ?= /lib/modules/$(shell uname -r)/build  
# PWD := $(shell pwd)
# 
# .PHONY: modules
# modules:
# 	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
# .PHONY: clean
# clean:
# 	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions Module* modules*
# 
# .PHONY: modules_install 
# endif


ifneq ($(KERNELRELEASE),)
MYMOD := hello
obj-m := $(MYMOD).o

else
#KERNELDIR ?= /home/linux/linux3.14/

KERNELDIR ?= /lib/modules/$(shell uname -r)/build  
PWD := $(shell pwd)

.PHONY: modules
modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
.PHONY: clean
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions Module* modules*

.PHONY: modules_install 
endif
