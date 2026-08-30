/* 
 * 1.MAGIC魔数常量
 * Multiboot 规范
 * GRUB的启动管理器（引导加载程序）会去硬盘里找操作系统,根据multiboot协议，
 * GRUB在文件头发现魔数常量就可以确定该文件为操作系统内核
 * 2.FLAGS标志常量 
 * 告诉GRUB在加载该文件时，将内存对齐，且返回内存大小信息
 * 3.CHECKSUM校验和常量
 * 数学验证，GRUB会将CHECKSUM与MAGIC和FLAGS三者求和，为0时说明文件未损坏
 * 
 */
.set MAGIC,0x1badb002
.set FLAGS,(1<<0 | 1<<1)
.set CHECKSUM,-(MAGIC + FLAGS)

/*
 * 设置.multiboot区域
 * 方便GRUB识别
 * 常量均占4字节长度
 *
 */
.section .multiboot
    .long MAGIC
    .long FLAGS
    .long CHECKSUM

/*
 * 代码区
 * 执行指令开始
 */
.section .text
.extern mgKernelMain ;// 声明外部函数
.global loader ;// 全局标签loader，使外部GRUB可以找到启动入口

// 启动点,调用内核代码
loader:
    /*
     * 将我们自己建的内存顶端地址($kernel_stack),放入cpu的堆栈指针寄存器(%esp)中
     * C++运行必须依赖“堆栈”（用来存临时变量和调用函数）,设置运行时必须的内存信息
     */
    mov $kernel_stack, %esp 
    /*
     * 把寄存器 %eax 和 %ebx 里的数据压入刚刚建好的堆栈里
     * 电脑启动时，GRUB会把“内存大小”和“硬件信息”存在这两个寄存器里，所以将它们push进堆栈传给C++的函数
     */
    push %eax
    push %ebx
    /*
     * 调用c++内核函数，正式进入c++部分，离开汇编
     * c++中函数应该是死循环(操作系统),正常情况下cpu一直留在c++的函数世界中
     * 万一退出了函数，cpu就会进入下面_stop部分
     */
    call mgKernelMain ;//c++函数调用，此时正式进入c++

_stop:
    cli ;// 关闭所有硬件中断（键盘、鼠标、定时器都不来打扰cpu）
    hlt ;// 让 CPU 进入睡眠停机状态，省电
    jmp _stop ;// 如果被特殊情况唤醒，重新循环_stop再次休眠


/* 内存准备区 */
.section .bss
.space 2*1024*1024  ;// 在内存中切出一块2Mb大小的空白内存
/*
 * 在这个 2MB 区域的最末尾（最顶部）贴上一个标签叫 kernel_stack
 * 计算机的堆栈是倒着生长的（数据从高地址往低地址存），所以在最顶部贴标签。
 * 配合前面的 mov $kernel_stack, %esp，CPU 就会从这个 2MB 的顶部开始往下使用内存。
 */
kernel_stack:
