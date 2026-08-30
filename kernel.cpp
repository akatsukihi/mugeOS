
/***********************************************************************************************
* 没有操作系统、没有C标准库（libc）的裸机环境下，向VGA显存写入字符
* 在32位x86保护模式下，物理内存地址 0xB8000 是 VGA 文本模式的显存映射起始地址（显示 80 列 × 25 行字符）
* 屏幕上的每个字符占用 2 个字节（一个 unsigned short，即 16 位）
* 高8位(0xFF00)，控制字符的前景色和背景色（颜色属性）
* 低8位(0x00FF)，存放字符的 ASCII 码
************************************************************************************************/
void printf(char* s){
    unsigned short* screenOut = (unsigned short*)0xb8000;
    for(int i=0; s[i] !='\0'; i++){
        // 保留显存中原本的颜色设置，仅将低 8 位替换为当前要输出的字符 ASCII 码，从而直接把文字绘制到屏幕上
        screenOut[i] = (screenOut[i] & 0xFF00) | s[i];
    }
}

/*************************************************************************************************
* 调用C++全局构造函数
* 手动遍历并执行所有全局对象/静态对象的 C++ 构造函数
* 普通用户态程序在启动时C运行时库（crt0）会自动调用全局对象的构造函数。
* 但在开发裸机内核时，没有这个运行时库。在linker.ld链接脚本中，定义了 start_ctors 和 end_ctors 两个符号，
* 用于标记 .init_array（存放全局构造函数函数指针列表的段）的起点和终点。
* callConstructors以指针的形式遍历这个数组，依次调用每一个构造函数指针 (*i)()，
* 确保C++全局对象在内核使用前被正确初始化
**************************************************************************************************/
typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors(){
    for(constructor* i = &start_ctors; i != &end_ctors; i++){
        (*i)();
    }
} 

/************************************************************************************************
* 内核入口函数
* 汇编引导代码（loader.s）在完成基本的环境设置后跳转过来的内核主函数
* 接收来自引导程序（GRUB/Multiboot）传递进来的两个关键参数：
* multiboot_structure：指向 Multiboot 信息结构体的指针（包含物理内存大小、命令行参数、驱动盘信息等）
* magicnumber：Multiboot 的魔数（用于校验是否由规范的 bootloader 正确引导）
*************************************************************************************************/
extern "C" void mgKernelMain(void * multiboot_structure , unsigned int magicnumber){
    printf("hello muge, Please complete your study of operating systems.\n");
    while(1);
}