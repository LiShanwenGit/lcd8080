KERN_DIR = /home/lsw/licheepi/linux-5.14
all:
	make -C $(KERN_DIR) M=$(shell pwd) modules
clean:
	rm -rf *.order *o *.symvers *.mod.c *.mod *.ko

lcd8080-objs += lcd8080-core.o  lcd8080-io.o  lcd8080-ili9341.o

obj-m += lcd8080.o

