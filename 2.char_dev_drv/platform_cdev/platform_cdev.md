# platform device

通常把在SoC内部继承的I2C、RTC、LCD、看门狗等控制器都归纳为paltform_device，而他们本身就是字符设备。

## 1. 重要的数据结构

### **设备** platform_device

```c
struct platform_device {
	const char	*name;
	int		id;
	bool		id_auto;
	struct device	dev;
	u32		num_resources;
	struct resource	*resource;

	const struct platform_device_id	*id_entry;
	char *driver_override; /* Driver name to force a match */

	/* MFD cell pointer */
	struct mfd_cell *mfd_cell;

	/* arch specific additions */
	struct pdev_archdata	archdata;
};
```

```c
struct device {
	struct device		*parent;

	struct device_private	*p;

	struct kobject kobj;
	const char		*init_name; /* initial name of the device */
	const struct device_type *type;

	struct mutex		mutex;	/* mutex to synchronize calls to
					 * its driver.
					 */

	struct bus_type	*bus;		/* type of bus device is on */
	struct device_driver *driver;	/* which driver has allocated this
					   device */
	void		*platform_data;	/* Platform specific data, device
					   core doesn't touch it */
	void		*driver_data;	/* Driver data, set and get with
					   dev_set/get_drvdata */
	struct dev_links_info	links;
	struct dev_pm_info	power;
	struct dev_pm_domain	*pm_domain;

#ifdef CONFIG_GENERIC_MSI_IRQ_DOMAIN
	struct irq_domain	*msi_domain;
#endif
#ifdef CONFIG_PINCTRL
	struct dev_pin_info	*pins;
#endif
#ifdef CONFIG_GENERIC_MSI_IRQ
	struct list_head	msi_list;
#endif

#ifdef CONFIG_NUMA
	int		numa_node;	/* NUMA node this device is close to */
#endif
	const struct dma_map_ops *dma_ops;
	u64		*dma_mask;	/* dma mask (if dma'able device) */
	u64		coherent_dma_mask;/* Like dma_mask, but for
					     alloc_coherent mappings as
					     not all hardware supports
					     64 bit addresses for consistent
					     allocations such descriptors. */
	u64		bus_dma_mask;	/* upstream dma_mask constraint */
	unsigned long	dma_pfn_offset;

	struct device_dma_parameters *dma_parms;

	struct list_head	dma_pools;	/* dma pools (if dma'ble) */

	struct dma_coherent_mem	*dma_mem; /* internal for coherent mem
					     override */
#ifdef CONFIG_DMA_CMA
	struct cma *cma_area;		/* contiguous memory area for dma
					   allocations */
#endif
	struct removed_region *removed_mem;
	/* arch specific additions */
	struct dev_archdata	archdata;

	struct device_node	*of_node; /* associated device tree node */
	struct fwnode_handle	*fwnode; /* firmware device node */

	dev_t			devt;	/* dev_t, creates the sysfs "dev" */
	u32			id;	/* device instance */

	spinlock_t		devres_lock;
	struct list_head	devres_head;

	struct klist_node	knode_class;
	struct class		*class;
	const struct attribute_group **groups;	/* optional groups */

	void	(*release)(struct device *dev);
	struct iommu_group	*iommu_group;
	struct iommu_fwspec	*iommu_fwspec;

	bool			offline_disabled:1;
	bool			offline:1;
	bool			of_node_reused:1;
	bool			state_synced:1;

	ANDROID_KABI_RESERVE(1);
	ANDROID_KABI_RESERVE(2);
	ANDROID_KABI_RESERVE(3);
	ANDROID_KABI_RESERVE(4);
	ANDROID_KABI_RESERVE(5);
	ANDROID_KABI_RESERVE(6);
	ANDROID_KABI_RESERVE(7);
	ANDROID_KABI_RESERVE(8);
};
```



### **驱动 platform_driver**

```c
struct platform_driver {
	int (*probe)(struct platform_device *);
	int (*remove)(struct platform_device *);
	void (*shutdown)(struct platform_device *);
	int (*suspend)(struct platform_device *, pm_message_t state);
	int (*resume)(struct platform_device *);
	struct device_driver driver;
	const struct platform_device_id *id_table;
	bool prevent_deferred_probe;
};
```

```c
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

	const struct dev_pm_ops *pm;
	void (*coredump) (struct device *dev);

	struct driver_private *p;

	ANDROID_KABI_RESERVE(1);
	ANDROID_KABI_RESERVE(2);
	ANDROID_KABI_RESERVE(3);
	ANDROID_KABI_RESERVE(4);
};
```

### **总线 bus_type**

```c
struct bus_type {
	const char		*name;
	const char		*dev_name;
	struct device		*dev_root;
	const struct attribute_group **bus_groups;
	const struct attribute_group **dev_groups;
	const struct attribute_group **drv_groups;

	int (*match)(struct device *dev, struct device_driver *drv);
	int (*uevent)(struct device *dev, struct kobj_uevent_env *env);
	int (*probe)(struct device *dev);
	void (*sync_state)(struct device *dev);
	int (*remove)(struct device *dev);
	void (*shutdown)(struct device *dev);

	int (*online)(struct device *dev);
	int (*offline)(struct device *dev);

	int (*suspend)(struct device *dev, pm_message_t state);
	int (*resume)(struct device *dev);

	int (*num_vf)(struct device *dev);

	int (*dma_configure)(struct device *dev);

	const struct dev_pm_ops *pm;

	const struct iommu_ops *iommu_ops;

	struct subsys_private *p;
	struct lock_class_key lock_key;

	bool need_parent_lock;

	ANDROID_KABI_RESERVE(1);
	ANDROID_KABI_RESERVE(2);
	ANDROID_KABI_RESERVE(3);
	ANDROID_KABI_RESERVE(4);
};
```



## 2. 设备注册和注销

初始化过程中可以完全按照cdev的步骤来，但也可以替换为如下的方法

设备注册和注销的过程类似cdev的 module_init 和 module_exit 上挂的函数，只不过这里挂的是 platform_driver->probe 和 platform_driver->remove

**定义注册用的结构体**

```c
static struct platform_driver mpp_service_driver = {
	.probe = mpp_service_probe,
	.remove = mpp_service_remove,
	.driver = {
		.name = "mpp_service",
		.of_match_table = of_match_ptr(mpp_dt_ids),
	},
};
```

**设备注册/注销**

```c
 // 注册： 调用 platform_driver->probe ,类似 module_init
#define platform_driver_register(drv) \
	__platform_driver_register(drv, THIS_MODULE)
extern int __platform_driver_register(struct platform_driver *,	struct module *);

// 注销： 调用 platform_driver->remove ,类似 module_exit
extern void platform_driver_unregister(struct platform_driver *)
```

**同时包含注册和注销的接口**

```c
module_platform_driver(__platform_driver)
		--> extern int __platform_driver_register(struct platform_driver *, struct module *)       // module_init()
				--> int driver_register(struct device_driver *drv)
		--> void platform_driver_unregister(struct platform_driver *drv)      // module_exit()
				--> void driver_unregister(struct device_driver *drv)
```



***注意：***

​		module_platform_driver()宏所定义的模块加载和卸载函数仅仅通过platform_driver_register()、platform_driver_unregister()函数进行platform_driver的注册与注销，而原先注册和注销字符设备的工作已经被移交到platform_driver的probe()和remove()成员函数中。注册完platform_driver <driverModuleName>之后可以发现路径/sys/bus/platform/drivers 目录下多出了一个名字叫<driverModuleName>的子目录。



**其他函数**

```c
// platform_device->dev->driver_data.  <====  data  注意这里使用的是device，不是注册时使用的driver
static inline void platform_set_drvdata(struct platform_device *pdev,	void *data)
```



## 3. 添加设备

对于paltform_device的定义通常在BSP板文件中实现，在板文件中，将platform_device归纳为一个数组，最终通过platform_add_devices()函数统一注册。platform_add_devices()函数可以将平台设备添加到系统中，该函数的内部调用了platform_device_register()函数以注册单个的平台设备，这个函数原型为：

```c
int platform_add_devices(struct platform_device **devs, int num)
```

devs: 平台设备数组的指针

num: 平台设备的数量

Linux 3.x 之后，ARM Linux不太喜欢人们以编码的形式去填写platform_device和注册，而倾向于根据设备树中的内容自动展开platform_device。



**老的注册设备的方法**

注册完名为<driverModuleName>的platform_driver之后，需要在板文件arch/arm/mach-<soc>/mach-<>.c中添加相应的代码，例如：

```c
static struct platform_device globalfifo_device = {
    .name = "globalfifo",
    .id   = -1,
};
```

并最终通过类似platform_add_devices()的函数把这个platform_device注册进系统。如果一切顺利，可以在/sysdevices/platform目录下看到一个名字叫globalfifo的子目录，/sys/devices/platform/globalfifo中会有一个driver文件，它是指向/sys/bus/platform/drivers/globalfifo的符号链接，这就证明驱动和设备匹配上了。





## 4. platform设备资源和数据

```c
struct resource{
    resource__size_t start;
    resource_size_t end;
    const char *name;
    unsigned long flags;
    struct resource *parent, *sibling, *child;
};
```

​		通常关系start、end和flags这三个字段，他们分别标明了资源的开始值、结束值和类型，flags可以为IORESOURCE_IO、IORESOURCE_MEM、IORESOURCE_IRQ、IORESOURCE_DMA等。start、end的含义会随着flags而变更，如果当flags为IORESOURCE_MEM时，start、end分别表示该platform_device占据的内存的开始地址和结束地址；当flags为IORESOURCE_IRQ时，start、end分别表示该platform_device使用的中断号的开始值和结束值，如果只使用了一个中断号，开始和结束值相同。对于同种类型的资源而言，可以有多分，例如某设备占据了两个内存区域，则可以定义两个IORESOURCE_MEM资源。

​		对于resource的定义也通常在BSP的板文件中进行，而在具体的设备驱动中通过platform_get_resource()这样的api来获取，此api原型为：

```c
struct resource *platform_get_resource(struct platform_device *,unsigned int, unsigned int);
```

​		对于IRQ而言，platform_getresource()还有一个进行了封装的变体platform_get_irq()，其原型为：

```c
int platform_get_irq(struct platform_device *dev, unsigned int num);
```

它实际上调用了"platform_getresource(dev, IORESOURCE_IRO, num);"。



​		设备除了可以在BSP中定义资源以外，还可以附加一些数据信息，这些配置信息也依赖于板，不宜直接放置在设备驱动上，因此platform也提供了platform_data的支持，platform_data的形式是由每个驱动自定义的，如对于DM9000网卡而言，paltform_data为一个dm9000_plat_data结构体，该结构体存放在 platform_device.dev.platform_data 字段下。

​		通过如下方式可以拿到paltform_data：

```c
static inline void *dev_get_platdata(const struct device *dev) 
```

其中pdev为platfrom_device 的指针。



引入platform的概念有如下优点：

1. 使得设备被挂接在一个总线上，符合Linux 2.6以后内核的设备模型。其结果是使配套的sysfs节点、设备电源管理都成为可能。
2. 隔离BSP和驱动。在BSP中定义platform设备和设备使用的资源、设备的具体配置信息，而在驱动中只需要通过通用api去获取资源和数据，做到了板相关代码和驱动代码的分离，使得驱动具有更好的可扩展性和跨平台性。
3. 让一个驱动支持多个设备。例如DM9000的驱动只有一份，但是我们可以在板级添加多份DM9000的platform_device，他们都可以与唯一的驱动匹配。

在Linux 3.x之后的内核中，DM9000驱动实际上已经可以通过设备树的方法被枚举。



## 5. demo

关注 globalfifo/ch12/globalfifo.c ，来自宋宝华的《Linux设备驱动开发详解》中的demo
