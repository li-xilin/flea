ROOT = ../..
include ../config.mak

TARGET = bootloader.gen.c
DEBUG_TARGET = bootloader_debug.exe
TEST_TARGET = bootloader_test.exe

OBJS = init.o dld.o boot.o sys.o

all: $(TARGET)

$(TARGET): bootloader.bin
	xxd -i $< $@

bootloader.bin: bootloader.pe
	$(OBJCOPY) -O binary -j .text $^ $@
	@stat -c "output size is %s bytes." $@

bootloader.pe: CFLAGS += -Os -DNDEBUG
bootloader.pe: $(OBJS)
	$(LINK) $(LDFLAGS) -o $@ -\( $^ -\)

$(DEBUG_TARGET): CFLAGS += -O0 -g
$(DEBUG_TARGET): ASFLAGS += -DDEBUG
$(DEBUG_TARGET): $(OBJS)
	$(CC) -o $@ $^ 

$(TEST_TARGET): winmain.c $(TARGET)
	$(CC) -o $@ $^ -g


dld.o: dld.asm
	$(AS) $(ASFLAGS) -o $@ $^
sys.o: sys.asm
	$(AS) $(ASFLAGS) -o $@ $^
init.o: init.asm
	$(AS) $(ASFLAGS) -o $@ $^

disas: bootloader.bin
	ndisasm -b32 $^

clean:
	$(RM) *.o *.exe *.pe *.bin $(TARGET)

.PHONY: all clean disas
