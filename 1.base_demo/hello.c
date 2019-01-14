/*
 * Linux驱动的两种加载方式:1.直接编译进内核,在linux启动时加载
 *                      2.采用内核模块的方式,可动态加载与卸载
 * 编译进内核(这里只简要叙述):1.复制源码到响应目录下(可能是/drivers/char)
 *                        2.Kconfig添加相应内容
 *                        3.Makefile添加相应内容
 *                        4.make menuconfig 选择添加的模块
 *                        5.进行编译
 * 编译进模块:必须包含 module_init(your_init_func)  //模块初始化接口
 *                  module_exit(your_exit_func)  //模块卸载接口
 * 模块运行在内核态,不能使用用户态C库函数中的printf函数,而要使用printk函数打印调试信息。由于printk打印分8个等级，等级高的被打印到控制台上，
 * 而等级低的却输出到日志文件中,数越小等级越高。
 * 执行:cat /proc/sys/kernel/printk 可以看到四个数,分别表示
 *   控制台日志级别：优先级高于该值的消息将被打印至控制台
 *   默认的消息日志级别：将用该优先级来打印没有优先级的消息
 *   最低的控制台日志级别：控制台日志级别可被设置的最小值(最高优先级)
 *   默认的控制台日志级别：控制台日志级别的缺省值
 * 如果有需要可以修改
 * 不够打印级别的信息会被写到日志中可通过dmesg 命令来查看
 */
#include <linux/module.h>        // module_init  module_exit
#include <linux/init.h>            // __init   __exit

// 模块安装函数
static int __init chrdev_init(void)
{    
    printk(KERN_INFO "chrdev_init helloworld init\n");

    return 0;
}

// 模块卸载函数
static void __exit chrdev_exit(void)
{
    printk(KERN_INFO "chrdev_exit helloworld exit\n");
}

module_init(chrdev_init);
module_exit(chrdev_exit);

// MODULE_xxx这种宏作用是用来添加模块描述信息
MODULE_LICENSE("GPL");                // 描述模块的许可证
MODULE_AUTHOR("Jinjin");              // 描述模块的作者
MODULE_DESCRIPTION("module test");    // 描述模块的介绍信息
MODULE_ALIAS("alias xxx");            // 描述模块的别名信息