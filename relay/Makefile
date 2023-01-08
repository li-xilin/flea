CC = gcc
LINK = ld
CFLAGS = -g -O0 -I ../include
LDFLAGS = -laxnet -laxcore -lpthread


TARGET = relay
OBJS = main.o uidpool.o server.o client.o rstack.o hostinf.o preboot.o bootloader.o

all: $(TARGET)

$(TARGET): $(OBJS) 
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean:
	$(RM) $(TARGET) $(OBJS)

.PHONY: all clean
