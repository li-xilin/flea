CC = i686-w64-mingw32-gcc
RM = rm -f

CFLAGS += -Wall -Werror -std=c99 -lkernel32 -I ../include

TARGET = flead

OBJS = main.o sysprop.o

all: $(TARGET).dll

$(TARGET).dll: LDFLAGS += -Wl,-s
$(TARGET).dll: CFLAGS += -DBUILD_SERVICE -Os -DNDEBUG -Os
$(TARGET).dll: $(OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

$(TARGET).exe: CFLAGS += -g -O0
$(TARGET).exe: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(OBJS) $(TARGET).exe $(TARGET).dll

.PHONY: all clean
