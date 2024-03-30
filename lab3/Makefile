.PHONY: clean directories out_elf run docker_cp

all: 
	make -C lib
	make -C kernel
	make -C bootloader

run:
	qemu-system-aarch64 -M raspi3 -kernel kernel/kernel.img -display none -serial null -serial stdio -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb

debug:
	qemu-system-aarch64 -M raspi3b -kernel kernel/kernel.img -display none -serial null -serial stdio -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb -s -S

run_bootloader:
	qemu-system-aarch64 -M raspi3b -kernel bootloader/bootloader.img -display none -serial null -serial pty -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb

debug_bootloader:
	qemu-system-aarch64 -M raspi3b -kernel bootloader/bootloader.img -display none -serial null -serial pty -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb -s -S

docker_cp:
	docker cp kernel/kernel.elf my_kali:/
	docker cp bootloader/bootloader.elf my_kali:/
	docker cp gdb_script my_kali:/

cpio:
	cd rootfs && find . | cpio -o -H newc > ../initramfs.cpio

build_bin:
	cd ./rootfs/bin/ && aarch64-linux-gnu-gcc -c test_bin.S
	cd ./rootfs/bin/ && aarch64-linux-gnu-ld -Tlinker.ld -nostdlib -nostartfiles -o test_bin test_bin.o
	cd ./rootfs/bin/ && aarch64-linux-gnu-objcopy -O binary test_bin test_bin.img

gen_lib:
	python3 gen_lib.py $(lib_name)


clean:
	rm -rf *.elf
	rm -rf $(BUILD_DIR)
	make -C lib clean
	make -C kernel clean
	make -C bootloader clean