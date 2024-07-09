# Seismological-Bureau

使用重新编译的内核（文件为rk3588-sd-ubuntu-jammy-desktop-6.1-arm64-20240521(self-kernel).img）作为操作系统安装到nanopiR6S上。

```bash
#换源
sudo cp /etc/apt/sources.list /etc/apt/sources.list.org
$ sudo sed -i -e 's/ports.ubuntu.com/mirrors.ustc.edu.cn/g' /etc/apt/sources.list
sudo apt-get update

#xdp所需核心库
git clone --recurse-submodules https://github.com/xdp-project/xdp-tutorial.git 
sudo apt install clang llvm libelf-dev libpcap-dev build-essential libc6-dev
sudo apt install linux-tools-common linux-tools-generic
sudo apt install tcpdump
sudo apt install m4

#自编译内核安装内核头文件
sudo dpkg -i /opt/archives/linux-headers-*.deb
#内核头文件会被安装到/usr/src/中
#而对于ubuntu官方固件可以这么安装
sudo apt install linux-headers-$(uname -r)

#进入xdp-tutorial文件夹中
./configure
make
```

会出现的问题及解决方法

1、在make时报错头文件<asm/types.h>找不到

解决方法：其实不止types.h，所有asm下的头文件都找不到，因为在本内核中asm文件夹实际名为asm-generic。

方法一：将/usr/include/asm-generic/映射到/usr/include/asm

```bash
#符号链接
sudo ln -s /usr/include/asm-generic /usr/include/asm
```

方法二：指定头文件<asm/*>位置。

```bash
#要么把#include<asm/*>都改成#include<asm-generic/*>
#要么把#include<asm/*>的库搜索路径改为自己安装的头文件路径，在/usr/src/linux-headers-*/arch/arm64/include下有这个asm文件
```

2、bpftool不支持框架生成

其实如果只使用libbpf库，不使用bpftool该步其实无所谓

```bash
#bpftool官网下载一个较新的bpftool版本，假设为bpftool-v7.4.0-arm64.tar.gz
tar -xvf bpftool-v7.4.0-arm64.tar.gz
#先查看之前bpftool放在哪里了,假设which结果为/usr/sbin/bpftool
which bpftool
#将之前的bpftool备份
mv /usr/sbin/bpftool /usr/sbin/bpftool.old
#把解压缩的bpftool移动过去
mv /path/bpftool /usr/sbin/bpftool
#给可执行权限
sudo chown +x /usr/sbin/bpftool
#检查是否成功
bpftool version
```

3、当前内核不支持AF_XDP套接字

如果用我编译好的内核（rk3588-sd-ubuntu-jammy-desktop-6.1-arm64-20240521(self-kernel).img）就不会出现这个问题。该问题的原因是原先的内核在编译时没启用AF_XDP套接字。

解决办法就是重新编译内核，教程我放在另一个文件中。

4、只有一个lan口可用，另外的两个网口用不了

内核编译的问题

解决方法一（推荐）：内建该驱动模块，详见MAKE.md。

解决方法二：在交叉编译内核后得到的out-modules文件夹中找到对应的驱动模块，例如realtek r8125可以用驱动模块r8169.ko。然后将该驱动模块替换掉操作系统中的r8169.ko驱动模块，重新加载该内核模块即可。

```bash
#替换用的r8169.ko模块在out-modules的lib/modules/6.1.57/kernel/drivers/net/ethernet/realtek下
#被替换的r8169.ko模块在操作系统的/lib/modules/6.1.57/kernel/drivers/net/ethernet/realtek下
```

5、无法实现两个口的桥接

还是内核编译的问题，推荐的解决方法还是内建该驱动模块，详见MAKE.md。

6、将xdp程序挂载到某网口上无法使用

这是因为R6S有一个集成在主板上的网口（LAN），该网口不支持AF_XDP套接字等高级能力，只能用来作管理口。

解决方法：将XDP程序挂载到另外的网口上。

7、将多个子.img打包为一个.img时，打包出来的.img无法使用（即用改.img刷的系统根本无法启动）

这是因为在生成某个子.img时，该子.img对应分区大小超过了打包时默认设置的存储范围

解决方法：详见MAKE.md。