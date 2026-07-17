
export STARPU_CFLAGS := $(shell pkg-config --cflags starpu-1.4)
export STARPU_LDLIBS := $(shell pkg-config --libs starpu-1.4)

CFLAGS := $(STARPU_CFLAGS) -Wall -fopenmp
LDLIBS += $(STARPU_LDLIBS) -lm -lc 

ifeq ($(RELEASE_MODE), 1)
	CFLAGS += -O3 -DRELEASE
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
	NVCCFLAGS = $(STARPU_CFLAGS) -arch=$(ARCH)

	ifeq ($(RELEASE_MODE), 1)
		NVCCFLAGS += -O2
	else
		NVCCFLAGS += -O0 -g
	endif
endif

ARGS = TTI 200 200 200 8 12.5 12.5 12.5 0.0001 0.001 4 0.0005

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
	./$(BIN) $(ARGS)

debug: $(BIN)
	@mkdir -p $(RESDIR)
	gdb --args ./$(BIN) $(ARGS)

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
