ROOT = ../../..
include $(ROOT)/modules/payload.mak

OBJS = main.o
TARGET = ../payload.dll
all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS)
clean:
	$(RM) $(OBJS) $(TARGET)

.PHONY: all clean

