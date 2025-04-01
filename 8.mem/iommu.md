# iommu组和iommu域

在 Linux IOMMU（输入输出内存管理单元）子系统中，**IOMMU 组（IOMMU Group）** 和
**IOMMU 域（IOMMU Domain）** 是两个关键概念，主要用于管理设备的地址映射和访问权限。

---

## IOMMU 域（IOMMU Domain）

**IOMMU 域**（`struct iommu_domain`）是 **IOMMU 进行地址转换的核心单位**，它定义
了一个独立的 **地址空间**，IOMMU 通过它来管理设备的虚拟地址到物理地址的映射。

### IOMMU 域的作用
- 控制 **设备的内存访问权限**。
- 维护 **虚拟地址（IOVA）到物理地址（PA）的映射关系**。
- **不同的域拥有不同的页表**，可以隔离设备的 DMA 访问。
- 在 **虚拟化（VFIO、PCI 直通）** 场景中，每个虚拟机通常会对应一个独立的 IOMMU 域。

### IOMMU 域的类型
IOMMU 域的类型通常由底层硬件和驱动决定，常见类型包括：
- **IOMMU_DOMAIN_DMA**（通用 DMA 映射模式）
  - 用于 Linux 通用 DMA 映射 API (`dma_map_*()`)。
  - 适用于一般设备，设备的 DMA 地址会经过 IOMMU 转换。

- **IOMMU_DOMAIN_IDENTITY**（直通模式）
  - IOMMU **不会修改地址映射**，即设备的物理地址和 CPU 看到的一致（1:1 映射）。
  - 主要用于性能敏感的场景，如直通 PCI 设备。

- **IOMMU_DOMAIN_UNMANAGED**（手动管理）
  - 由 **驱动程序或用户态** 自行管理映射关系。
  - 适用于 VFIO（虚拟化设备直通）。

- **IOMMU_DOMAIN_BLOCKED**（禁止访问）
  - 设备无法访问任何内存。
  - 主要用于安全性，防止未授权设备进行 DMA 操作。

### 关键 API
```c
// 分配 IOMMU 域
struct iommu_domain *domain = iommu_domain_alloc(&platform_bus_type);

// 在 IOMMU 域中创建映射
iommu_map(domain, iova, phys_addr, size, prot);

// 解除 IOMMU 映射
iommu_unmap(domain, iova, size);

// 释放 IOMMU 域
iommu_domain_free(domain);
```

---

## IOMMU 组（IOMMU Group）

**IOMMU 组**（`struct iommu_group`）是 IOMMU 进行 **设备管理** 的基本单位，**一个
IOMMU 组内的所有设备共享同一个 IOMMU 域**。

### 为什么需要 IOMMU 组？
- **某些设备必须共享 IOMMU 映射**
  - 例如 **PCIe 设备和它的 PCIe 交换桥（PCI Switch）** 可能必须位于同一组，因为
    它们共享相同的 IOMMU 访问权限。
  - **某些 SoC 设备**（比如 GPU 和其相关 DMA 控制器）也可能被分配到同一个 IOMMU 组。

- **安全性（特别是虚拟化场景）**
  - VFIO（虚拟机设备直通）要求 **整个 IOMMU 组必须一起分配给同一个虚拟机**。
  - 如果一个 IOMMU 组内包含多个设备，那么这些设备必须一起分配，不能分割。

### 如何创建和获取 IOMMU 组？
设备注册时，IOMMU 子系统会为它们创建或分配到 IOMMU 组：
```c
// 获取设备所属的 IOMMU 组
struct iommu_group *group = iommu_group_get(dev);
if (!group) {
    pr_err("Failed to get IOMMU group\n");
    return -EINVAL;
}

// 释放 IOMMU 组
iommu_group_put(group);
```

### IOMMU 组的绑定
在 IOMMU 体系下，**所有设备必须通过 IOMMU 组绑定到 IOMMU 域**，这样才能正确访问
DMA 资源：
```c
// 绑定 IOMMU 组到 IOMMU 域
iommu_attach_group(domain, group);

// 解除绑定
iommu_detach_group(domain, group);
```

---

## IOMMU 组与 IOMMU 域的关系
- **一个 IOMMU 组只能绑定到一个 IOMMU 域**。
- **一个 IOMMU 域可以管理多个 IOMMU 组**（但通常每个域只用于一个 IOMMU 组）。
- **所有属于同一个 IOMMU 组的设备必须共享相同的地址映射**。

它们的关系可以归纳如下：
1. IOMMU 组是设备的管理单位，一个 IOMMU 组内的所有设备共享相同的 IOMMU 配置。
2. IOMMU 域是地址映射的管理单位，负责维护虚拟地址（IOVA）到物理地址（PA）的转换
   规则。
3. IOMMU 组必须绑定到一个 IOMMU 域，才能正确使用 IOMMU 进行地址映射。
4. 一个 IOMMU 域可以绑定多个 IOMMU 组，但通常不会这么做，而是每个 IOMMU 组绑定到
   一个独立的 IOMMU 域。

#### 示例关系
| IOMMU 组 | 包含的设备 | 绑定的 IOMMU 域 |
|----------|----------|---------------|
| IOMMU 组 0 | `PCIe 设备 A, PCIe 设备 B` | IOMMU 域 1 |
| IOMMU 组 1 | `SATA 控制器` | IOMMU 域 2 |
| IOMMU 组 2 | `GPU, GPU DMA 控制器` | IOMMU 域 3 |

---

## 真实场景：VFIO（虚拟化设备直通）
在 **KVM/QEMU** 环境中，给虚拟机直通 PCI 设备时，IOMMU 组的概念至关重要：
- **所有属于同一 IOMMU 组的设备必须同时直通**，否则会带来安全风险。
- **如果一个 IOMMU 组包含多个 PCI 设备，用户必须分配整个组**，不能只直通其中一个设备。

例如：
```bash
# 查看 IOMMU 组信息（对于 PCI 设备 0000:00:1f.0）
find /sys/kernel/iommu_groups/ -type l | grep 0000:00:1f.0
```

如果 `0000:00:1f.0` 设备的 IOMMU 组包含多个设备，则必须直通整个组。

---

## 总结
| **概念** | **作用** |
|----------|----------|
| **IOMMU 域（IOMMU Domain）** | 定义一个地址映射空间，管理设备的 DMA 地址转换 |
| **IOMMU 组（IOMMU Group）** | 设备的管理单位，组内设备共享 IOMMU 访问权限 |

- **IOMMU 组用于组织设备**，确保 **必须共享 DMA 访问的设备** 在一个组里。
- **IOMMU 域用于管理地址映射**，多个 IOMMU 组可以共享一个 IOMMU 域。

这些概念主要用于：
- **内存隔离（防止 DMA 攻击）**
- **虚拟化（VFIO 直通 PCI 设备）**
- **地址转换（物理地址和设备地址映射）**

# iommu组/域和dma的关系

IOMMU 域（IOMMU Domain）/ IOMMU 组（IOMMU Group） 和 DMA（直接内存访问）紧密相连，
但它们的作用不同：

- **IOMMU 负责管理和保护设备的 DMA 访问**，它提供 **地址映射（IOVA -> 物理地址）**
  和 **访问控制**。
- **DMA 是设备访问内存的方式**，当 IOMMU 使能时，设备的 DMA 请求会经过 IOMMU 进行
  地址转换和权限检查。

---

## DMA 在无 IOMMU 和有 IOMMU 时的区别
### 无 IOMMU（直接物理地址访问）
当 IOMMU **未启用** 时，设备的 DMA 直接访问 **物理地址**：
```plaintext
设备 DMA 地址 == 物理地址（PA）
```
- 设备直接访问物理内存，容易造成 **内存安全问题（DMA 攻击）**。
- 在嵌入式系统、某些简单 SoC 设备上，这种模式较为常见。

### 有 IOMMU（IOMMU 进行地址转换）
当 IOMMU **启用** 后，设备的 DMA 请求先经过 IOMMU：
```plaintext
设备 DMA 地址（IOVA）  ->  IOMMU  ->  物理地址（PA）
```
- 设备只能访问 **IOMMU 允许的地址范围**，从而实现 **隔离和保护**。
- 设备看到的是 **IOMMU 分配的虚拟地址（IOVA, I/O 虚拟地址）**，而不是物理地址。

---

## IOMMU 域（IOMMU Domain）与 DMA
### IOMMU 域的作用
**IOMMU 域决定了 DMA 地址如何被映射**，它可以：
- **提供一个虚拟地址空间（IOVA），并映射到物理地址**，设备使用 IOVA 进行 DMA 操作。
- **提供权限控制**，防止 DMA 访问未经授权的地址。

### IOMMU 域与 DMA 关系
当设备进行 DMA 时，需要使用 IOMMU 提供的映射：
```c
struct iommu_domain *domain = iommu_domain_alloc(&platform_bus_type);

// 设备分配 IOVA
dma_addr_t iova = dma_map_single(dev, cpu_addr, size, DMA_TO_DEVICE);

// IOMMU 建立映射（IOVA -> PA）
iommu_map(domain, iova, phys_addr, size, prot);
```
- `dma_map_single()` 会 **分配一个 IOVA 地址** 并映射到物理地址。
- `iommu_map()` 负责 **维护 IOMMU 的地址映射表**。

---

## IOMMU 组（IOMMU Group）与 DMA
### 为什么 IOMMU 组会影响 DMA？
- **IOMMU 组内的设备必须共享相同的地址映射**。
- **如果 DMA 设备和其控制器在同一 IOMMU 组内，它们必须共用 IOMMU 配置**。

### IOMMU 组如何影响 DMA ？
当 IOMMU 组绑定到 IOMMU 域时，设备的 DMA 访问受 IOMMU 限制：
```c
struct iommu_group *group = iommu_group_get(dev);
iommu_attach_group(domain, group);
```
这意味着：
1. 组内 **所有设备** 的 DMA **必须共享相同的地址转换规则**。
2. **多个设备共享一个 IOVA 地址空间**，这对于 **PCIe 设备直通** 或 **多个 DMA 设备协作** 很重要。

---

## DMA 和 IOMMU 典型使用场景
### 普通设备 DMA（使用 IOMMU 进行地址转换）
适用于：
- **嵌入式 SoC、服务器**
- **NVMe、GPU、NIC**
- 设备使用 `dma_map_single()` 分配 IOVA 地址。

流程：
```plaintext
设备发起 DMA（IOVA 地址） -> IOMMU 转换（IOVA -> 物理地址） -> 访问内存
```

### VFIO（PCI 直通）
适用于：
- **虚拟化（QEMU/KVM）**
- **GPU 直通、网卡直通**
- **整个 IOMMU 组直通给虚拟机**

流程：
```plaintext
VM 设备（IOVA） -> IOMMU 转换（IOVA -> 物理地址） -> 物理内存
```
所有 **IOMMU 组内的设备必须被一起直通**，不能拆分。

### IOMMU 直通模式（IOMMU_DOMAIN_IDENTITY）
适用于：
- **高性能计算（HPC）**
- **IOMMU 存在但不做地址转换**
- **IOMMU 仅用于访问权限控制**

流程：
```plaintext
设备发起 DMA（物理地址） -> IOMMU 允许直接访问
```

---

## 结论
| **概念** | **IOMMU 组** | **IOMMU 域** | **DMA** |
|----------|-------------|-------------|---------|
| **作用** | 设备管理单位，组内设备共享 IOMMU 规则 | 地址映射管理，提供 IOVA 到物理地址转换 | 设备访问内存的方式 |
| **是否影响 DMA** | 是，组内设备共享 DMA 规则 | 是，IOMMU 域决定 DMA 地址映射 | 是，设备通过 DMA 访问 IOVA |
| **典型使用场景** | PCIe 设备管理、VFIO 直通 | 地址转换、内存保护 | 外设访问内存 |

### **IOMMU 域、IOMMU 组、DMA 是如何连接的？**
1. **IOMMU 组** 定义了 **哪些设备共享相同的 IOMMU 规则**。
2. **IOMMU 域** 定义了 **设备的地址转换规则（IOVA -> 物理地址）**。
3. **DMA 通过 IOMMU 进行地址转换**，使设备能够访问正确的内存区域。
