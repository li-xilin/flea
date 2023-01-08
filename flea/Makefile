CC = gcc
CFLAGS = -g -O0 -I ../include
LDFLAGS = -laxnet -laxcore -lpthread -liniparser -ldl

OBJS = main.o tunnel.o modset.o lshost.o rmhost.o insmod.o run.o rmmod.o lsmod.o sysinfo.o library.o
TARGET = flea

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(TARGET) $(OBJS)

.PHONY: all clean
