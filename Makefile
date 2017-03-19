NAME=ctime_modifier
obj-m += $(NAME).o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc -o changer changer.c -std=c11
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f changer
