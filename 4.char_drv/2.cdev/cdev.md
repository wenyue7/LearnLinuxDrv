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


## 设备号

通过查看 cat /proc/devices 文件可以得到系统中注册的设备，第一列为主设备号，第二列为设备名称  

### 主设备号和次设备号

字符设备通过文件系统中的设备名称进行访问。这些名称称为特殊文件或设备文件或简称为文件系统树的节点，它们通常位于 /dev目录中。  

原则上，**主设备号**标识设备对应的驱动程序，但现代的 Linux 内核也允许多个驱动程序共享主设备号。  
**次设备号**由内核使用，标识设备文件所指的设备，可以通过次设备号获得一个指向内核设备的指针，也可以将次设备号当作设备本地数据的索引。  
<font color=red>user < - - >dev file<-- [主设备号] --> driver <-- [次设备号] --> dev</font>

获得 dev_t 的主设备号或次设备号：
```c
MAJOR(dev_t dev);  // 获得主设备号
MINOR(dev_t dev);  // 获得从设备号
```  

将主设备号和次设备号转换成 dev_t 类型：
```c
dev_t devNo = MKDEV(int major, int minor);
```  

ls -l输出的第一列中的“c”标识表示字符设备，块设备由“b”标识。本章的重点是字符设备，但以下大部分信息也适用于块设备。

使用`ls -l`命令，可以在修改日期之前看到两个数字（以逗号分隔，其有时候显示文件长度），这些数字是特定设备的主设备号和次设备号。  
以下列表显示了系统上出现的一些设备。它们的主设备号是 1、4、7 和 10，次设备号是 1、3、5、64、65 和 129。
```shell
 crw-rw-rw- 1 root   root    1, 3   Feb 23 1999  null
 crw------- 1 root   root   10, 1   Feb 23 1999  psaux
 crw------- 1 rubini tty     4, 1   Aug 16 22:22 tty1
 crw-rw-rw- 1 root   dialout 4, 64  Jun 30 11:19 ttyS0
 crw-rw-rw- 1 root   dialout 4, 65  Aug 16 00:00 ttyS1
 crw------- 1 root   sys     7, 1   Feb 23 1999  vcs1
 crw------- 1 root   sys     7, 129 Feb 23 1999  vcsa1
 crw-rw-rw- 1 root   root    1, 5   Feb 23 1999  zero
```
**主设备号可以理解为驱动程序的索引**，与设备的驱动程序关联。例如，/dev/null和 /dev/zero均由驱动程序 1 管理，而虚拟控制台和串行终端由驱动程序 4 管理；同样，vcs1和 vcsa1设备均由驱动程序 7 管理。打开设备时，内核在使用主设备号将执行操作分派给适当的驱动程序。

次设备号仅由主设备号指定的驱动程序使用；内核的其他部分不使用它，而只是将其传递给驱动程序。一个驱动程序控制多个设备是很常见的（如列表所示）；次要编号为驱动程序提供了区分它们的方法。

<font color=red>
根据主设备号，操作系统能选择正确的驱动程序。  
根据次设备号，驱动程序能知道要操作的设备。  
</font>

不过，2.4 版内核引入了一项新的（可选）功能，即设备文件系统或devfs。如果使用该文件系统，设备文件的管理会变得简单，并且会有很大不同；另一方面，新的文件系统带来了一些用户可见的不兼容性，并且在我们撰写本文时，系统分销商尚未选择它作为默认功能。前面的描述和下面有关添加新驱动程序和特殊文件的说明假设devfs不存在。

当devfs没有被使用时，向系统添加一个新的驱动程序意味着为其分配一个主设备号。应在驱动程序（模块）初始化时通过调用以下函数进行分配，该函数在 中定义<linux/fs.h>：
```c
// include/linux/fs.h
int register_chrdev(unsigned int major, const char *name,
                    struct file_operations *fops);
```

返回值表示操作成功或失败。返回负值表示错误，0 返回正值表示成功。参数major是所请求的主设备号，name是设备的名称，它将出现在 /proc/devices 中，并且fops是指向函数指针数组的指针，用于调用驱动程序的入口点。

主编设备号是一个小整数，用作 char 驱动程序静态数组的索引；后面会解释如何选择主号码。2.0内核支持128个设备；2.2 和 2.4 将该数字增加到 256（同时保留值 0 和 255 以供将来使用）。  
次设备号也是八位数量。它们不会传递给 register_chrdev，因为如上所述，它们仅由驱动程序本身使用。开发者社区要求增加内核可支持的设备数量；将设备数量增加到至少 16 位是 2.5 开发系列的既定目标。

一旦驱动程序在内核表中注册，其操作就会与给定的主设备号相关联。**每当操作与该主设备号关联的字符设备时，内核都会从该file_operations结构中查找并调用正确的函数**。因此，传递给register_chrdev 的指针应指向驱动程序内的全局结构体，而不是模块初始化函数的本地结构体。

由以上内容可知，必须将设备名称插入到 /dev目录中并与驱动程序的主设备号和次设备号相关联，才可以使用驱动程序。  
在文件系统上创建设备节点的命令是 mknod；此操作需要超级用户权限。除了正在创建的文件的名称之外，该命令还采用三个参数。例如，命令
```shell
mknod /dev/scull0 c 254 0
```
创建一个 char 设备 ( c)，其主设备号为 254，次设备号为 0。次设备号应在 0 到 255 范围内，因为由于历史原因，它们有时存储在单个字节中。有充分的理由扩展可用次要编号的范围，但目前八位限制仍然有效。
请注意，一旦由mknod创建，特殊设备文件就会保留下来，除非明确删除它，就像存储在磁盘上的任何信息一样。可以通过rm /dev/scull0来删除本示例中创建的设备。

申请多个设备号
```c
// fs/char_dev.c
// 参数说明：
// first: 要分配的设备编号范围的起始值，常被置为0
// count: 所请求的连续设备编号的个数，如果count非常大，则所请求的范围可能和下一个主设备号重叠，但只要我们所请求的编号范围是可用的，就不会带来任何问题
// name: 和该编号范围关联的设备名称，它将出现在 /proc/devices 和 sysfs 中  
int register_chrdev_region(dev_t from, unsigned count, const char *name)

// 如果已知要使用的起始设备号，使用 register_chardev_region ，如果不知道，使用：
// 参数说明：
// dev: 用于输出的参数，在调用成功后保存已分配范围的第一个编号
// firseminor: 要使用的被请求的第一个次设备号，通常是0
// count、name参数与 register_chardev_region 一样  
int alloc_chardev_region(dev_t *dev, unsigned int firstminor, unsigned int count, char *name);
```

释放设备号
```c
// 释放设备号：
void unregister_chrdev_region(dev_t first, unsigned int count);
```

### 主设备号动态分配
一些主设备号静态分配给最常见的设备。这些设备的列表可以在Documentation/devices.txt中找到 。由于已经分配了许多编号，因此为新驱动程序选择唯一的编号可能很困难，但是自定义驱动程序比可用的主设备号要多得多。您可以使用为“实验或本地使用”保留的主要编号之一：[ 14 ]，但如果您尝试多个“本地”驱动程序或发布驱动程序供第三方使用，您将再次遇到选择问题一个合适的数字。

幸运的是可以请求动态分配主号码。major如果在调用 register_chrdev 时将参数 设置为 0 ，则该函数会选择一个空闲号码并将其返回。返回的主编号始终为正，而负返回值为错误代码。请注意，两种情况下的行为略有不同：如果调用者请求动态编号，则该函数返回分配的主编号，但在成功注册预定义主编号时返回 0（不是主编号）。

对于私有驱动，我们强烈建议您使用动态分配的方式来获取您的主设备号，而不是从当前空闲的设备号中随机选择一个号。另一方面，如果您的驱动程序旨在对整个社区有用并包含在官方内核树中，则您需要申请分配一个主编号以供专用。

动态分配的缺点是您无法提前创建设备节点，因为不能保证分配给模块的主设备号始终相同。这意味着您将无法使用驱动程序的按需加载。对于驱动程序的正常使用来说，这几乎不是问题，因为一旦分配了编号，您就可以从 中读取它/proc/devices。因此，要使用动态主设备号加载驱动程序，可以用一个简单的脚本来替换insmod的调用，该脚本在调用insmod后 读取/proc/devices以创建特殊文件。

典型的/proc/devices文件如下所示：
```shell
Character devices:
 1 mem
 2 pty
 3 ttyp
 4 ttyS
 6 lp
 7 vcs
 10 misc
 13 input
 14 sound
 21 sg
180 usb

Block devices:
 2 fd
 8 sd
 11 sr
 65 sd
 66 sd
 ```
因此，可以使用awk 等工具编写脚本，加载已分配动态编号的模块。
以下脚本(scull_load.sh)是scull发行版的一部分。以模块形式分发驱动程序的用户可以从系统rc.local文件中调用此类脚本，或者在需要该模块时手动调用它。
```shell
#!/bin/sh
module="scull"
device="scull"
mode="664"

# invoke insmod with all arguments we were passed
# and use a pathname, as newer modutils don't look in . by default
/sbin/insmod -f ./$module.o $* || exit 1

# remove stale nodes
rm -f /dev/${device}[0-3]

major=`awk "\\$2==\"$module\" {print \\$1}" /proc/devices`

mknod /dev/${device}0 c $major 0
mknod /dev/${device}1 c $major 1
mknod /dev/${device}2 c $major 2
mknod /dev/${device}3 c $major 3

# give appropriate group/permissions, and change the group.
# Not all distributions have staff; some have "wheel" instead.
group="staff"
grep '^staff:' /etc/group > /dev/null || group="wheel"

chgrp $group /dev/${device}[0-3]
chmod $mode /dev/${device}[0-3]
```

通过重新定义变量和调整mknod 行，可以将该脚本改编为另一个驱动程序的加载脚本。以上脚本创建了四个设备，因为scull源中的默认值是四个。

脚本的最后几行可能看起来很晦涩：为什么要更改设备的组和模式？原因是该脚本必须由超级用户运行，因此新创建的特殊文件归root所有。默认的权限位使得只有 root 具有写访问权限，而任何人都可以获得读访问权限。通常，设备节点需要不同的访问策略，因此必须以某种方式更改访问权限。我们的脚本中的默认设置是授予一组用户访问权限，但您的需求可能会有所不同。后续在 sculluid 的代码 将演示驱动程序如何强制执行自己的设备访问授权。然后可以使用scull_unload脚本 来清理/dev目录并删除模块。

作为使用一对脚本进行加载和卸载的替代方法，您可以编写一个 init 脚本，放置在这些脚本的目录中。作为 scull源代码的一部分，我们提供了一个相当完整且可配置的 init 脚本示例，称为 scull.init; 它接受常规参数——“start”或“stop”或“restart”——并执行scull_load和 scull_unload的角色。

如果重复创建和销毁/dev节点听起来有点大材小用，那么有一个有用的解决方法。如果您仅加载和卸载单个驱动程序，则可以在第一次使用脚本创建特殊文件后使用rmmod和insmod ：动态数字不是随机的，如果您选择相同的数字，则可以指望选择相同的数字。不要与其他（动态）模块混淆。在开发过程中避免冗长的脚本很有用。但显然，这个技巧一次不能扩展到多个驱动程序。

我们认为，分配主编号的最佳方法是默认动态分配，同时让您可以选择在加载时甚至编译时指定主编号。scull实现使用全局变量来scull_major保存所选的数字。该变量被初始化为 SCULL_MAJOR，在 scull.h中定义。分布式源中的默认值为 SCULL_MAJOR0，表示“使用动态分配”。scull_major 用户可以接受默认值或选择特定的主编号，方法是在编译前修改宏或在insmod命令行上指定值。insmod 指定可以通过scull_load脚本实现，用户可以在命令行上将参数传递给insmod。

这是我们在scull源代码 中用于获取主设备号的代码：
```shell
 result = register_chrdev(scull_major, "scull", &scull_fops);
 if (result < 0) {
  printk(KERN_WARNING "scull: can't get major %d\n",scull_major);
  return result;
 }
 if (scull_major == 0) scull_major = result; /* dynamic */
```

### 从系统中删除驱动程序
当模块从系统中卸载时，必须释放主设备号。这是通过以下函数完成的，您可以从模块的清理函数中调用该函数：

```c
int unregister_chrdev(unsigned int Major, const char *name);
```

参数是正在发布的主设备号和关联设备的名称。内核将该名称与拥有该主设备号的注册名称（如果有）进行比较：如果不同，则返回-EINVAL。如果主设备号超出允许的范围，内核也会返回-EINVAL，如果相同则注销该设备。

未能在清理函数中取消注册资源会产生不良影响。/proc/devices下次尝试读取它时会生成错误，因为其中一个字符串 name仍然指向已注销驱动模块的内存，而该内存不再被映射。这种错误称为 oops，这是内核在尝试访问无效地址时打印的消息。

当您卸载驱动程序而不取消注册主设备号时，恢复将很困难，因为unregister_chrdev 中的strcmp函数必须取消引用指向原始模块的指针。如果您无法取消已经注册的主设备号，则必须重新加载同一模块和另一个专门为取消已注册的主设备号而构建的模块。如果您没有更改代码，那么幸运的是，有问题的模块将获得相同的地址，并且字符串将位于相同的位置。当然，更安全的选择是重新启动系统。

除了卸载模块之外，您还经常需要清理已注销驱动程序的设备文件。该任务可以通过与加载时使用的脚本配对的脚本来完成。该脚本scull_unload为我们的示例设备完成工作；作为替代方案，您可以调用scull.init stop。

如果未从 /dev中删除动态设备文件，则可能会出现意外错误：如果两个驱动程序都使用动态主设备号，开发者电脑上空闲的 /dev/framegrabber 可能会在一个月后引发警报。“没有这样的文件或目录”是打开 /dev/framegrabber 比新驱动程序产生的更友好的响应。

### dev_t 和 kdev_t
到目前为止我们已经讨论了主设备号。现在是时候讨论次设备号以及驱动程序如何使用它来区分设备了。

每次内核调用设备驱动程序时，它都会告诉驱动程序正在对哪个设备进行操作。主设备号和次设备号以同一种数据类型配对，驱动程序使用该数据类型来标识特定设备。组合设备号（主设备号和次设备号连接在一起）位于结构体字段i_rdev中 ，inode稍后介绍。某些驱动程序函数接收指向struct inode的指针作为第一个参数。因此，使用指针 inode（就像大多数驱动程序编写者所做的那样），可以通过查看inode->i_rdev来提取设备号。

从历史上看，Unix 声明dev_t（设备类型）来保存设备号。它曾经是<sys/types.h>中定义的 16 位整数值。如今，有时需要超过 256 个次要编号，但更改 dev_t很困难，因为如果结构更改，有些应用程序就会崩溃。因此，虽然已经为更大的设备编号奠定了大部分基础，但它们目前仍为 16 位整数。

然而，在 Linux 内核中， kdev_t使用了不同的类型 。该数据类型被设计为每个内核函数的黑匣子。用户程序根本不知道kdev_t，内核函数也不知道kdev_t. 如果 kdev_t保持隐藏状态，它可以在不同版本中定义为不同的样子，而无需更改每个人的设备驱动程序。

有关的信息kdev_t定义在<linux/kdev_t.h>。无需在驱动程序中显式包含标头，因为 <linux/fs.h>它会为您完成。

您可以执行以下宏和函数的操作kdev_t：
```
MAJOR(kdev_t dev);
    Extract the major number from a kdev_t structure.

MINOR(kdev_t dev);
    Extract the minor number.

MKDEV(int ma, int mi);
    Create a kdev_t built from major and minor numbers.

kdev_t_to_nr(kdev_t dev);
    Convert a kdev_t type to a number (a dev_t).

to_kdev_t(int dev);
    Convert a number to kdev_t. Note that dev_t is not defined in kernel mode, and therefore int is used.
```


### 内核管理设备号
[Linux设备驱动-内核如何管理设备号](https://zhuanlan.zhihu.com/p/446439925#:~:text=%E6%AC%A1%E8%AE%BE%E5%A4%87%E5%8F%B7%E7%94%B1%E9%A9%B1%E5%8A%A8%E7%A8%8B%E5%BA%8F%E4%BD%BF%E7%94%A8%EF%BC%8C%E9%A9%B1%E5%8A%A8%E7%A8%8B%E5%BA%8F%E7%94%A8%E6%9D%A5%E6%8F%8F%E8%BF%B0%E4%BD%BF%E7%94%A8%E8%AF%A5%E9%A9%B1%E5%8A%A8%E7%9A%84%E8%AE%BE%E5%A4%87%E7%9A%84%E5%BA%8F%E5%8F%B7%EF%BC%8C%E5%BA%8F%E5%8F%B7%E4%B8%80%E8%88%AC%E4%BB%8E%200%20%E5%BC%80%E5%A7%8B%E3%80%82%20Linux%20%E8%AE%BE%E5%A4%87%E5%8F%B7%E7%94%A8%20dev_t%20%E7%B1%BB%E5%9E%8B%E7%9A%84%E5%8F%98%E9%87%8F%E8%BF%9B%E8%A1%8C%E6%A0%87%E8%AF%86%EF%BC%8C%E8%BF%99%E6%98%AF%E4%B8%80%E4%B8%AA%2032%E4%BD%8D,%2A%2F%20%E2%80%8B%20typedef%20u32%20__kernel_dev_t%3B%20typedef%20__kernel_dev_t%20dev_t%3B)  

## 字符设备驱动注册和注销
### 重要数据
```c
// include/linux/device/driver.h
struct device_driver {
	const char		*name;
	struct bus_type		*bus;  

	struct module		*owner;
	const char		*mod_name;	/* used for built-in modules */  

	bool suppress_bind_attrs;	/* disables bind/unbind via sysfs */
	enum probe_type probe_type;  

	const struct of_device_id	*of_match_table;
	const struct acpi_device_id	*acpi_match_table;  

	int (*probe) (struct device *dev);
	void (*sync_state)(struct device *dev);
	int (*remove) (struct device *dev);
	void (*shutdown) (struct device *dev);
	int (*suspend) (struct device *dev, pm_message_t state);
	int (*resume) (struct device *dev);
	const struct attribute_group **groups;
	const struct attribute_group **dev_groups;  

	const struct dev_pm_ops *pm;
	void (*coredump) (struct device *dev);  

	struct driver_private *p;
};
```  
  

### 接口
```c
// linux/include/linux/module.h
module_init
module_exit
```  

## 字符设备注册和注销
### 重要数据
```c
// linux/include/linux/cdev.h
struct cdev {
	struct kobject kobj;
	struct module *owner;
	const struct file_operations *ops;
	struct list_head list;
	dev_t dev;
	unsigned int count;
} __randomize_layout;
```  

### 接口  

申请空间 指定文件操作/方法
```c
// linux/fs/char_dev.c
struct cdev *my_cdev = cdev_alloc();
```  

初始化已分配到的结构体
```c
void cdev_init(struct cdev *cdev, struct file_operations *fops);
my_cdev->owner = THIS_MODULE;  // 所有者
my_cdev->ops = &my_fops;
```  

注册设备到内核
```c
// linux/fs/char_dev.c
int cdev_add(struct cdev *dev, dev_t num, unsigned int count);
```
参数说明：
dev:  cdev结构体
num: 该设备对应的地一个设备编号
count: 应该和该设备关联的设备编号的数量
注意：可能存在注册失败的问题，因此要关注返回值！  

从内核中注销
```c
// linux/fs/char_dev.c
void cdev_del(strut cdev *dev);
```  

内核中的老接口
```c
int register_chrdev(unsigned int major, const char *name, struct file_operations *fops);  // 注册
int unregister_chrdev(unsigned int major, const char *name); // 注销
```  

### 创建设备节点
- mknod  

mknod DEVNAME {b | c}  MAJOR  MINOR
1. DEVNAME是要创建的设备文件名，如果想将设备文件放在一个特定的文件夹下，就需要先用mkdir在dev目录下新建一个目录
2. b和c 分别表示块设备和字符设备
3. MAJOR和MINOR分别表示主设备号和次设备号：
      为了管理设备，系统为每个设备分配一个编号，设备号由主设备号和次设备号组成。主设备号标示某一种类的设备，次设备号用来区分同一类型的设备。linux操作系统中为设备文件编号分配了32位无符号整数，其中前12位是主设备号，后20位为次设备号，所以在向系统申请设备文件时主设备号不能超过4095，次设备号不能超过2^20 -1。  

device_create  

## 字符设备和驱动的绑定
见本文中 “设备号的作用”  

## 重要的数据结构  

大部分基本的驱动程序操作涉及到三个重要的内核数据结构，分别是 file_operations、file和inode  

### 文件操作
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

### inode结构  

内核用inode结构在内部表示文件，因此它和file结构不同，file表示打开的文件描述符。对于单个文件，可能会有 许多个 表示打开的文件描述符(file)，但他们都指向 单个 inode结构。
inode结构中包含了大量有关文件的信息。但通常只有下面两个字段对编写驱动程序代码有用：  

dev_t i_rdev
- 对表示设备文件的inode结构，该字段包含了真正的设备编号  

struct cdev *i_cdev
- struct cdev 是表示字符设备在内核中的结构。当inode指向一个字符设备文件时，该字段包含了指向struct cdev结构体的指针  

为了鼓励编写可移植性更强的代码，建议使用如下方法从 inode 中获取主设备号和从设备号
```c
unsigned int iminor(struct inode *inode);
unsigned int iminor(struct inode *inode);
```  

## open 和 release  

open方法提供给驱动程序以初始化的能力，从而为以后的操作做准备，open大多数情况下应该完成如下工作：
- 检测设备特定的错误（诸如设备未就绪或类似的硬件问题）
- 如果设备是首次打开，则对其进行初始化
- 如果有必要，更新 f_op 指针
- 分配并填写置于 filp->private_data 里的数据  

原型：
```c
int (*open)(struct inode *inoed, struct file *filep);
```  

其中 inode 参数在 i_cdev 字段中包含了我们所需要的信息，即 我们先前设置的 cdev 结构体  

这里一个函数可能需要使用到：
```c
container_of(pointer, container_type, container_field);  // 定义在<linux/kernel.h>中
```  

该函数根据指定结构体(container_type)和其内部的成员(container_field)，计算入参(pointer)对应的结构体(container_type)实例入口，这里的入参(pointer)也是结构体(container_type)的成员(container_field)  

当我们创建一个自己使用的结构体时，如果在结构体内部放了 cdev，则可以根据结构体内部放的 cdev设备 (open函数中的 inode->i_cdev) 计算出我们自己创建的结构体的入口  

release 方法与 open方法相反，该方法应该完成以下工作：
- 释放由 open 分配、保存在 filp->private_data 中的所有内容
- 在最后一次关闭操作时关闭设备  

如何保证关闭一个设备文件的次数不会比打开的次数多？  

内核对每个 file 结构都会记录被引用的次数。无论是 fork 还是 dup，都不会创建新的数据结构（仅由 open 创建），他们只是增加已有结构体中的计数。只有在 file 结构体归零时，close系统调用才会执行 release 方法，这只在删除这个结构体时才会发生，因此release和close的系统调用间的关系保证了对于每次open驱动程序只会看到对应的一次release调用。  
  

## 5. read 和 write  

read 拷贝数据到应用程序空间，write拷贝数据到内核空间，原型：
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

read 方法
- 如果返回值等于传递给read系统调用的count参数，则说明所请求的字节数传输成功完成了
- 如果返回值是正的，但是比count小，说明只有部分数据成功传输。这种情况因设备的不同可能有许多原因。大部分情况下程序会重新读书据。例如，如果用fread函数读数据，这个库函数就会不断调用系统调用，直到所请求的数据传输完毕为止
- 如果返回值为 0，表示已经到达了文件尾
- 负值意味着发生了错误，该值指明了发生了什么错误，错误码在<linux/errno.h>中定义  

write 方法
- 如果返回值等于count，则完成了所请求数目的字节传送
- 如果返回值是正的，但小于 count， 则只传输了部分数据。程序很可能再次写入余下的数据
- 如果值为0,意味着什么也没写入。这个结果不是错误。类似read方法，标准库会重复调用write
- 负值意味着发生了错误，类似read方法  

read 和 write 方法的 buff 参数是用户空间的指针。因此，内核代码不能直接引用其中的内容，原因如下：
- 由于驱动程序所运行的架构不同或内核配置不同，在内核模式中运行时，用户空间的指针可能是无效的。该地址可能根本无法被映射到内核空间，或者可能指向某些随机数据
- 即使该指针在内核空间中代表相同的东西，但用户空间的内存是分页的，而在系统接口被调用时，涉及到的内存可能根本就不在RAM中。对用户空间内存的直接引用将导致页错误，而这对于内核代码来说是不允许发生的事情。其结果可能是一个"oops"，他将导致调用该系统接口的进程死亡。
- 我们讨论的指针可能由用户程序提供，而该程序可能存在缺陷或者是个恶意程序。如果我们的驱动程序盲目引用用户提供的指针，将导致系统出现打开的后门，从而允许用户空间程序随意访问或覆盖系统中的内存。  

从内核空间访问用户空间数据的接口：
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
```c
if(!access_ok(VERIFY_WRITE, buf, count))
	return -EFAULT;
	
if(__put_user(inb(i),tmp) < 0)
	return -EFAULT;
```  

如果要复制的内存是简单类型，如：char、int、long等，则可以使用简单的 put_user() (内核->用户)和 get_user()(用户->内核)
__put_user()和put_user()的区别在于前者不进行类似access_ok()的检查，后者会进行这一检查。  

## demo  

ch6 文件夹是送宝华的《Linux设备驱动开发详解》的第六章的demo  

scull 是《设备驱动程序》的demo  

