# Linux内核中IO,IOR,IOW,IOWR宏的用法与解析

在驱动程序里， ioctl() 函数上传送的变量 cmd 是应用程序用于区别设备驱动程序请求处理内容的值。cmd除了可区别数字外，还包含有助于处理的几种相应信息。 cmd的大小为 32位，共分 4 个域：

​	**bit31~bit29** 3位为 “区别读写” 区，作用是区分是读取命令还是写入命令。
​	**bit28~bit16** 13位为 "数据大小" 区，表示 ioctl() 中的 arg 变量传送的内存大小。
​	**bit15~bit08**  8位为 “魔数"(也称为"幻数")区，这个值用以与其它设备驱动程序的 ioctl 命令进行区别。
​	**bit07~bit00**  8位为 "区别序号" 区，是区分不同命令的序号。

例如命令码中的 “区分读写区” 里的值可能是 _ IOC_NONE （0值）表示无数据传输，_ IOC_READ (读)， _ IOC_WRITE (写) ， _ IOC_READ|_IOC_WRITE (双向)。

```c
#define _IOC_NONE   1U                                                                                                             
#define _IOC_READ   2U                                                                                                             #define _IOC_WRITE  4U  
```

内核定义了 **_IO()** , **_IOR()** , **_IOW()** 和 **_IOWR()** 这 4 个宏来辅助生成上面的 cmd 。

```c
#define _IO(type,nr)        _IOC(_IOC_NONE,(type),(nr),0)
#define _IOR(type,nr,size)  _IOC(_IOC_READ,(type),(nr),sizeof(size))
#define _IOW(type,nr,size)  _IOC(_IOC_WRITE,(type),(nr),sizeof(size))
#define _IOWR(type,nr,size) _IOC(_IOC_READ|_IOC_WRITE,(type),(nr),sizeof(size))

// _IO(), _IOR(), _IOW() 和 _IOWR() 都使用这个宏，只不过参数不一样
// dir << 29, type << 8, nr << 0, size << 16
// 读写		 魔数		  区别序号	 数据大小
#define _IOC(dir,type,nr,size) \                                                                                                           (((dir)  << _IOC_DIRSHIFT) | \                                                                                                     ((type) << _IOC_TYPESHIFT) | \                                                                                                     ((nr)   << _IOC_NRSHIFT) | \                                                                                                       ((size) << _IOC_SIZESHIFT))

// _IOC_DIRSHIFT    29
#define _IOC_DIRSHIFT    (_IOC_SIZESHIFT + _IOC_SIZEBITS) 
//							16				13
#define _IOC_SIZESHIFT   (_IOC_TYPESHIFT + _IOC_TYPEBITS)
//							8				8
#define _IOC_TYPESHIFT   (_IOC_NRSHIFT + _IOC_NRBITS)
//							0				8
#define _IOC_NRSHIFT     0
#define _IOC_NRBITS      8
#define _IOC_TYPEBITS    8
#define _IOC_SIZEBITS   13

// _IOC_TYPESHIFT   8
#define _IOC_TYPESHIFT   (_IOC_NRSHIFT + _IOC_NRBITS)
#define _IOC_NRSHIFT     0
#define _IOC_NRBITS      8

// _IOC_NRSHIFT    0
#define _IOC_NRSHIFT     0

// _IOC_SIZESHIFT	16
#define _IOC_SIZESHIFT   (_IOC_TYPESHIFT + _IOC_TYPEBITS) 
#define _IOC_TYPESHIFT   (_IOC_NRSHIFT + _IOC_NRBITS)
#define _IOC_NRSHIFT     0
#define _IOC_NRBITS      8
#define _IOC_TYPEBITS    8

```





**魔数 (magic number)**
      魔数范围为 0~255 。通常，用英文字符 "A" ~ "Z" 或者 "a" ~ "z" 来表示。设备驱动程序从传递进来的命令获取魔数，然后与自身处理的魔数想比较，如果相同则处理，不同则不处理。魔数是拒绝误使用的初步辅助状态。设备驱动程序可以通过 **_IOC_TYPE** (cmd) 来获取魔数。不同的设备驱动程序最好设置不同的魔数，但并不是要求绝对，也是可以使用其他设备驱动程序已用过的魔数。

```c
// _IOC_TYPESHIFT 8
// _IOC_TYPEMASK bit0-bit7 全是1
// 将 nr 向右移动八位，然后使用 _IOC_TYPEMASK 屏蔽高位比特，取到一个八位的魔数
#define _IOC_TYPE(nr)       (((nr) >> _IOC_TYPESHIFT) & _IOC_TYPEMASK)
#define _IOC_TYPESHIFT   (_IOC_NRSHIFT + _IOC_NRBITS)
#define _IOC_NRSHIFT     0
#define _IOC_NRBITS      8
#define _IOC_TYPEMASK    ((1 << _IOC_TYPEBITS)-1)
#define _IOC_TYPEBITS    8
```



**基(序列号)数**
      基数用于区别各种命令。通常，从 *0*开始递增，相同设备驱动程序上可以重复使用该值。例如，读取和写入命令中使用了相同的基数，设备驱动程序也能分辨出来，原因在于设备驱动程序区分命令时使用 switch ，且直接使用命令变量 cmd 值。创建命令的宏生成的值由多个域组合而成，所以即使是相同的基数，也会判断为不同的命令。设备驱动程序想要从命令中获取该基数，就使用下面的宏：
_IOC_NR (cmd)

```c
// 将 nr 向右移0位，然后与 _IOC_NRMASK 相与，取低八位
#define _IOC_NR(nr)         (((nr) >> _IOC_NRSHIFT) & _IOC_NRMASK)
#define _IOC_NRSHIFT     0
#define _IOC_NRMASK      ((1 << _IOC_NRBITS)-1)
#define _IOC_NRBITS      8
```



**变量型**
设备驱动程序想要从传送的命令获取相应的值，就要使用宏函数：   _IOC_SIZE(cmd)





# 总结

**关键记住使用方法**

创建命令（内部调用的都是宏 _IOC(dir,type,nr,size)）：

```c
_IO(type,nr)
_IOR(type,nr,size)
_IOW(type,nr,size)
_IOWR(type,nr,size)
```

获取域值：

```c
_IOC_TYPE(cmd)  // 魔数 type
_IOC_NR(cmd)    // 区别序号 nr
_IOC_SIZE(cmd)  // 数据大小 size
_IOC_DIR(cmd)   // 数据方向 read/write
```



关于如何使用可以在linux内核代码中自行搜索相关关键字查看