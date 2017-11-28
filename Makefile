obj-m += DriverMorse.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	$(CC) DriverMorse.c -o DriverMorse
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
