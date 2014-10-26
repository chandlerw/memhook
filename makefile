
#CC = gcc
CFLAGS += -g -O0
CFLAGS += -DMEM_HOOK_MT 

TARGET = libmemhook.a

LIBOBJS = mem_hook.o
TESTOBJS = fun2.o main.o

all: ${LIBOBJS}
	$(AR) -rcs $(TARGET) *.o

clean:
	rm *.o ${TARGET}

