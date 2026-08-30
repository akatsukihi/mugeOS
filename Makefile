###################
# 核心作用:
# 将C++源文件和汇编源文件编译为32位目标文件（.o），
# 通过链接脚本（linker.ld）将它们拼装成不依赖任何操作系统的纯二进制内核镜像（mgkernel.bin），
# 并提供将其安装到系统的伪目标
###################


# ==========================================
# 1. 变量定义区（编译与链接参数）
# ==========================================

# GPPPARAMS: 传递给 C++ 编译器（g++）的参数标志
# -m32: 强制生成 32 位 x86 代码（因为内核在 32 位保护模式下启动）。
# -fno-use-cxa-atexit: 禁用 __cxa_atexit 运行时函数（C++ 析构默认需要标准库支持，裸机环境下没有标准库）。
# -nostdlib: 不链接标准 C/C++ 库（如 glibc、iostream 等），只依赖自己的内核代码。
# -fno-builtin: 禁用 GCC 内置函数优化（防止编译器将循环强行优化为调用标准库的 memcpy/memset）。
# -fno-rtti: 禁用 C++ 运行时类型识别（Run-Time Type Information，如 typeid 和 dynamic_cast，需要标准库支持）。
# -fno-exceptions: 禁用 C++ 异常处理机制（try/catch 需要复杂的运行时支持结构，裸机下无法使用）。
# -fno-leading-underscore: 禁止在编译出的 C/C++ 符号前自动加下划线，确保符号与汇编文件中的名称严格一致（如 kernelMain 而非 _kernelMain）。
# -Wno-write-strings: 忽略将字符串字面量赋值给非 const char* 时产生的警告。
GPPPARAMS = -m32 -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore -Wno-write-strings

# ASPARAMS: 传递给汇编器（as）的参数
# --32: 告诉 GNU Assembler 生成 32 位的目标文件。
ASPARAMS = --32

# LDPARAMS: 传递给链接器（ld）的参数
# -melf_i386: 指定生成 32 位 ELF 格式（i386 架构）的可执行镜像。
LDPARAMS = -melf_i386

# objects: 编译内核所需的所有目标文件列表
objects = loader.o kernel.o


# ==========================================
# 2. 隐式模式规则区（自动化编译）
# ==========================================

# 规则：如何将任何 .cpp 源文件编译为对应的 .o 目标文件
# % 是通配符；$@ 代表目标文件（如 xx.o）；$< 代表第一个依赖文件（如 xx.cpp）
%.o: %.cpp
		g++ $(GPPPARAMS) -o $@ -c $<
# 规则：如何将任何 .s 汇编源文件编译为对应的 .o 目标文件
%.o: %.s
		as $(ASPARAMS) -o $@ $<


# ==========================================
# 3. 核心构建与安装目标区
# ==========================================

# 目标：生成最终的内核镜像文件 mgkernel.bin
# 依赖于：链接脚本 linker.ld 以及所有的目标文件 ($(objects))
# 命令解读：
#   $< 代表依赖项列表中的第一个文件（即 linker.ld，通过 -T 传递给 ld）
#   $@ 代表目标文件名（即 mgkernel.bin）
#   $(objects) 会展开为 loader.o kernel.o
mgkernel.bin: linker.ld $(objects) 
		ld $(LDPARAMS) -T $< -o $@ $(objects)

# 伪目标：安装内核镜像
# 依赖于：mgkernel.bin（如果 mgkernel.bin 没生成，会先触发编译）
# 作用：以 root 权限将生成的 mgkernel.bin 复制到系统的 /boot/ 目录下，供引导程序（如 GRUB）读取。
install: mgkernel.bin
		sudo cp $< /boot/mgkernel.bin