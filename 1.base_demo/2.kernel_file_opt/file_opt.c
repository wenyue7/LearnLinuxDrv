/*
 * 参考文章: https://www.programmerall.com/article/7102881112/
 *
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
