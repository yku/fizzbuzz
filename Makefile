EXTRA_CFLAGS += -Wall
CFILES = fizzbuzz.c

obj-m += fizzbuzz.o
sample-objs := $(CFILES:.c=.o)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	sudo insmod fizzbuzz.ko
	sudo mknod --mode=666 /dev/fizzbuzz c `grep fizzbuzz /proc/devices | awk '{print $$1;}'` 0

uninstall:
	sudo rmmod fizzbuzz
	sudo rm -f /dev/fizzbuzz
	
