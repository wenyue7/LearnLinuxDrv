# 字符设备

```c
struct cdev {
    struct kobject kobj;
    struct module *owner;
    const struct file_operations *ops;
    struct list_head list;
    dev_t dev;                   /* 设备号 32位，其中12位为主设备号，20位为次设备号 */
    unsigned int count;
} __randomize_layout;
```



## 1. 设备号

### **主设备号和次设备号**

原则上，主设备号标识设备对应的驱动程序，但现代的 Linux 内核也允许多个驱动程序共享主设备号。

次设备号由内核使用，标识设备文件所指的设备，可以通过次设备号获得一个指向内核设备的指针，也可以将次设备号当作设备本地数据的索引。



**获得 dev_t 的主设备号或次设备号：**

```c
MAJOR(dev_t dev);  // 获得主设备号
MINOR(dev_t dev);  // 获得从设备号
```



**将主设备号和次设备号转换成 dev_t ：**

```c
MKDEV(int major, int minor);
```



### **分配和释放设备号：**

在建立一个字符设备之前，我们的驱动程序首先要做的事情就是获得一个或者多个设备编号。使用接口在 <linux/ fs.h> 中声明：

```c
int register_chardev_region(dev_t first, unsigned int count, char *name);	
```

*参数说明：*

first: 要分配的设备编号范围的起始值，常被置为0

count: 所请求的连续设备编号的个数，如果count非常大，则所请求的范围可能和下一个主设备号重叠，但只要我们所请求的编号范围是可用的，就不会带来任何问题

name: 和该编号范围关联的设备名称，它将出现在 /proc/devices 和 sysfs 中



如果已知要使用的起始设备号，使用 register_chardev_region ，如果不知道，使用：

```c
int alloc_chardev_region(dev_t *dev, unsigned int firstminor, unsigned int count, char *name);
```

*参数说明：*

dev: 用于输出的参数，在调用成功后保存已分配范围的第一个编号

firseminor: 要使用的被请求的第一个次设备号，通常是0

count、name参数与 register_chardev_region 一样



释放设备号：

```c
void unregister_chrdev_region(dev_t first, unsigned int count);
```



#### ***register_chardev_region 和 alloc_chardev_region***：

如果使用指定的方式（register_chardev_region）：一旦驱动程序被广泛使用，可能造成冲突和麻烦。

动态分配的方式（unregister_chrdev_region）：由于分配的主设备号不能始终保持一致，所以无法预先创建设备节点。但该问题有办法解决。



加载一个使用动态主设备号的设备驱动程序，对insmode的调用可以换成一个简单的脚本，该脚本在调用insmode之后读取/proc/devices以获得新分配的主设备号，然后创建对应的设备文件。动态分配主设备号的情况下，要加载这类驱动程序模块的脚本，可以使用awk这类工具从/proc/devices中获取信息，并在/dev目录中创建设备文件。

加载脚本：

```shell
#!/bin/sh

module="scull"
device="scull"
mode="664"

# Group: since distributions do it differently, look for wheel or use staff.
if grep -q '^staff:' /etc/group; then
    group="staff"
else
    group="wheel"
fi

# Invoke insmod with all provided arguments. Use a pathname, as insmod doesn't
# look in . by default.
/sbin/insmod ./$module.ko $* || exit 1

# Retrieve the device major number.
major=$(awk "\$2==\"$module\" {print \$1}" /proc/devices)

# Remove stale nodes and replace them, then give gid and perms.

# Usually a load script is shorter; scull just has a lot of associated devices.

rm -f /dev/${device}[0-3]
mknod /dev/${device}0 c $major 0
mknod /dev/${device}1 c $major 1
mknod /dev/${device}2 c $major 2
mknod /dev/${device}3 c $major 3
ln -sf ${device}0 /dev/${device}
chgrp $group /dev/${device}[0-3] 
chmod $mode  /dev/${device}[0-3]

rm -f /dev/${device}pipe[0-3]
mknod /dev/${device}pipe0 c $major 4
mknod /dev/${device}pipe1 c $major 5
mknod /dev/${device}pipe2 c $major 6
mknod /dev/${device}pipe3 c $major 7
ln -sf ${device}pipe0 /dev/${device}pipe
chgrp $group /dev/${device}pipe[0-3] 
chmod $mode  /dev/${device}pipe[0-3]

rm -f /dev/${device}single
mknod /dev/${device}single  c $major 8
chgrp $group /dev/${device}single
chmod $mode  /dev/${device}single

rm -f /dev/${device}uid
mknod /dev/${device}uid   c $major 9
chgrp $group /dev/${device}uid
chmod $mode  /dev/${device}uid

rm -f /dev/${device}wuid
mknod /dev/${device}wuid  c $major 10
chgrp $group /dev/${device}wuid
chmod $mode  /dev/${device}wuid

rm -f /dev/${device}priv
mknod /dev/${device}priv  c $major 11
chgrp $group /dev/${device}priv
chmod $mode  /dev/${device}priv

```

卸载脚本：

```shell
#!/bin/sh

module="scull"
device="scull"

# Invoke rmmod with all provided arguments.
/sbin/rmmod $module $* || exit 1

# Remove stale nodes.

rm -f /dev/${device} /dev/${device}[0-3] 
rm -f /dev/${device}priv
rm -f /dev/${device}pipe /dev/${device}pipe[0-3]
rm -f /dev/${device}single
rm -f /dev/${device}uid
rm -f /dev/${device}wuid

```





## 2. 字符设备注册和注销

**申请空间 指定文件操作/方法**

```c
struct cdev *my_cdev = cedv_alloc();
```

**初始化已分配到的结构体**

```c
void cdev_init(struct cdev *cdev, struct file_operations *fops);
my_cdev->owner = THIS_MODULE;  // 所有者
my_cdev->ops = &my_fops;
```

**注册设备到内核** 

```c
int cdev_add(struct cdev *dev, dev_t num, unsigned int count);
```

*参数说明：*

dev:  cdev结构体

num: 该设备对应的地一个设备编号

count: 应该和该设备关联的设备编号的数量

注意：可能存在注册失败的问题，因此要关注返回值！



**从内核中注销**

```c
void cdev_del(strut cdev *dev);
```



**内核中的老接口**

```c
int register_chrdev(unsigned int major, const char *name, struct file_operations *fops);  // 注册
int unregister_chrdev(unsigned int major, const char *name); // 注销
```



## 3. 重要的数据结构

大部分基本的驱动程序操作涉及到三个重要的内核数据结构，分别是 file_operations、file和inode

### **文件操作**

```c
struct file_operations {
    struct module *owner;
    loff_t (*llseek) (struct file *, loff_t, int);
    ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
    ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
    ssize_t (*read_iter) (struct kiocb *, struct iov_iter *);
    ssize_t (*write_iter) (struct kiocb *, struct iov_iter *);
    int (*iopoll)(struct kiocb *kiocb, bool spin);
    int (*iterate) (struct file *, struct dir_context *);
    int (*iterate_shared) (struct file *, struct dir_context *);
    __poll_t (*poll) (struct file *, struct poll_table_struct *); 
    long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
    long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
    int (*mmap) (struct file *, struct vm_area_struct *);
    unsigned long mmap_supported_flags;
    int (*open) (struct inode *, struct file *);
    int (*flush) (struct file *, fl_owner_t id); 
    int (*release) (struct inode *, struct file *);
    int (*fsync) (struct file *, loff_t, loff_t, int datasync);
    int (*fasync) (int, struct file *, int);
    int (*lock) (struct file *, int, struct file_lock *);
    ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int); 
    unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
    int (*check_flags)(int);
    int (*flock) (struct file *, int, struct file_lock *);
    ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *, size_t, unsigned int);
    ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *, size_t, unsigned int);
    int (*setlease)(struct file *, long, struct file_lock **, void **);
    long (*fallocate)(struct file *file, int mode, loff_t offset, 
              loff_t len);
    void (*show_fdinfo)(struct seq_file *m, struct file *f);
#ifndef CONFIG_MMU
    unsigned (*mmap_capabilities)(struct file *);
#endif
    ssize_t (*copy_file_range)(struct file *, loff_t, struct file *,
            loff_t, size_t, unsigned int);  
    loff_t (*remap_file_range)(struct file *file_in, loff_t pos_in,
                   struct file *file_out, loff_t pos_out,
                   loff_t len, unsigned int remap_flags);
    int (*fadvise)(struct file *, loff_t, loff_t, int);
} __randomize_layout;

```



### file结构

file结构与用户空间的FILE没有任何关联，FILE在C库中定义，且不会出现在内核空间

file结构代表一个打开的文件（它不仅仅限定于设备驱动程序，系统中每个打开的文件在内核空间都有一个file结构）

file指的是结构本身，filp则是指向该结构的指针

```c
struct file {
    union {
        struct llist_node   fu_llist;
        struct rcu_head     fu_rcuhead;
    } f_u;
    struct path     f_path;
    struct inode        *f_inode;   /* cached value */
    const struct file_operations    *f_op;

    /*
     * Protects f_ep, f_flags.
     * Must not be taken from IRQ context.
     */
    spinlock_t      f_lock;
    enum rw_hint        f_write_hint;
    atomic_long_t       f_count;
    unsigned int        f_flags;
    fmode_t         f_mode;
    struct mutex        f_pos_lock;
    loff_t          f_pos;
    struct fown_struct  f_owner;
    const struct cred   *f_cred;
    struct file_ra_state    f_ra;

    u64         f_version;
#ifdef CONFIG_SECURITY
    void            *f_security;
#endif
    /* needed for tty driver, and maybe others */
    void            *private_data;

#ifdef CONFIG_EPOLL
    /* Used by fs/eventpoll.c to link all the hooks to this file */
    struct hlist_head   *f_ep;
#endif /* #ifdef CONFIG_EPOLL */
    struct address_space    *f_mapping;
    errseq_t        f_wb_err;
    errseq_t        f_sb_err; /* for syncfs */
} __randomize_layout
  __attribute__((aligned(4)));  /* lest something weird decides that 2 is OK */

```



### **inode结构**

内核用inode结构在内部表示文件，因此它和file结构不同，file表示打开的文件描述符。对于单个文件，可能会有**许多个**表示打开的文件描述符(file)，但他们都指向**单个**inode结构。

inode结构中包含了大量有关文件的信息。但通常只有下面两个字段对编写驱动程序代码有用：

**dev_t i_rdev**

​		对表示设备文件的inode结构，该字段包含了真正的设备编号

**struct cdev *i_cdev**

​		struct cdev 是表示字符设备在内核中的结构。当inode指向一个字符设备文件时，该字段包含了指向struct cdev结构体的指针

为了鼓励编写可移植性更强的代码，建议使用如下方法从 inode 中获取主设备号和从设备号

```c
unsigned int iminor(struct inode *inode);
unsigned int iminor(struct inode *inode);
```





## 4. open 和 release

**open方法提供给驱动程序以初始化的能力，从而为以后的操作做准备，open大多数情况下应该完成如下工作：**

​		检测设备特定的错误（诸如设备未就绪或类似的硬件问题）

​		如果设备是首次打开，则对其进行初始化

​		如果有必要，更新 f_op 指针

​		分配并填写置于 filp->private_data 里的数据

原型：

```c
int (*open)(struct inode *inoed, struct file *filep);
```

其中 inode 参数在 i_cdev 字段中包含了我们所需要的信息，即**我们先前设置的 cdev 结构体**



这里一个函数可能需要使用到：

```c
container_of(pointer, container_type, container_field);  // 定义在<linux/kernel.h>中
```

该函数根据指定结构体(container_type)和其内部的成员(container_field)，计算入参(pointer)对应的结构体(container_type)实例入口，这里的入参(pointer)也是结构体(container_type)的成员(container_field)

当我们创建一个自己使用的结构体时，如果在结构体内部放了 cdev，则可以根据结构体内部放的 cdev设备 (open函数中的 inode->i_cdev) 计算出我们自己创建的结构体的入口



**release 方法与 open方法相反，该方法应该完成以下工作：**

​		释放由 open 分配、保存在 filp->private_data 中的所有内容

​		在最后一次关闭操作时关闭设备



**如何保证关闭一个设备文件的次数不会比打开的次数多？**

内核对每个 file 结构都会记录被引用的次数。无论是 fork 还是 dup，都不会创建新的数据结构（仅由 open 创建），他们只是增加已有结构体中的计数。只有在 file 结构体归零时，close系统调用才会执行 release 方法，这只在删除这个结构体时才会发生，因此release和close的系统调用间的关系保证了对于每次open驱动程序只会看到对应的一次release调用。



## 5. read 和 write

**read 拷贝数据到应用程序空间，write拷贝数据到内核空间，原型：**

```c
ssize_t read(struct file *filp, char __user *buff, size_t count, loff_t *offp);
ssize_t write(struct file *filp, const char __user *buff, size_t count, loff_t *offp);
```

filp: 文件指针

count: 数据传输长度

buff: 指向用户空间的缓冲区

offp: 指向"long offset type（长偏移量类型）"对象的指针，这个对象指明用户在文件中进行存取操作的位置

出错时，read 和 write 方法都返回一个负值。大于等于0的返回值告诉调用程序成功传输了多少字节。如果在正确传输部分数据之后发生了错误，则返回值是成功传输的字节数，这个错误只能在下一次函数调用时才会得到报告。

尽管内核函数通过返回负值来表示错误，但运行在用户空间的程序看到的始终是返回-1。为了找到出错原因，用户空间爱呢的程序必须访问 errno 变量。



**read 方法**

​		如果返回值等于传递给read系统调用的count参数，则说明所请求的字节数传输成功完成了

​		如果返回值是正的，但是比count小，说明只有部分数据成功传输。这种情况因设备的不同可能有许多原因。大部分情况下程序会重新读书据。例如，如果用fread函数读数据，这个库函数就会不断调用系统调用，直到所请求的数据传输完毕为止

​		如果返回值为 0，表示已经到达了文件尾

​		负值意味着发生了错误，该值指明了发生了什么错误，错误码在<linux/errno.h>中定义



**write 方法**

​		如果返回值等于count，则完成了所请求数目的字节传送

​		如果返回值是正的，但小于 count， 则只传输了部分数据。程序很可能再次写入余下的数据

​		如果值为0,意味着什么也没写入。这个结果不是错误。类似read方法，标准库会重复调用write

​		负值意味着发生了错误，类似read方法



**read 和 write 方法的 buff 参数是用户空间的指针。因此，内核代码不能直接引用其中的内容，原因如下：**

​		由于驱动程序所运行的架构不同或内核配置不同，在内核模式中运行时，用户空间的指针可能是无效的。该地址可能根本无法被映射到内核空间，或者可能指向某些随机数据

​		即使该指针在内核空间中代表相同的东西，但用户空间的内存是分页的，而在系统接口被调用时，涉及到的内存可能根本就不在RAM中。对用户空间内存的直接引用将导致页错误，而这对于内核代码来说是不允许发生的事情。其结果可能是一个"oops"，他将导致调用该系统接口的进程死亡。

​		我们讨论的指针可能由用户程序提供，而该程序可能存在缺陷或者是个恶意程序。如果我们的驱动程序盲目引用用户提供的指针，将导致系统出现打开的后门，从而允许用户空间程序随意访问或覆盖系统中的内存。



**从内核空间访问用户空间数据的接口：**

```c
unsigned long copy_to_user(void __user *to, const void *from, unsigned long count);
unsigned long copy_from_user(void *to, const void __user *from, unsigned long count);
```

这两个内核函数的作用并不限于数据传输，他们还检查用户空间的指针是否有效。如果指针无效，就不会进行拷贝；另外如果在拷贝过程中遇到无效地址，则仅仅会复制部分数据。在这两种情况下返回值是还需要拷贝的内存数量值。



如果不需要检查用户空间的指针，或者已经检查过，则可以调用：

```c
"__copy_to_user" 和 "__copy_from_user"
```





__user:

__user 是一个宏，表明其后的指针指向用户空间，实际上更多的充当了代码自注释的功能



内核空间虽然可以访问用户空间的缓冲区，但是在访问之前，一般需要先检查其合法性，通过 access_ok(type, addr, size)进行判断，以确定传入的缓冲区的确属于用户空间，例如：

```
if(!access_ok(VERIFY_WRITE, buf, count))
	return -EFAULT;
	
if(__put_user(inb(i),tmp) < 0)
	return -EFAULT;
```



如果要复制的内存是简单类型，如：char、int、long等，则可以使用简单的 put_user() (内核->用户)和 get_user()(用户->内核)

__put_user()和put_user()的区别在于前者不进行类似access_ok()的检查，后者会进行这一检查。



## 6. demo

ch6 文件夹是送宝华的《Linux设备驱动开发详解》的第六章的demo

scull 是《设备驱动程序》的demo