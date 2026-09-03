#ifndef __GDT_H
#define __GDT_H

#include "types.h"

class GlobalDescriptorTable{
    public:
    class SegmentDescriptor{
        private:
            uint16_t limit_lo;
            uint16_t base_lo;
            uint8_t base_hi;
            uint8_t type;
            uint8_t flags_limit_hi;
            uint8_t base_vhi;
        public:
            SegmentDescriptor(uint32_t base, uint32_t limit, uint8_t type);
            uint32_t Base();
            uint32_t Limit();

    } __attribute__((packed));

    SegmentDescriptor nullSegmentSelector;
    SegmentDescriptor unusedSegmentSelector;
    SegmentDescriptor codeSegmentSelector;
    SegmentDescriptor dataSegmentSelector;

    public:
        GlobalDescriptorTable();
        ~GlobalDescriptorTable();

        uint16_t CodeSegmentSelector();
        uint16_t DataSegmentSelector();
};

/**
           
一、GDT的核心作用

1. 分段管理（Segmentation）
   GDT 内部包含多个表项，每个表项叫做一个段描述符（Segment Descriptor）。
   每个描述符里记录了：
    基地址（Base）：这一段内存从哪里开始。
    段界限（Limit）：这一段内存有多大（防止越界访问）。
    属性（Attributes）：这一段是代码段还是数据段？是否可读可写
2. 权限控制与保护（Privilege Levels）
   GDT 里的每个段描述符都标注了安全级别——DPL（Descriptor Privilege Level），
   范围是 0 到 3：
    Ring 0（最高权限）：通常分配给内核代码和数据。
    Ring 3（最低权限）：分配给用户态的应用程序。
    当 CPU 尝试访问某个段时，会硬件级别检查当前的特权级（CPL）是否满足要求，防止普通应用篡改内核数据。
3. 现代系统中的“平坦模型”（Flat Model）
   现代操作系统（如 Linux、Windows）其实“架空”了 GDT 的分段功能。
    现代系统使用页表（Paging）来管理内存，而不是分段。为了绕过分段，内核在 GDT 中配置了所谓的平坦模型：把内核和用户态的段基地址都设为 0，段界限都设为最大值（4GB 或整个 64 位空间）。
    GDT 并没有把内存划分为“内核区”或“应用区”，而是设置了几个重叠覆盖整个内存空间的“大通道”，有的通道标为 Ring 0（内核用），有的标为 Ring 3（用户用），具体的内存隔离交给了后面的页表去精细化处理。

            
二、GDTR 寄存器（全局描述符表寄存器）、段寄存器

1. GDTR 寄存器
    GDT 本身是操作系统在物理内存中开辟的一块表格空间。
    内核在启动时，在内存中构建好 GDT，然后执行一条特权指令 LGDT（Load GDT），把这段内存的首地址和表格长度加载到 GDTR 寄存器中。从此，CPU 就知道了 GDT 的驻留位置。

2. 段寄存器（CS, DS, SS, ES, FS, GS）
    在保护模式下，这些老式段寄存器的含义变了。它们不再直接存储内存地址，而是存储段选择子（Segment Selector）。段选择子本质上是一个“索引/指针”。
    CS（Code Segment）：代码段寄存器。
    DS（Data Segment）：数据段寄存器。
    SS（Stack Segment）：栈段寄存器。
    当 CPU 执行一条指令，或者去读写内存时，它会查看对应的段寄存器。
    第一步：CPU 看到 CS 寄存器里的值是 0x0008（转换为二进制，代表 GDT 的第 1 号项）。
    第二步：CPU 根据 GDTR 找到 GDT 表，取出第 1 号段描述符。
    第三步：CPU 检查该描述符的属性。如果这是一个内核代码段，而当前执行的程序只有用户态特权，CPU 会立刻触发 General Protection Fault（通用保护错误，即蓝屏或崩溃）。
    第四步：通过检查后，CPU 将描述符中的基地址与指令中的偏移地址相加，得到线性地址。
3. CPU 的流水线与隐藏寄存器（描述符高速缓存）
    如果每次内存访问 CPU 都要去查一次内存中的 GDT，系统会慢得像乌龟。
    因此，CPU 内部为每个段寄存器都配有一个隐藏的、不可见的“描述符高速缓存寄存器”。
    一旦你把选择子送入 CS 或 DS，CPU 就会自动去内存 GDT 中把对应的 8 字节描述符读出来，缓存在这个隐藏寄存器中。
    除非段寄存器被重新赋值，否则后续的内存访问直接读取缓存，实现零延迟。

三、在64位x86-64时代，GDT的作用
    GDT依然在，只是作用被进一步弱化了。
    在 64 位长模式（Long Mode）下，Intel/AMD 彻底取消了大部分代码段和数据段的“基地址”和“界限”检查（强制全平坦，基地址永远视作 0）。
    但是它在现代 CPU 中依然扮演着两个无法替代的角色：
        1.特权级切换：CPU 依然需要通过 CS 寄存器指向的 GDT 表项来判断当前 CPU 究竟是处于 Ring 0（内核态） 还是 Ring 3（用户态）。
        2.TSS（任务状态段）：GDT 还用来存储一种特殊的描述符叫 TSS，现代操作系统用它来找到内核栈（Kernel Stack）的地址，从而在发生硬件中断或系统调用时，能够安全地从用户态切换回内核态。
**/

#endif
