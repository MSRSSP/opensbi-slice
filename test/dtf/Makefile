default: sifive_kernel.dtb sifive_hello.dtb
help:
	echo "# Make sure your have <target>.dts;\nmake <target>.dtb;"

%.dtb: %.dts 
	dtc -I dts -O dtb $<  > $@

clean:
	rm -rf *.dtb