/*
 * 参考文章: https://www.programmerall.com/article/7102881112/
 *
 * ============================================================================
 * 内核态文件操作：
 * (参考博客：https://blog.csdn.net/STN_LCD/article/details/79088975)
 *
 * 1. 打开文件
 * strcut file* filp_open(const char* filename, int open_mode, int mode);
 *   filename： 表明要打开或创建文件的名称(包括路径部分)。在内核中打开的文件时
 *              需要注意打开的时机，很容易出现需要打开文件的驱动很早就加载并打开
 *              文件，但需要打开的文件所在设备还不有挂载到文件系统中，而导致打开
 *              失败。
 *   open_mode：文件的打开方式，其取值与标准库中的open相应参数类似，可以取
 *              O_CREAT,O_RDWR,O_RDONLY等。
 *   mode：创建文件时使用，设置创建文件的读写权限，其它情况可以匆略设为0
 *   ret：该函数返回strcut file*结构指针，供后继函数操作使用，该返回值用IS_ERR()
 *        来检验其有效性。
 *
 * 2.读写文件
 * kernel中文件的读写操作可以使用vfs_read()和vfs_write，在使用这两个函数前需要
 * 说明一下get_fs()和 set_fs()这两个函数。
 * vfs_read() vfs_write()两函数的原形如下：
 *   ssize_t vfs_read(struct file* filp, char __user* buffer, size_t len, loff_t* pos);
 *   ssize_t vfs_write(struct file* filp, const char __user* buffer, size_t len, loff_t* pos);
 * 注意这两个函数的第二个参数buffer，前面都有__user修饰符，这就要求这两个buffer
 * 指针都应该指向用空的内存，如果对该参数传递kernel空间的指针，这两个函数都会返回
 * 失败-EFAULT。但在Kernel中，我们一般不容易生成用户空间的指针，或者不方便独立
 * 使用用户空间内存。要使这两个读写函数使用kernel空间的buffer指针也能正确工作，
 * 需要使用set_fs()函数或宏(set_fs()可能是宏定义)，如果为函数，其原形如下：
 *   void set_fs(mm_segment_t fs);
 * 该函数的作用是改变kernel对内存地址检查的处理方式，其实该函数的参数fs只有两个
 * 取值：USER_DS，KERNEL_DS，分别代表用户空间和内核空间，
 * 默认情况下，kernel取值为USER_DS，即对用户空间地址检查并做变换。那么要在这种
 * 对内存地址做检查变换的函数中使用内核空间地址，就需要使用set_fs(KERNEL_DS)
 * 进行设置。get_fs()一般也可能是宏定义，它的作用是取得当前的设置，这两个函数的
 * 一般用法为：
 *   mm_segment_t old_fs;
 *   old_fs = get_fs();
 *   set_fs(KERNEL_DS);
 *   ...... //与内存有关的操作
 *   set_fs(old_fs);
 * 还有一些其它的内核函数也有用__user修饰的参数，在kernel中需要用kernel空间的内存
 * 代替时，都可以使用类似办法。
 * 使用vfs_read()和vfs_write()最后需要注意的一点是最后的参数loff_t * pos，pos所
 * 指向的值要初始化，表明从文件的什么地方开始读写。
 *
 * 3.关闭读写文件
 *   int filp_close(struct file*filp, fl_owner_t id);
 * 该函数的使用很简单，第二个参数一般传递NULL值，也有用current->files作为实参的。
 *
 * 使用以上函数的其它注意点：
 * 1. 其实Linux Kernel组成员不赞成在kernel中独立的读写文件(这样做可能会影响到策略
 *    和安全问题)，对内核需要的文件内容，最好由应用层配合完成。
 * 2. 在可加载的kernel module中使用这种方式读写文件可能使模块加载失败，原因是内核
 *    可能没有EXPORT你所需要的所有这些函数。
 * 3. 分析以上某些函数的参数可以看出，这些函数的正确运行需要依赖于进程环境，因此，
 *    有些函数不能在中断的handle或Kernel中不属于任可进程的代码中执行，否则可能出现
 *    崩溃，要避免这种情况发生，可以在kernel中创建内核线程，将这些函数放在线程
 *    环境下执行(创建内核线程的方式请参数kernel_thread()函数)。
 *
 * ============================================================================
 *
 */


#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

static struct file *fp;
static struct file *wfp;

static int __init init(void)
{
    printk("%s()\n", __FUNCTION__);
#define FN "/sdcard/wanna"
    /*
       filp_open() is an asynchronous execution function. It will open the specified file asynchronously.
       If the function is ended without doing other things after opening the file, it is very likely that the following printing will not be displayed.
       */
    fp = filp_open("/sdcard/wanna", O_RDONLY, 0); //The file mode of parameter 3 has little effect on reading files.
    printk("fs file address:0x%p\n", fp);
    msleep(100);
    if(IS_ERR(fp))//Use this IS_ERR() to check whether the pointer is valid, but you cannot directly determine whether the pointer is NULL.
    {
        printk("cannot open fs.\n");
        goto FS_END;
    }

    char *out_file_name;
    const int of_len = strlen(FN) + 5;
    out_file_name = kmalloc(of_len, GFP_KERNEL);
    if(out_file_name == NULL)
    {
        printk("cannot malloc.\n");
        goto FS_END;
    }
    memset(out_file_name, 0, of_len);

    snprintf(out_file_name, of_len, "%s%s", FN, "_out");
    printk("out_file_name:%s\n", out_file_name);
    wfp = filp_open(out_file_name, O_WRONLY|O_CREAT, 0666);
    msleep(100);
    if(IS_ERR(wfp))
    {
        printk("cannot open the write file.\n");
        wfp = NULL;
    }

    mm_segment_t old_fs;
    old_fs = get_fs();
    set_fs(KERNEL_DS);

    int size = 0;
    char rbuf[6];
    loff_t pos = 0;
    loff_t wpos = 0;
    while(1)
    {
        memset(rbuf, 0, 6);
        /*
           Parameter 2 requires the memory address of the __user space,
           If you want to use the array created in the kernel directly,
           The file system state should be switched to KERNEL_DS state before use. That is, the above set_fs(KERNEL_DS) call.

           Parameter 3 is the position of the reading pointer. For ordinary files, it must pass in a valid loff_t pointer to realize the "breakpoint reading" function.
           */
        size = vfs_read(fp, rbuf, 3, &pos);
        printk("read ret:%d, pos:%ld\n", size, pos);
        if(size < 1)
        {
            printk("read end.\n");
            break;
        }

        printk("\t%s\n", rbuf);

        if(wfp)
        {
            //Copy the content of the file that was previously read to another file.
            size = vfs_write(wfp, rbuf, size, &wpos);
            printk("write ret:%d, pos:%ld\n", size, wpos);
        }
    }
    set_fs(old_fs);

    msleep(50);

FS_END:

    return 0;
}

static void __exit exit(void)
{
    if(!IS_ERR(fp))
    {
        printk("closing fs file.\n");
        int ret = filp_close(fp, NULL);
        printk("close ret:%d\n", ret);
    }

    if(wfp && !IS_ERR(wfp))
    {
        printk("closing wfp.\n");
        int ret = filp_close(wfp, 0);
        printk("close wfp ret:%d\n", ret);
    }
    msleep(100);
}

module_init(init);
module_exit(exit);
MODULE_LICENSE("GPL");
