all clean:
	$(MAKE) -C kernel/bootloader $@
	$(MAKE) -C kernel/dispatcher $@
	$(MAKE) -C libflea $@
	$(MAKE) -C flead $@
	$(MAKE) -C flea $@
	$(MAKE) -C relay $@
	$(MAKE) -C modules $@

.PHONY: all clean
