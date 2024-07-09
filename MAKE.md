## 重新编译内核

1、先服务器上安装docker，用docker构建现成的交叉编译环境（docker在docker-ubuntu-lxde-novnc.tar压缩包内）（https://zhuanlan.zhihu.com/p/651148141 docker安装教程）

2、启动容器

```bash
tar docker-ubuntu-lxde-novnc.tar
cd docker-ubuntu-lxde-novnc
docker build --no-cache -t docker-ubuntu-lxde-novnc .

mkdir ~/work
chown 1000:1000 ~/work
docker run -d --privileged -v /dev:/dev \
    --name docker-ubuntu-lxde-novnc \
    -p 6080:80 \
    -p 5900:5900 \
    -e HTTP_PASSWORD=password \
    -e VNC_PASSWORD=password \
    -e PUID=1000 \
    -e PGID=1000 \
    -e USER=ubuntu \
    -e PASSWORD=ubuntu \
    -v ~/.gitconfig:/home/ubuntu/.gitconfig:ro \
    -v ~/work:/home/ubuntu/work \
    -e RESOLUTION=1280x720 \
    --cpus="$(nproc)" \
    docker-ubuntu-lxde-novnc:latest
  
docker exec -it docker-ubuntu-lxde-novnc /bin/bash

```

3、交叉编译内核

```bash
#这部分是用于交叉编译NanopiR6S的ubuntu内核
#该NanopiR6S交叉编译方法网站在https://wiki.friendlyelec.com/wiki/index.php/NanoPi_R6S/zh#.E6.96.B9.E6.B3.951:_.E4.BD.BF.E7.94.A8Docker.E8.BF.9B.E8.A1.8C.E4.BA.A4.E5.8F.89.E7.BC.96.E8.AF.91下的11.如何编译系统
#如果需要编译其它设备或者其它内核，请找到其对应的设备和内核说明进行编译


git clone https://github.com/friendlyarm/kernel-rockchip --single-branch --depth 1 -b nanopi6-v6.1.y kernel-rockchip
cd kernel-rockchip
export PATH=/opt/FriendlyARM/toolchain/11.3-aarch64/bin/:$PATH
touch .scmversion
# 配置内核
# option1: 加载Linux系统配置
make CROSS_COMPILE=aarch64-linux-gnu- ARCH=arm64 nanopi6_linux_defconfig
# 启动配置界面（该部用于配置内核）(本项目必须重新配置，后边有配置详解)
make CROSS_COMPILE=aarch64-linux-gnu- ARCH=arm64 menuconfig
# 编译内核
make CROSS_COMPILE=aarch64-linux-gnu- ARCH=arm64 nanopi6-images -j$(nproc)
# 编译驱动模块
mkdir -p out-modules && rm -rf out-modules/*
make CROSS_COMPILE=aarch64-linux-gnu- ARCH=arm64 INSTALL_MOD_PATH="$PWD/out-modules" modules -j$(nproc)
make CROSS_COMPILE=aarch64-linux-gnu- ARCH=arm64 INSTALL_MOD_PATH="$PWD/out-modules" modules_install
KERNEL_VER=$(make CROSS_COMPILE=aarch64-linux-gnu- ARCH=arm64 kernelrelease)
[ ! -f "$PWD/out-modules/lib/modules/${KERNEL_VER}/modules.dep" ] && depmod -b $PWD/out-modules -E Module.symvers -F System.map -w ${KERNEL_VER}
(cd $PWD/out-modules && find . -name \*.ko | xargs aarch64-linux-strip --strip-unneeded)
```

在执行完毕后会得到三个重要的文件，kernel.img 、resource.img，和一个文件夹out-modules

4、将交叉编译好的内核打包为一个镜像

```bash
#使用打包脚本sd-fuse，该脚本仅限于对rk_3588处理器使用，若其它设备使用了其它处理器，请参考其对应的设备说明
git clone https://github.com/friendlyarm/sd-fuse_rk3588 -b kernel-6.1.y --single-branch sd-fuse_rk3588-kernel6.1
cd sd-fuse_rk3588-kernel6.1

#这里下载的是R6S ubuntu的源码(我给的文件夹中有这个文件，可以不用wget），你用在什么设备上就去下对应设备的源码。一般设备说明网站中会提供其所有源码等工具的百度网盘地址，其中就有你设备对应内核的源码，wget太慢可以用那个源码
wget http://112.124.9.243/dvdfiles/rk3588/images-for-eflasher/ubuntu-jammy-desktop-arm64-images.tgz
tar xvzf ubuntu-jammy-desktop-arm64-images.tgz

#将ubuntu-jammy-desktop-arm64-images中的kernel.img 、resource.img替换为自己编译的

#在sd-fuse_rk3588最外层目录下执行，打包成一个镜像文件（要注意打包过程中报的信息，尤其是会出现某个子.img大小超过可用空间大小的报错信息，解决方法在下边）
./mk-sd-image.sh ubuntu-jammy-desktop-arm64
#生成的镜像在out文件夹中，命令形式为rk3588-sd-ubuntu-jammy-desktop-6.1-arm64-YYYYMMDD.img
```





```bash
###内核配置(menuconfig)
##这是R6S的配置方法，其它设备根据系统版本不同可能略有差异
#先LOAD默认配置文件.config，在全部配置好后选择SAVE，保存配置到该.config，然后不断EXIT退出即可
1、内建AF_XDP sockets
Networking support ---> Networking options ---> [*] XDP sockets(勾选*)
2、内建网络驱动模块
Device Drivers ---> Network device support ---> Ethernet driver support ---> <*>     Realtek 8169/8168/8101/8125 ethernet support(勾选*)
3、内建桥接模块
Networking support ---> Networking options ---> <*> 802.1d Ethernet Bridging(勾选*)
Networking support ---> Networking options ---> Network packet filtering framework ---> [*] Advanced netfilter configuration(勾选*) & <*> Bridged IP/ARP packets filtering(勾选*)
4、内建BTF
###内建BTF之前需要在编译环境下安装两个库
sudo apt install libelf-dev dwarves

DEBUG_INFO_REDUCED：Kernel hacking ---> Compile-time checks and compiler options ---> [ ] Reduce debugging information (取消勾选*) & [*] Generate BTF typeinfo(勾选*)
```

```bash
###打包.img时报错某个子.img对应分区可用空间不足（结果就是该.img刷到机子上后系统根本无法使用和启动）
##ubuntu-jammy-desktop-arm64下有个parameter.txt
vim parameter.txt
#找到该部分
=======================
CMDLINE: mtdparts=rk29xxnand:0x00002000@0x00004000(uboot),0x00002000@0x00006000(misc),0x00002000@0x00008000(dtbo),0x00008000@0x0000a000(resource),0x00015000@0x00012000(kernel),0x00010000@0x00027000(boot),0x00010000@0x00037000(recovery),0x00780000@0x00047000(rootfs),-@0x007c7000(、)
=======================
#其中哪个子.img报错分区空间不足，就改哪个.img的部分，以0x00008000@0x0000a000(resource)举例来说，@前边的是该子.img对应分区的大小，@后边是该子.img对应分区在整个系统中的起始偏移值。只需要更改对应子.img对应分区的起始偏移值即可更改该子.img可用分区空间大小，更改到合适大小不报错即可。

#需要注意的是，改了某个子.img,也要改在它分区空间位置后边的子.img的位置，比如resource从0x00008000@0x0000a000变成了0x00009000@0x0000b000,那么kernel就要从0x00015000@0x00012000变成0x00015000@0x00013000，对应后边的boot、recovery、rootfs、userdata:grow也都要变。
```

