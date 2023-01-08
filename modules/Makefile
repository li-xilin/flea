all clean:
	for f in $$(ls -d */); do \
		cmd= $(MAKE) -C $$f $@; \
		echo $$cmd; eval $$cmd; \
	done

.PHONY: all clean
