AR = ar
RM = rm -f
CC = i686-w64-mingw32-gcc

CFLAGS += -Wall -Werror -std=c99 -I $(ROOT)/include/

DISABLE_DEBUG = no
DISABLE_CASSERT = no

ifeq ($(DISABLE_DEBUG),yes)
	CFLAGS += -O2
else
	CFLAGS += -g -O0
endif

ifeq ($(DISABLE_CASSERT),yes)
	CFLAGS += -DNDEBUG
endif

