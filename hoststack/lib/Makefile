libsbi-objs-y+=sbi.o

libsbi-objs-path-y=$(foreach obj,$(libsbi-objs-y),$(build_dir)/lib/$(obj))

libsbi: $(build_dir)/lib/libsbi.a

$(build_dir)/lib/libsbi.a: $(libsbi-objs-path-y)
	$(call compile_ar,$@,$^)


$(build_dir)/%.o: %.c
	$(call compile_cc,$@,$<)
