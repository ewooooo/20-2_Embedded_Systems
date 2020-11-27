CC = gcc
obj-m := BufferedMem.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	rm -f *. ko
	rm -f *.o
	rm -f *.mod.*
	rm -f .*. cmd

