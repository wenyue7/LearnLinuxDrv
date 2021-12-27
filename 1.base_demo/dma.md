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

