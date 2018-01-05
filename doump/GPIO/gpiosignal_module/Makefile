#KDIR = /lib/modules/4.9.38-v7/build
KDIR = /lib/modules/`uname -r`/build

obj-m := gpiosignal_module.o

default:
	$(MAKE) -C $(KDIR) M=$$PWD modules

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean

