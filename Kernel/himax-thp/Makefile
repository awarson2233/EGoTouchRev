# himax-thp 树外模块。
#   make [KDIR=<内核构建树>] [MO=<产物目录>]
# 产物经 Kbuild 的 MO= 落在 build/kernel/<内核版本>/，源码目录不留产物；也可 cmake --build --preset linux-kmod。
# 内核源码树没有 Module.symvers（没有完整编译）时加 KBUILD_MODPOST_WARN=1，符号在加载时解析。

ifneq ($(KERNELRELEASE),)
obj-m := himax-thp.o thp-slave-dt.o
# Slave 的 DT overlay 编进 thp-slave-dt.ko，见 gaokun3-thp-slave.dtso
thp-slave-dt-y := thp-slave-dt-main.o gaokun3-thp-slave.dtbo.o
else
KDIR ?= /lib/modules/$(shell uname -r)/build
KREL := $(shell cat $(KDIR)/include/config/kernel.release)
MO ?= $(abspath $(CURDIR)/../../build/kernel/$(KREL))

all:
	mkdir -p $(MO)
	$(MAKE) -C $(KDIR) M=$(CURDIR) MO=$(MO) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) MO=$(MO) clean
endif
