# SDK_HOME = /home/yhlee/SDK0220/1.7.3/sysroots
# IOT_HOME = /opt/iot-devkit/1.7.3/sysroots

# PATH := $(PATH):$(SDK_HOME)/x86_64-pokysdk-linux/usr/bin/i586-poky-linux
CC = gcc
ARCH = x86
SROOT = /usr/src/linux-headers-4.4.0-28-generic
# CROSS_COMPILE = i586-poky-linux-
# SROOT=$(SDK_HOME)/i586-poky-linux/
all:
	g++ -o main assignment1.cpp -lpthread
clean:
	rm -f *.ko
	rm -f *.o
	rm -f Module.symvers
	rm -f modules.order
	rm -f *.mod.c
	rm -rf .tmp_versions
	rm -f *.mod.c
	rm -f *.mod.o
	rm -f \.*.cmd
	rm -f Module.markers
	rm -f $(APP) 

