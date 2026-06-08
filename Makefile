
export STARPU_CFLAGS := $(shell pkg-config --cflags starpu-1.4)
export STARPU_LDLIBS := $(shell pkg-config --libs starpu-1.4)

CFLAGS := $(STARPU_CFLAGS) -Wall
LDLIBS += $(STARPU_LDLIBS) -lm -lc

# if COMPILE_MODE is not define, the makefile will generate a
# missing separator Error because it will fail to parse the echo line
# AKA, doing this as a throw because the compile_mode should be defined
ifndef COMPILE_MODE
echo $(error, compile mode not defined)
endif

ifeq ($(COMPILE_MODE), release)
CFLAGS += -O3
else
CFLAGS += -O0 -g
endif

export PARENT_DIR := $(CURDIR)

BIN = main

SRCDIR = src
OBJDIR = objs
RESDIR = results
DERIVATIVESDIR = $(SRCDIR)/derivatives
export INCLUDEDIR = src/includes

SRCS_ = argparse.c derivatives.c kernel.c main.c medium.c mem.c vector.c io.c
SRCS := $(addprefix $(SRCDIR)/, $(SRCS_))
OBJS := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o,$(SRCS))

CUDADIR = $(SRCDIR)/cuda
CUDAOBJS = $(OBJDIR)/cuda_kernel.o
# set to native, but can be changed on the system
ARCH ?= native

ifeq ($(CUDA_BACKEND), 1)
	CFLAGS += -DCUDA_BACKEND
	OBJS += $(CUDAOBJS)
	LDLIBS += -lcudart

	NVCC = nvcc
	NVCCFLAGS = -O2 $(STARPU_CFLAGS) -arch=$(ARCH)
endif

# TODO: add CUDA compilation 
# nvcc src/cuda/kernel.cu -o k.o -c -I ./src/includes

.PHONY: all clean run print test debug valgrind lsp

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(LDLIBS) $^ -o $@



# rule to specify dependency
$(OBJDIR)/derivatives.o: $(SRCDIR)/derivatives.c $(DERIVATIVESDIR)/cross-deriv-gen.py
	python3 $(DERIVATIVESDIR)/cross-deriv-gen.py > $(DERIVATIVESDIR)/cross-deriv.gen.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -I $(INCLUDEDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -I $(INCLUDEDIR)

$(OBJDIR)/cuda_kernel.o: $(CUDADIR)/kernel.cu $(SRCDIR)/derivatives/derivatives-impl.h
	@mkdir -p $(OBJDIR)
	$(NVCC) $(NVCCFLAGS) -c $< -o $@ -I $(INCLUDEDIR)

run: $(BIN)
	@mkdir -p $(RESDIR)
	@echo "Runing $(BIN). Out putting at ./$(RESDIR)"
	./$(BIN) TTI 32 32 32 4 12.5 12.5 12.5 0.001 0.5 4 0.01

debug: $(BIN)
	@mkdir -p $(RESDIR)
	gdb --args ./$(BIN) TTI 32 32 32 4 12.5 12.5 12.5 0.001 0.5 4 0.01

lsp:
	bear -- make clean all

valgrind: $(BIN)
	@mkdir -p $(RESDIR)
	valgrind ./$(BIN) TTI 16 16 16 4 12.5 12.5 12.5 0.001 0.1 2 0.01

print:
	@echo "Sources: $(SRCS)"
	@echo "Objects: $(OBJS)"

# need to pass the commands directly inline
test:
	python3 $(DERIVATIVESDIR)/cross-deriv-gen.py > $(DERIVATIVESDIR)/cross-deriv.gen.c
	$(MAKE) -C tests 

clean:
	rm -f $(BIN) $(OBJS) $(CUDAOBJS)
