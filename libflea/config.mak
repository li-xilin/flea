AR            = ar
RM            = rm -f
CC            = gcc

CFLAGS += -Wall --pedantic -Werror -std=c99 -I $(ROOT)/libflea -I $(ROOT)/public -D_DEFAULT_SOURCE

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

