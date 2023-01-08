CC = gcc

CFLAGS += -I $(ROOT)/include -g -O0 -fPIC

LDFLAGS += -L $(ROOT)/libflea -lflea -laxnet -laxcore -laxkit

