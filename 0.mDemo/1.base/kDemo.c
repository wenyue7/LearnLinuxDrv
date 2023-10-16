/*************************************************************************
    > File Name: kDemo.c
    > Author: LiHongjin
    > Mail: 872648180@qq.com 
    > Created Time: Fri Oct 13 16:02:51 2023
 ************************************************************************/

/* ============================================================================
 * Linux驱动的两种加载方式:
 *   1.直接编译进内核,在linux启动时加载
 *   2.采用内核模块的方式,可动态加载与卸载
 *
 * 编译进内核(这里只简要叙述):
 *   1.复制源码到相应目录下(可能是/drivers/char)
 *   2.Kconfig添加相应内容
 *   3.Makefile添加相应内容
 *   4.make menuconfig 选择添加的模块
 *   5.进行编译
 * 编译进模块，必须包含以下接口:
 *   module_init(your_init_func)  //模块初始化接口
 *   module_exit(your_exit_func)  //模块卸载接口
 * 模块运行在内核态,不能使用用户态C库函数中的printf函数,而要使用printk函数打印调试信息。由于printk打印分8个等级，等级高的被打印到控制台上，
 * 而等级低的却输出到日志文件中,数越小等级越高。
 * 执行:cat /proc/sys/kernel/printk 可以看到四个数,分别表示
 *   控制台日志级别：优先级高于该值的消息将被打印至控制台
 *   默认的消息日志级别：将用该优先级来打印没有优先级的消息
 *   最低的控制台日志级别：控制台日志级别可被设置的最小值(最高优先级)
 *   默认的控制台日志级别：控制台日志级别的缺省值
 * 如果有需要可以修改
 * 不够打印级别的信息会被写到日志中可通过dmesg 命令来查看
 *
 * ============================================================================
 * 内核态文件操作：
 * (参考博客：https://blog.csdn.net/STN_LCD/article/details/79088975)
 *
 * 1. 打开文件
 * strcut file* filp_open(const char* filename, int open_mode, int mode);
 *      filename： 表明要打开或创建文件的名称(包括路径部分)。在内核中打开的文件时需要注意打开的时机，很容易出现需要打开文件的驱动很早就
 *                 加载并打开文件，但需要打开的文件所在设备还不有挂载到文件系统中，而导致打开失败。
 *      open_mode： 文件的打开方式，其取值与标准库中的open相应参数类似，可以取O_CREAT,O_RDWR,O_RDONLY等。
 *      mode： 创建文件时使用，设置创建文件的读写权限，其它情况可以匆略设为0
 *      该函数返回strcut file*结构指针，供后继函数操作使用，该返回值用IS_ERR()来检验其有效性。
 *
 * 
 * 2.读写文件
 * kernel中文件的读写操作可以使用vfs_read()和vfs_write，在使用这两个函数前需要说明一下get_fs()和 set_fs()这两个函数。
 * vfs_read() vfs_write()两函数的原形如下：
 *      ssize_t vfs_read(struct file* filp, char __user* buffer, size_t len, loff_t* pos);
 *      ssize_t vfs_write(struct file* filp, const char __user* buffer, size_t len, loff_t* pos);
 * 注意这两个函数的第二个参数buffer，前面都有__user修饰符，这就要求这两个buffer指针都应该指向用空的内存，如果对该参数传递kernel空间的指针，
 * 这两个函数都会返回失败-EFAULT。但在Kernel中，我们一般不容易生成用户空间的指针，或者不方便独立使用用户空间内存。要使这两个读写函数使用
 * kernel空间的buffer指针也能正确工作，需要使用set_fs()函数或宏(set_fs()可能是宏定义)，如果为函数，其原形如下：
 *      void set_fs(mm_segment_t fs);
 * 该函数的作用是改变kernel对内存地址检查的处理方式，其实该函数的参数fs只有两个取值：USER_DS，KERNEL_DS，分别代表用户空间和内核空间，
 * 默认情况下，kernel取值为USER_DS，即对用户空间地址检查并做变换。那么要在这种对内存地址做检查变换的函数中使用内核空间地址，就需要使用
 * set_fs(KERNEL_DS)进行设置。get_fs()一般也可能是宏定义，它的作用是取得当前的设置，这两个函数的一般用法为：
 *      mm_segment_t old_fs;
 *      old_fs = get_fs();
 *      set_fs(KERNEL_DS);
 *      ...... //与内存有关的操作
 *      set_fs(old_fs);
 * 还有一些其它的内核函数也有用__user修饰的参数，在kernel中需要用kernel空间的内存代替时，都可以使用类似办法。
 * 使用vfs_read()和vfs_write()最后需要注意的一点是最后的参数loff_t * pos，pos所指向的值要初始化，表明从文件的什么地方开始读写。
 *
 *
 * 3.关闭读写文件
 *      int filp_close(struct file*filp, fl_owner_t id);
 * 该函数的使用很简单，第二个参数一般传递NULL值，也有用current->files作为实参的。
 *
 *
 * 使用以上函数的其它注意点：
 * 1. 其实Linux Kernel组成员不赞成在kernel中独立的读写文件(这样做可能会影响到策略和安全问题)，对内核需要的文件内容，最好由应用层配合完成。
 * 2. 在可加载的kernel module中使用这种方式读写文件可能使模块加载失败，原因是内核可能没有EXPORT你所需要的所有这些函数。
 * 3. 分析以上某些函数的参数可以看出，这些函数的正确运行需要依赖于进程环境，因此，有些函数不能在中断的handle或Kernel中不属于任可进程的代码中
 * 执行，否则可能出现崩溃，要避免这种情况发生，可以在kernel中创建内核线程，将这些函数放在线程环境下执行(创建内核线程的方式请参数kernel_thread()函数)。
 *
 * ============================================================================
 *
 */

#include <linux/init.h>         // __init   __exit
#include <linux/module.h>       // module_init  module_exit

//-- file opt
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>


static char *init_desc = "default init desc";
static char *exit_desc = "default exit desc";
static char *demo1_export_test = "this is export symbol from module kDemo_1_base";
// Linux 的 /proc/kallsyms 文件对应着内核符号表，它记录了符号以及符号所在的内存地址。
// 模块可以使用如下宏导出符号到内核符号表中，导出的符号可以被其他模块使用，只需要
// 使用前声明一下就可以。
// EXPORT_SYMBOL(<symbolName>)
// EXPORT_SYMBOL_GPL(<symbolName>)  // 只适用于包含GPL许可权的模块
EXPORT_SYMBOL(demo1_export_test);


// 模块加载函数
// 在Linux中，所有表示为__init 的函数如果直接编译进内核，在连接时都会放在.init.text
// 这个区段内
// 所有的__init 函数在在区段.initcall.init中还保留了一份函数指针，在初始化时，内核会
// 通过这些函数指针调用这些__init函数并在初始化完成后释放init区段（包括.init.text、
// .initcall.init等）的内存
// 除了函数以外，数据也可以被定义为__initdata，对于知识初始化阶段需要的数据，内核
// 在初始化完后，也可以释放他们占用的内存
// #define __init		__section(".init.text")
// #define __initdata	__section(".init.data")
// #define __exitdata	__section(".exit.data")
// #define __exit_call	__used __section(".exitcall.exit")
//
// 在Linux内核中，可以使用request_module(const char *fmt, ...)函数加载模块内核模块
static int __init demo_1_init(void)
{    
    // 可以使用 cat /dev/kmsg 实时查看打印
    printk(KERN_INFO "module %s init desc:%s\n", __func__, init_desc);

    return 0;
}

// 模块卸载函数
static void __exit demo_1_exit(void)
{
    printk(KERN_INFO "module %s exit desc:%s\n", __func__, exit_desc);

    return;
}

module_init(demo_1_init);
module_exit(demo_1_exit);

// 可以为模块定义一个参数，在加载模块时，用户可以像模块传递参数：
//     module_param(<paraName>, <paraType>, <para Read/write permissions>)
// 安装传参，如果有多个参数需要以空格进行分隔：
//     insmod/modprobe <module>.ko <paraName>=<paraValue>
// 如果不传递，参数将使用模块内定义的缺省值。如果模块被内置（编译进kernel），
// 就无法insmod了，但是bootloader可以在bootargs里以"<modName>.<paraName>=<value>"
// 的形式给该内置的模块传递参数
//
// 参数的类型可以是byte、short、ushort、int、uint、long、ulong、charp（字符指针）、
// bool、invbool（布尔的反），在模块被编译时会将 module_param 中声明的类型与变量
// 定义的类型进行比较，判断是否一致。
//
// 模块也可以拥有参数组：
// module_param_array(<arrayName>, <arrayType>, <arrayLen>, <para Read/write permissions>)
//
// 模块被加载之后，在 /sys/module/ 目录下将出现以此模块命名的目录。
// 当参数为0时，表示该参数不可读写，参数文件也不会导出到用户可访问的文件系统
// 当参数不为0时，此模块的目录下将出现parameters目录，其中包含一系列以参数命名的
// 文件节点，这些文件的权限就是传入module_param()的权限，文件的内容为参数的值
//
// 权限：
// #define S_IRWXUGO	(S_IRWXU|S_IRWXG|S_IRWXO)
// #define S_IALLUGO	(S_ISUID|S_ISGID|S_ISVTX|S_IRWXUGO)
// #define S_IRUGO		(S_IRUSR|S_IRGRP|S_IROTH)
// #define S_IWUGO		(S_IWUSR|S_IWGRP|S_IWOTH)
// #define S_IXUGO		(S_IXUSR|S_IXGRP|S_IXOTH)
module_param(init_desc, charp, S_IRUGO);
module_param(exit_desc, charp, S_IRUGO);


// 内核模块领域可接受的LICENSE包括 “GPL”、“GPL v2”、“GPL and additional rights”、
// “Dual BSD/GPL”、“Dual MPL/GPL”和“Proprietary”（关于模块是否可采用非GPL许可权，
// 如 Proprietary，这个在学术界是有争议的）
// 大多数情况下内核模块应该遵守GPL兼容许可权。Linux内核模块最常见的是使用GPL v2
MODULE_LICENSE("GPL v2");                       // 描述模块的许可证
// MODULE_xxx这种宏作用是用来添加模块描述信息（可选）
MODULE_AUTHOR("Lhj <872648180@qq.com>");        // 描述模块的作者
MODULE_DESCRIPTION("base demo for learning");   // 描述模块的介绍信息
MODULE_ALIAS("base demo");                      // 描述模块的别名信息
