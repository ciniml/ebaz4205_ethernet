.PHONY: deploy

deploy: images/linux/BOOT.BIN  images/linux/image.ub boot.scr
	cp images/linux/BOOT.BIN $(SDCARD)/
	cp images/linux/image.ub $(SDCARD)/
	cp boot.scr $(SDCARD)/


boot.scr: boot.txt
	mkimage -A arm -O linux -T script -C none -a 0 -e 0 -d boot.txt boot.scr

images/linux/BOOT.BIN images/linux/image.ub: ../../design_1_wrapper.xsa
	petalinux-config --get-hw-description ../..
	petalinux-build
	petalinux-package --boot --fsbl ./build/tmp/deploy/images/zynq-generic/fsbl-zynq-generic.elf --fpga images/linux/system.bit --u-boot --force