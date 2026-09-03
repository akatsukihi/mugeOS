#include "gdt.h"

GlobalDescriptorTable::GlobalDescriptorTable():nullSegmentSelector(0,0,0),
unusedSegmentSelector(0,0,0),
codeSegmentSelector(0,64*1024*1024,0x9A),
dataSegmentSelector(0,64*1024*1024,0x92){
/**          
+---------------------------------------------------+
|                                                   |
|                  GDTR 寄存器                      |
| (存储 GDT 在内存中的 [基地址] 和 [表界限])          |
+---------------------------------------------------+
                         |
                         v 指向
+---------------------------------------------------+
|               内存中的 GDT 表                      |
|  [段描述符 0] -> 空 (Null)                         |
|  [段描述符 1] -> 内核代码段 (Base=0, Limit=4G, R0) |  <---+
|  [段描述符 2] -> 内核数据段 (Base=0, Limit=4G, R0) |      |
|  [段描述符 3] -> 用户代码段 (Base=0, Limit=4G, R3) |      |
+---------------------------------------------------+     |
                         ^                                | 硬件自动

                         | 索引 (Selector)                | 寻址加载
+---------------------------------------------------+     |
|               CPU 段寄存器 (CS / DS / SS)          |     |
| (存储 [段选择子]: 包含索引值 + 当前特权级 CPL)       |----+
+---------------------------------------------------+
**/
    // x86的gdtr寄存器需要6字节，小端
    uint32_t i[2];
    i[0] = sizeof(GlobalDescriptorTable) << 16;
    i[1] = (uint32_t)this;

    asm volatile("lgdt (%0)": :"p"(((uint8_t*)i)+2));
}

GlobalDescriptorTable::~GlobalDescriptorTable(){

}

uint16_t GlobalDescriptorTable::DataSegmentSelector(){
    return (uint8_t*)&dataSegmentSelector - (uint8_t*)this;
}


uint16_t GlobalDescriptorTable::CodeSegmentSelector(){
    return (uint8_t*)&codeSegmentSelector - (uint8_t*)this;
}


GlobalDescriptorTable::SegmentDescriptor::SegmentDescriptor(uint32_t base, uint32_t limit, uint8_t flags){
  
    uint8_t* target = (uint8_t*)this;
    if(limit <= 65536){
        target[6] = 0x40;
    }else{
        if((limit & 0xFFF) != 0xFFF){
            limit = (limit >> 12)-1;
        }else{
            limit = limit >> 12;
        }
        target[6] = 0xC0;
    }
    target[0] = limit & 0xFF;
    target[1] = (limit >> 8) & 0xFF;
    target[6] |= (limit >> 16) & 0xF;

    target[2] = base & 0xFF;
    target[3] = (base >> 8) & 0xFF;
    target[4] = (base >> 16) & 0xFF;
    target[7] = (base >> 24) & 0xFF;

    target[5] = flags;

}



uint32_t GlobalDescriptorTable::SegmentDescriptor::Base(){

    uint8_t* target = (uint8_t*)this;
    uint32_t result = target[7];
    result = (result << 8) + target[4];
    result = (result << 8) + target[3];
    result = (result << 8) + target[2];

    return result;
}

uint32_t GlobalDescriptorTable::SegmentDescriptor::Limit(){

    uint8_t* target = (uint8_t*)this;
    uint32_t result = target[6] & 0xF;
    result = (result << 8) + target[1];
    result = (result << 8) + target[0];

    if((target[6] & 0xC0) == 0xC0){
        result = (result << 12) | 0xFFF;
    }
    return result;
}


/**
由于历史兼容性原因（从 16 位 80286 到 32 位 80386 的演进），Intel 将这个 64 位的数据结构拆得非常零碎，基地址和段界限都被打散在不同的字节中

31          24 23 22 21 20 19      16 15 14 13 12 11        8 7          0
+--------------+--+--+--+--+----------+--+-----+--+----------+------------+

|   基地址      |G |D |0 |A |  段界限  |P | DPL |S |   TYPE   |   基地址    | 高 32 位
|  (31~24位）   |  |B |  |V | (19~16位)|  |     |  |          |  (23~16位) | (Dword 1)
+--------------+--+--+--+--+----------+--+-----+--+----------+------------+

31                                 16 15                                  0
+-------------------------------------+------------------------------------+

|                基地址                |               段界限               | 低 32 位
|               (15~0位)               |              (15~0位)              | (Dword 0)
+-------------------------------------+------------------------------------+

通常用两个 32 位的双字（Dword）来展示它的位分布
把这 64 位拼凑起来，它主要由三大核心部分组成：基地址（32位）、段界限（20位） 和 控制/属性标志位（12位）

1. 基地址（Base Address）—— 占 32 位分布：
打碎分布在低 32 位的 16~31位，以及高 32 位的 0~7位、24~31位。
  含义：该内存段在物理内存（或线性地址空间）中的起始地址。32 位刚好可以寻址 4GB 的全内存空间。
2. 段界限（Segment Limit）—— 占 20 位分布：分布在低 32 位的 0~15位，以及高 32 位的 16~19位。
  含义：该内存段的最大长度/大小。配合下面的 G 位 来决定具体单位。计算方式：如果 G = 0，单位是字节（Byte），最大能表示 2²⁰ = 1MB。如果 G = 1，单位是页（4KB），最大能表示 2²⁰ × 4KB = 4GB（平坦模型通常这么干）。
3. 属性与控制位（Attributes）—— 核心控制区
  这些标志位决定了处理器的硬件保护机制。
  ① G（Granularity，粒度位）—— 高 32 位第 23 位
    0：段界限单位是 1 字节（Byte）。
    1：段界限单位是 4KB（Page）。
  ② D/B（Default / Big，默认操作数大小位）—— 高 32 位第 22 位
    对代码段称为 D 位：
      0：该段内指令默认使用 16 位地址和 16 位操作数。
      1：该段内指令默认使用 32 位地址和 32 位操作数。
    对栈段（SS 寄存器指向的段）称为 B 位：
      0：隐式堆栈操作（如 PUSH/POP）使用 16 位堆栈指针（SP）。
      1：使用 32 位堆栈指针（ESP）。
  ③ L（64-bit Code Segment，64位代码段标志）—— 高 32 位第 21 位
    1：表示这是一个 64 位长模式下的代码段。如果 L=1，则 D 位必须清零。
    0：表示 32 位或 16 位模式。
  ④ AVL（Available，可用位）—— 高 32 位第 20 位
    硬件完全不使用这一位。它是专门留给操作系统内核开发人员使用的，你可以用它来标记任何自定义的信息
  ⑤ P（Present，存在位）—— 高 32 位第 15 位
    1：该段当前已经加载在物理内存中。
    0：该段不在物理内存中（例如被换出到了磁盘交换分区中）。如果此时 CPU 尝试访问这个段，会触发一个 #NP（段不存在）中断，内核会捕获它并把数据从硬盘读回内存。
  ⑥ DPL（Descriptor Privilege Level，描述符特权级）—— 高 32 位第 14~13 位（共 2 位）
    含义：访问这个段所需的最低特权级别（也就是传说中的 Ring 0 ~ Ring 3）。
      00 (Ring 0)：最高特权，内核专享。
      11 (Ring 3)：最低特权，用户态程序。
  ⑦ S（System，系统/数据代码段标志）—— 高 32 位第 12 位
    1：非系统段（常规的用户或内核代码段、数据段）。
    0：系统段（如中断门、调用门、任务状态段 TSS 等特殊硬件结构）。
  ⑧ TYPE（段类型）—— 高 32 位第 11~8 位（共 4 位）
    这一组位的具体含义取决于上面的 S 位。当 S = 1（常规代码/数据段）时，这 4 位分为两种情况：
      如果最高位（第11位）为 0：代表这是一个【数据段】
        第10位 (E)：扩展方向。0 = 向上扩展（常规数据段）；1 = 向下扩展（通常用于栈段）。
        第9位 (W)：可写位。0 = 只读；1 = 可读可写。
        第8位 (A)：已访问位（Accessed）。只要 CPU 读写过这个段，硬件就会自动把这一位置 1。内核可以用它来做内存置换算法。
      如果最高位（第11位）为 1：代表这是一个【代码段】
        第10位 (C)：依从位（Conforming）。0 = 非依从（普通情况，低特权代码不能直接跳转进来）；1 = 依从段（允许低特权代码跳转进来执行，但执行时保持低特权）。
        第9位 (R)：可读位。0 = 只能执行，不能读取（防反汇编）；1 = 既能执行也能读取。
        第8位 (A)：已访问位（Accessed）。同样由 CPU 硬件自动置 1。

现代操作系统的“全平坦模型”
  现代操作系统（Linux/Windows）在初始化 GDT 时，通常会配置两个最核心的常规描述符（一个内核代码段，一个内核数据段）。它们的二进制配置翻译过来就是：
    1.内核代码段描述符（Kernel Code Descriptor）
      Base = 0x00000000
      Limit = 0xFFFFF 且 G = 1（即大小为 0xFFFFF × 4KB = 4GB）
      S = 1 (常规段)，TYPE = 1010 (代码段、可读、未访问)
      DPL = 00 (Ring 0 内核级)
    2.内核数据段描述符（Kernel Data Descriptor）
      Base = 0x00000000
      Limit = 0xFFFFF 且 G = 1（4GB）
      S = 1 (常规段)，TYPE = 0010 (数据段、可写、未访问)
      DPL = 00 (Ring 0 内核级)
  通过这种设计，整个 4GB 内存融合成了一个大段，彻底淡化了“分段”的界限，从而全面拥抱“分页管理”。
**/
