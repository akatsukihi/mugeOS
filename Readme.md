1. c++环境安装
    1.1 sudo apt update
        sudo apt install -y build-essential gdb
        g++ --version

2. wsl2安装os所需编译环境
    2.1 sudo apt update
        sudo apt install -y build-essential nasm qemu-system-x86 grub-pc-bin xorriso

3. 编译运行内核
    3.1 编译
        make loader.o
        make kernel.o
        make mgkernel.bin
        make install
    3.2 运行
        qemu-system-i386 -kernel /boot/mgkernel.bin