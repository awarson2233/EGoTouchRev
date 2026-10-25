# himax-thp 树外模块。
#   make KDIR=<已 modules_prepare 的内核源码树>
# 内核源码树没有 Module.symvers（没有完整编译）时加 KBUILD_MODPOST_WARN=1，符号在加载时解析。

ifneq ($(KERNELRELEASE),)
obj-m := himax-thp.o thp-slave-dt.o
# Slave 的 DT overlay 编进 thp-slave-dt.ko，见 gaokun3-thp-slave.dtso
thp-slave-dt-y := thp-slave-dt-main.o gaokun3-thp-slave.dtbo.o
else
KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
endif
