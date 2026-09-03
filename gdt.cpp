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
