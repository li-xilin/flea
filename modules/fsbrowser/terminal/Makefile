ROOT = ../../..
include $(ROOT)/modules/terminal.mak

OBJS = main.o
TARGET = ../terminal.so

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS)
clean:
	$(RM) $(OBJS) $(TARGET)

.PHONY: all clean

