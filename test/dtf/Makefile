default: sifive_kernel.dtb sifive_hello.dtb
help:
	echo "# Make sure your have <target>.dts;\nmake <target>.dtb;"

%.dtb: %.dts 
	dtc -I dts -O dtb $<  > $@

virt.dtb0:
	qemu-system-riscv64 -smp 5 -machine virt,dumpdtb=virt.dtb0

virt.dts0: virt.dtb0
	dtc -I dtb -O dts virt.dtb0  > $@

sifive_u.dtb0:
	qemu-system-riscv64 -smp 5 -machine sifive_u,dumpdtb=sifive_u.dtb0

sifive_u.dts0: sifive_u.dtb0
	dtc -I dtb -O dts sifive_u.dtb0  > $@

clean:
	rm -rf *.dtb