AS = nasm
CC = i686-w64-mingw32-gcc
LINK = ld
OBJCOPY = objcopy

ASFLAGS = -fwin -Ox
CFLAGS = -Wall -fno-builtin -Wno-builtin-declaration-mismatch \
		-std=c99 -ffreestanding -nostdlib -nodefaultlibs \
		-Wno-strict-aliasing
CFLAGS += -I $(ROOT)/include
LDFLAGS =  -e _start -m i386pe
