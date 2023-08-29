参考博客：

[深入讲解，linux内核——设备驱动模型](https://zhuanlan.zhihu.com/p/501817318)

[](https://blog.51cto.com/u_13267193/5370840)

# kobject

Linux系统中， kobject结构体是组成设备驱动模型的基本结构，相当于面向对象体系架构中的总基类

- kobject提供了最基本的设备对象管理能力，每一个在内核中注册的kobject对象都对应于sysfs文件系统中的一个目录(而不是文件)
- kobject是各种对象最基本的单元，提供一些公用型服务如：对象引用计数、维护对象链表、对象上锁、对用户空间的表示
- 设备驱动模型中的各种对象其内部都会包含一个kobject

```c
// linux/include/linux/kobject.h
struct kobject {
    const char          *name; //kobject的名字，且作为一个目录的名字
    struct list_head    entry; //连接下一个kobject结构
    struct kobject      *parent; //如果父设备存在，指向父kobject结构体，
    struct kset         *kset; //指向kset集合
    const struct kobj_type  *ktype; //指向kobject的属性描述符
    struct kernfs_node      *sd; /* sysfs directory entry */  //对应sysfs的文件目录
    struct kref             kref; //kobject的引用计数
#ifdef CONFIG_DEBUG_KOBJECT_RELEASE
    struct delayed_work     release;
#endif
    unsigned int state_initialized:1; //表示该kobject是否初始化
    unsigned int state_in_sysfs:1; //表示是否加入sysfs中
    unsigned int state_add_uevent_sent:1;
    unsigned int state_remove_uevent_sent:1;
    unsigned int uevent_suppress:1;
};
```

# kobj_type

使用该kobject设备的共同属性，很多书中简称为ktype，每一个kobject都需要绑定一个ktype来提供相应功能

```c
// kobject 的属性
struct kobj_type {
    void (*release)(struct kobject *kobj); //释放kobject和其占用的函数
    const struct sysfs_ops *sysfs_ops; // ！！重要，提供该对象在sysfs中的操作方法（show和store）
    const struct attribute_group **default_groups; //属性数组的方法，提供在sysfs中以文件形式存在的属性
    const struct kobj_ns_type_operations *(*child_ns_type)(const struct kobject *kobj);
    const void *(*namespace)(const struct kobject *kobj);
    void (*get_ownership)(const struct kobject *kobj, kuid_t *uid, kgid_t *gid);
};


// show()函数用于读取一个属性到用户空间
//  函数的第1个参数是要读取的kobject的指针，它对应要读的目录
//  第2个参数是要读的属性
//  第3个参数是存放读到的属性的缓存区
//  当函数调用成功后，会返回实际读取的数据长度，这个长度不能超过PAGESIZE个字节大小
// store()函数将属性写入内核中
//  函数的第1个参数是与写相关的kobject的指针，它对应要写的目录
//  第2个参数是要写的属性
//  第3个参数是要写入的数据
//  第4个参数是要写入的参数长度，这个长度不能超过PAGE-SIZE个字节大小。只有当拥有属性有写权限时，才能调用store0函数。
const struct sysfs_ops kobj_sysfs_ops = {
    .show   = kobj_attr_show, /*读属性操作函数*/
    .store  = kobj_attr_store, /*写属性操作函数*/
};


// linux/include/linux/sysfs.h
/**
 * struct attribute_group - data structure used to declare an attribute group.
 * @name:	Optional: Attribute group name
 *		If specified, the attribute group will be created in
 *		a new subdirectory with this name.
 * @is_visible:	Optional: Function to return permissions associated with an
 *		attribute of the group. Will be called repeatedly for each
 *		non-binary attribute in the group. Only read/write
 *		permissions as well as SYSFS_PREALLOC are accepted. Must
 *		return 0 if an attribute is not visible. The returned value
 *		will replace static permissions defined in struct attribute.
 * @is_bin_visible:
 *		Optional: Function to return permissions associated with a
 *		binary attribute of the group. Will be called repeatedly
 *		for each binary attribute in the group. Only read/write
 *		permissions as well as SYSFS_PREALLOC are accepted. Must
 *		return 0 if a binary attribute is not visible. The returned
 *		value will replace static permissions defined in
 *		struct bin_attribute.
 * @attrs:	Pointer to NULL terminated list of attributes.
 * @bin_attrs:	Pointer to NULL terminated list of binary attributes.
 *		Either attrs or bin_attrs or both must be provided.
 */
struct attribute_group {
    umode_t         (*is_visible)(struct kobject *,
    const char      *name;
                            struct attribute *, int);
    umode_t         (*is_bin_visible)(struct kobject *,
                            struct bin_attribute *, int);
    struct attribute    **attrs;
    struct bin_attribute    **bin_attrs;
};

// kobj_sysfs_ops作为操作接口挂到kobj_type中（kset_ktype.sysfs_ops）
const struct sysfs_ops kobj_sysfs_ops = {
    .show   = kobj_attr_show,
    .store  = kobj_attr_store,
};
// kset_ktype 是提供给 kset 使用的，关于kset的描述见后续内容
static const struct kobj_type kset_ktype = {
    .sysfs_ops  = &kobj_sysfs_ops,
    .release    = kset_release,
    .get_ownership  = kset_get_ownership,
};
```

![](./drive/pic1.png)



# class

@startuml
start
:new page;
if (Page.onSecurityCheck) then (true)
  :Page.onInit();
  if (isForward?) then (no)
    if (continue processing?) then (no)
      stop
    endif

    if (isPost?) then (yes)
      :Page.onPost();
    else (no)
      :Page.onGet();
    endif
    :Page.onRender();
  endif
else (false)
endif

stop

@enduml
