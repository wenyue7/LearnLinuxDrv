# old doc

从上面的代码来看，要实现一个 dma-buf exporter驱动，需要执行3个步骤：
1. dma_buf_ops
2. DEFINE_DMA_BUF_EXPORT_INFO
3. dma_buf_export()

**注意：** 其中 *dma_buf_ops* 的回调接口中，如下接口又是必须要实现的，缺少任何一个都将导致 *dma_buf_export()* 函数调用失败！

- map_dma_buf
- unmap_dma_buf
- map
- map_atomic
- mmap
- release



| 内核态调用               | 对应 dma_buf_ops 中的接口 | 说明                                                         |
| ------------------------ | ------------------------- | ------------------------------------------------------------ |
| dma_buf_kmap()           | map                       |                                                              |
| dma_buf_kmap_atomic()    | map_atomic                |                                                              |
| dma_buf_vmap()           | vmap                      |                                                              |
| dma_buf_attach()         | attach                    | 该函数实际是 *“dma-buf attach device”* 的缩写，用于建立一个 dma-buf 与 device 的连接关系，这个连接关系被存放在新创建的 *dma_buf_attachment* 对象中，供后续调用 *dma_buf_map_attachment()* 使用。如果 device 对后续的 map attachment 操作没有什么特殊要求，可以不实现。 |
| dma_buf_map_attachment() | map_dma_buf               | 该函数实际是 *“dma-buf map attachment into sg_table”* 的缩写，它主要完成2件事情：1. 生成 sg_table     2. 同步 Cache |
|                          |                           |                                                              |
|                          |                           |                                                              |


# DMA

## Device memory mapping relationship

from https://docs.kernel.org/core-api/dma-api-howto.html


```
             CPU                  CPU                  Bus
           Virtual              Physical             Address
           Address              Address               Space
            Space                Space

          +-------+             +------+             +------+
          |       |             |MMIO  |   Offset    |      |
          |       |  Virtual    |Space |   applied   |      |
        C +-------+ --------> B +------+ ----------> +------+ A
          |       |  mapping    |      |   by host   |      |
+-----+   |       |             |      |   bridge    |      |   +--------+
|     |   |       |             +------+             |      |   |        |
| CPU |   |       |             | RAM  |             |      |   | Device |
|     |   |       |             |      |             |      |   |        |
+-----+   +-------+             +------+             +------+   +--------+
          |       |  Virtual    |Buffer|   Mapping   |      |
        X +-------+ --------> Y +------+ <---------- +------+ Z
          |       |  mapping    | RAM  |   by IOMMU
          |       |             |      |
          |       |             |      |
          +-------+             +------+
```

ioremap: C->B->A
- map kernal virtual address space to cpu pyh address space
- map pyh addr to bus addr space by host birdge
- read/write bus addr space by kernel virtual address

dma_alloc_coherent: X->Y, Z->Y
- map kernal virtual address space to cpu pyh address space
- map bus addr space to cpu pyh address space
- read/write pyh space by kernel virtual address
- config bus address to device, device read/write phy space by device bus address


## kmap / vmap
from https://blog.csdn.net/hexiaolong2009/article/details/102596761

**CPU Access**

Starting from linux-3.4, dma-buf introduces a CPU operation interface,
allowing developers to directly use the CPU in the kernel space to access 
the physical memory of dma-buf.

The following dma-buf API implements CPU access to dma-buf memory in kernel space:
- dma_buf_kmap()
- dma_buf_kmap_atomic()
- dma_buf_vmap()


Through the dma_buf_kmap() / dma_buf_vmap() operation, the actual physical 
memory can be mapped to the kernel space, and converted into a virtual address 
that the CPU can continuously access, so that subsequent software can directly 
read and write this physical memory. Therefore, regardless of whether the buffer 
is physically continuous, the virtual addresses after kmap/vmap mapping must be 
continuous.

The above three interfaces correspond to the kmap(), kmap_atomic() and vmap() 
functions in the linux memory management subsystem (MM) respectively. 
The differences between the three are as follows:

| function | description |
|----|----|
| kmap()        | can only map 1 page at a time, may sleep, and can only be called in the process context |
| kmap_atomic() | can only map 1 page at a time, will not sleep, and can be called in interrupt context |
| vmap()        | can map multiple pages at a time, and these pages can be physically discontinuous, and can only be called in the process context |


## map attachment
from https://blog.csdn.net/hexiaolong2009/article/details/102596772

**DMA Access**

There are two main APIs provided by dma-buf for DMA hardware access:
- dma_buf_attach()
- dma_buf_map_attachment()
These two interfaces are called in a strict order, they must be attached first,
and then map attachment, because the parameters of the latter are provided by 
the former, so these two interfaces are usually inseparable.

The reverse operation interfaces corresponding to the above two APIs are: dma_buf_dettach() and dma_buf_unmap_attachment()


**sg_table**

sg_table is the ultimate goal of dma-buf for DMA hardware access,
and it is also the only way for DMA hardware to access discrete memory!

sg_table is essentially a linked list composed of many single physically 
continuous buffers, but this linked list is discrete as a whole, so it can 
well describe the discrete buffers allocated from high-end memory. 
Of course, it can also be used to describe physically continuous buffers 
allocated from low-end memory.

sg_table represents the entire linked list, and each of its linked list items 
is represented by scatterlist. Therefore, a scatterlist corresponds to a 
physically continuous buffer. We can obtain the physical address and length 
of the buffer corresponding to a scatterlist through the following interface:
- sg_dma_address(sgl)
- sg_dma_len(sgl)

With the physical address and length of the buffer, we can configure these 
two parameters into the DMA hardware register, so that the DMA hardware can 
access the buffer. So how to access the entire discrete buffer? Of course, 
use a for loop to continuously parse the scatterlist and configure the DMA 
hardware registers continuously!


**dma_buf_attach()**

This function is actually the abbreviation of "dma-buf attach device", 
which is used to establish a connection relationship between dma-buf and device.
This connection relationship is stored in the newly created dma_buf_attachment 
object for subsequent calls to dma_buf_map_attachment().

This function corresponds to the attach callback interface in dma_buf_ops. 
If the device has no special requirements for subsequent map attachment 
operations, it may not be implemented.

**dma_buf_map_attachment()**

This function is actually the abbreviation of "dma-buf map attachment into sg_table", 
it mainly completes 2 things:
1. Generate sg_table: 
The choice to return sg_table instead of physical address 
is for compatibility with all DMA hardware (with or without IOMMU), since sg_table 
can represent both contiguous and non-contiguous physical memory.

2. Synchronize Cache:
The purpose of synchronizing the Cache is to prevent the buffer from being filled 
by the CPU in advance, and the data is temporarily stored in the Cache instead of 
the DDR, causing the DMA to access not the latest valid data. Such problems can 
be avoided by writing back the data in the Cache to the DDR. Similarly, after 
the DMA accesses the memory, the Cache needs to be set to be invalid, so that 
the subsequent CPU can directly read the memory data from the DDR (not from the Cache). 
Usually we use the following streaming DMA mapping interface to complete Cache 
synchronization:
- dma_map_single() / dma_unmap_single()
- dma_map_page() / dma_unmap_page()
- dma_map_sg() / dma_unmap_sg()

For more descriptions of the dma_map_*() functions, recommen to read 
https://blog.csdn.net/jasonchen_gbd/article/details/79462064

dma_buf_map_attachment() corresponds to the map_dma_buf callback interface in 
dma_buf_ops, the callback interface (including unmap_dma_buf) is mandatory to 
implement, otherwise dma_buf_export() will fail to execute.

**为什么需要 attach 操作 ？**

同一个 dma-buf 可能会被多个 DMA 硬件访问，而每个 DMA 硬件可能会因为自身硬件能力
的限制，对这块 buffer 有自己特殊的要求。比如硬件 A 的寻址能力只有0x0 ~ 0x10000000，
而硬件 B 的寻址能力为 0x0 ~ 0x80000000，那么在分配 dma-buf 的物理内存时，就必须
以硬件 A 的能力为标准进行分配，这样硬件 A 和 B 都可以访问这段内存。否则，如果
只满足 B 的需求，那么 A 可能就无法访问超出 0x10000000 地址以外的内存空间，
道理其实类似于木桶理论。

因此，attach 操作可以让 exporter 驱动根据不同的 device 硬件能力，来分配最合适的
物理内存。

通过设置 device->dma_params 参数，来告知 exporter 驱动该 DMA 硬件的能力限制。

但是在上一篇的示例中，dma-buf 的物理内存都是在 dma_buf_export() 的时候就分配好的，
而 attach 操作只能在 export 之后才能执行，那我们如何确保已经分配好的内存是符合
硬件能力要求的呢？这就引出了下面的话题。

**何时分配内存？**

答案是：既可以在 export 阶段分配，也可以在 map attachment 阶段分配，甚至可以
在两个阶段都分配，这通常由 DMA 硬件能力来决定。

首先，驱动人员需要统计当前系统中都有哪些 DMA 硬件要访问 dma-buf；
然后，根据不同的 DMA 硬件能力，来决定在何时以及如何分配物理内存。

通常的策略如下（假设只有 A、B 两个硬件需要访问 dma-buf ）：

如果硬件 A 和 B 的寻址空间有交集，则在 export 阶段进行内存分配，分配时以 
A / B 的交集为准；
如果硬件 A 和 B 的寻址空间没有交集，则只能在 map attachment 阶段分配内存。
对于第二种策略，因为 A 和 B 的寻址空间没有交集（即完全独立），所以它们实际上是
无法实现内存共享的。此时的解决办法是： A 和 B 在 map attachment 阶段，都分配
各自的物理内存，然后通过 CPU 或 通用DMA 硬件，将 A 的 buffer 内容拷贝到 B 的 
buffer 中去，以此来间接的实现 buffer “共享”。

另外还有一种策略，就是不管三七二十一，先在 export 阶段分配好内存，然后在首次 
map attachment 阶段通过 dma_buf->attachments 链表，与所有 device 的能力进行
一一比对，如果满足条件则直接返回 sg_table；如果不满足条件，则重新分配符合所有 
device 要求的物理内存，再返回新的 sg_table。


## mmap

In order for applications to read and write dma-buf memory directly in 
user space, dma_buf_ops provides us with an mmap callback interface, 
which can directly map the physical memory of dma-buf to user space, 
so that applications can access ordinary files. Access the physical 
memory of dma-buf.

In addition to the mmap callback interface provided by dma_buf_ops, 
dma-buf also provides us with the dma_buf_mmap() kernel API, so that 
we can use local materials in other device drivers, directly refer to 
the mmap implementation of dma-buf, and indirectly realize Device 
driver's mmap file operation interface.


## dma-buf and file
from https://blog.csdn.net/hexiaolong2009/article/details/102596802

dma-buf is essentially a combination of buffer and file, not only that,
the file is also an opened file, and the file is also an anonymous file

**fd**

The following kernel API realizes the conversion between dma-buf and fd:
- dma_buf_fd()：dma-buf --> new fd
- dma_buf_get()：fd --> dma-buf


**get / put**

As long as it is a file, there will be a reference count (f_count) inside.
When using the dma_buf_export() function to create a dma-buf,
the reference count is initialized to 1; when the reference count is 0,
the release callback interface of dma_buf_ops will be automatically 
triggered and the dma-buf object will be released.


The commonly used functions for manipulating file reference counts 
in the linux kernel are fget() and fput(), and dma-buf encapsulates 
them on this basis, as follows:
- get_dma_buf()
- dma_buf_get()
- dma_buf_put()

function summary:
| function | description |
|----|----|
| get_dma_buf() | only adds 1 to the reference count |
| dma_buf_get() | adds 1 to the reference count and converts fd to a dma_buf pointer |
| dma_buf_put() | reference count minus 1 |
| dma_buf_fd()  | reference count unchanged, only fd created |


