include config.mak

TARGET = libflea.a
OBJS = libflea.o win32_error.o
CFLAGS += -fPIC -I ../include

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) -rcs $@ $(OBJS)
clean:
	$(RM) $(OBJS) $(TARGET)

.PHONY: all clean
