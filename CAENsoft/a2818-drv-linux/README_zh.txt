                       CAEN A2818 Linux 驱动程序使用说明
                       =================================

Author: 李霞 <Exaos.Lee@gmail.com>
Date: 2010-03-31


Table of Contents
=================
1 简介 
2 文件说明 
3 使用说明 
4 已知问题 


1 简介 
~~~~~~~
  本目录下的源码来自 CAEN A2818 Linux Driver 1.13 版本，略作编译文件的
  修改，以使其能够在 Linux 2.6 内核上正常编译。

2 文件说明 
~~~~~~~~~~~

  + 编译内核必须的文件
    - a2818.c/h -- 内核驱动程序
    - Makefile  -- GNU Make 调用文件

  + 说明文档
    - ~README_zh.org~ -- 本文件
    - ~Readme.txt~  -- CAEN 的说明文档
    - ~ReleaseNotes.txt~ -- 程序发行声明
    - ~CAEN_License_Agreement.txt~ -- 版本授权

  + 其它
    - Makefile.2.4  -- 针对 2.4 内核的原始编译文件
    - Makefile.2.6  -- 针对 2.6 内核的原始编译文件
    - a2818-rc-deb.sh -- 在 Debian 下自动加载 a2818 内核的系统脚本文件

3 使用说明 
~~~~~~~~~~~

  + 内核编译

  make

  + 加载内核
    1. 拷贝编译生成的 a2818.ko 文件到目录 ~/opt/DAQ/lib/kmods/~ 下。
    2. 拷贝 a2818-rc-deb.sh 文件为 ~/etc/init.d/a2818~
    3. 执行如下命令加载内核

  /etc/init.d/a2818 start
    4. 加载成功会，使用命令 ~dmesg | grep a2818~ 会得到如下类似的消息

  [  550.277623] a2818: CAEN A2818 CONET controller driver v1.13s
  [  550.277623] a2818:   Copyright 2004, CAEN SpA
  [  550.278459] a2818: found A2818 adapter at iomem 0xfe0ff000 irq 0, PLX at 0xfe0fe000
  [  550.289936] a2818:   CAEN A2818 Loaded.
  [  550.290144] a2818:   CAEN A2818: 1 device(s) found.
       同时，在 ~/dev/~ 目录下会生成一系列名为 ~a2818_*~ 类似的设备文
       件，这些文件为以后设备操作作准备。

4 已知问题 
~~~~~~~~~~~
  + 在 linux-2.6.32-3-686 内核 (Debian testing) 上编译通过，但无法加载内核模块，
    错误如下：

  [32269.744773] a2818: CAEN A2818 CONET controller driver v1.13s
  [32269.744779] a2818:   Copyright 2004, CAEN SpA
  insmod: error inserting '/opt/DAQ/lib/kmods/a2818.ko': -1 No such device
    疑似与未安装硬件有关。
