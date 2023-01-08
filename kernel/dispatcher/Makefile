ROOT = ../..
include ../config.mak

TARGET = dispatcher.gen.c
DEBUG_TARGET = dispatcher_debug.exe
TEST_TARGET = dispatcher_test.exe

OBJS = main.o server.o api.o

all: $(TARGET)

$(TARGET): dispatcher.bin
	xxd -i $< $@

dispatcher.bin: dispatcher.pe
	$(OBJCOPY) -O binary -j .text $^ $@
	@stat -c "output size is %s bytes." $@

dispatcher.pe: CFLAGS += -O2 -DNDEBUG
dispatcher.pe: $(OBJS)
	$(LINK) $(LDFLAGS) -o $@ -\( $^ -\)

$(DEBUG_TARGET): CFLAGS = -O0 -g -DDEBUG_BUILD -I ../../include
$(DEBUG_TARGET): $(OBJS)
	$(CC) -o $@ $^ -ldnsapi

$(TEST_TARGET): winmain.c $(TARGET)
	$(CC) -o $@ $^ -g -ldnsapi

disas: dispatcher.bin
	ndisasm -b32 $^

clean:
	$(RM) *.o *.exe *.pe *.bin $(TARGET)

.PHONY: all clean disas
