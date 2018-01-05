#KDIR = /lib/modules/3.18.7-v7/build
KDIR = /lib/modules/4.9.38-v7/build

obj-m := gpiotimer_module.o

default:
	$(MAKE) -C $(KDIR) M=$$PWD modules

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean

