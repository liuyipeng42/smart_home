CURRENT_PATH := $(shell pwd)
KERNELDIR := /home/lyp/Codes/I.MX6ULL/linux-imx-rel_imx_4.1.15_2.1.0_ga_alientek
CROSS_COMPILE := arm-linux-gnueabihf-

all:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH)/drivers ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) modules
	$(MAKE) -C drivers_test
	find $(CURRENT_PATH)/drivers -type f \( -name "*.o" -o -name "*.mod.c" -o -name "*.mod.o" -o -name "*.order" -o -name ".*.cmd" -o -name "Module.symvers" \) -delete
	rm -rf $(CURRENT_PATH)/drivers/.tmp_versions

	mkdir -p $(CURRENT_PATH)/drivers/build
	mv $(CURRENT_PATH)/drivers/*.ko $(CURRENT_PATH)/drivers/build

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH)/drivers clean
	$(MAKE) -C drivers_test clean
