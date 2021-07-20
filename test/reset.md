## System-wise vs per-cpu reset test
* Checkout zz-reset branch 
* Checkout qemu zz-reset branch 

* Use runkernel.sh script to launch two domains; \
In domain 1,`reboot -f` will reset all; \
In domain 2, `reboot -f` tries to only reset harts in its own domain; [Under development] -> [kernel crashes in domain 2 and could be reset by domain 1] -> [reboot works for domain 2]


### QEMU change list
1. hw/riscv/sifive_u.c registers a core-granularity reset unit at 0x100000.
1. hw/cpu/sifive_d_reset.c defines the core-granularity reset unit.
1. softmmu/runstate.c defines how to handle core-granularity reset by identify domain_reset event and a reset_hart_id var.
1. Support pause_all_vcpus while keeping clock enabled; 

Issues:
1. Need to pause_all_vcpus instead of pause_vcpu(cpu); This may cause side effect to other domains; 
1. Clock cannot stop. The rebooted domain may raise an error message due to timeout when waiting for reboot before actual reboot;
1. reboot is obviously slower when we reset CPU one by one per write to an unique offset in the reset unit; 

### OpenSBI change list
1. lib/sbi/d_fdt.c remove syscon-reset unit not belonging to the domain; Linux [syscon-reboot driver](https://github.com/torvalds/linux/blob/master/drivers/power/reset/syscon-reboot.c) would parse the passed device tree and register syscon-reset unit;
1. lib/utils/reset/d_fdt_reset_domain.c and lib/utils/sys/d_reset.c register the reset in a reset ecall; This is skippable, as a gues does not need M mode for reset;
1. lib/sbi/d_fdt.c add a function to copy kernel image from 0x80200000 (in host domain);
1. lib/sbi/sbi_hsm.c add a domain-wide hsm initialization to allow domain boot hart to start without system boot hart.
1. test/dtf/sifive_kernel.dts defines syscon-reboot unit per cpu; d_fdt.c will use cpu property in that node to determine whether to use it or not;
