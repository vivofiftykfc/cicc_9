# README

## 编辑说明

makefile编译的，具体命令看makefile文件结尾定义的，反正就是make、make all几个

然后这个有个问题，测试的时候会卡一下，然后弹出来一大堆，可能是不满一个buffer不传数据？再试一试吧。

## 超声避障（slaver_core_for_ultrasound）

在支持openamp`（linux+baremetal）`的系统镜像下测试：

将编译好的`openamp_core0.elf`置于`/lib/firmware`目录下，将测试程序`rpmsg.c`置于`~/`目录下，执行：

```bash
echo start > /sys/class/remoteproc/remoteproc0/state
echo rpmsg_chrdev > /sys/bus/rpmsg/devices/virtio0.rpmsg-openamp-demo-channel.-1.0/driver_override
modprobe rpmsg_char
```

然后编译运行`rpmsg.c`文件即可测试