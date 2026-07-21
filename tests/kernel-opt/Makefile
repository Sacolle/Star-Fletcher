CFLAGS := -Wall -fopenmp
LDLIBS += -lm -lc 

ifeq ($(RELEASE_MODE), 1)
	CFLAGS += -O3
else
	CFLAGS += -O0 -g
endif

LDLIBS += -lcudart

ARCH ?= native

NVCC = nvcc
NVCCFLAGS =  -arch=$(ARCH)

ifeq ($(RELEASE_MODE), 1)
	NVCCFLAGS += -O2
else
	NVCCFLAGS += -O0 -g
endif

BIN = main

SRCDIR = ../../src
INCLUDEDIR = ../../src/includes

.PHONY: all clean run print test debug valgrind lsp

all: $(BIN)

$(BIN): main.o kernel.o vector.o
	$(CC) $(CFLAGS) $(LDLIBS) $^ -o $@

main.o: main.c 
	$(CC) $(CFLAGS) -c $^ -o $@ -I $(INCLUDEDIR)

vector.o: $(SRCDIR)/vector.c
	$(CC) $(CFLAGS) -c $^ -o $@ -I $(INCLUDEDIR)

kernel.o: kernel.cu 
	$(NVCC) $(NVCCFLAGS) -c $< -o $@ -I $(INCLUDEDIR)

clean:
	rm *.o
