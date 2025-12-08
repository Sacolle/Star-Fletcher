
export STARPU_CFLAGS := $(shell pkg-config --cflags starpu-1.4)
export STARPU_LDLIBS := $(shell pkg-config --libs starpu-1.4)

CFLAGS := $(STARPU_CFLAGS) -Wall
LDLIBS += $(STARPU_LDLIBS) -lm

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
export INCLUDEDIR = src/includes

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o,$(SRCS))

.PHONY: all clean run print test debug

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(LDLIBS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -I $(INCLUDEDIR)

run: $(BIN)
	@echo "Runing $(BIN)."
	./$(BIN) VTI 16 16 16 4 12.5 12.5 12.5 1 3.0 2

run2: $(BIN)
	@echo "Runing $(BIN)"
	./$(BIN) VTI 64 64 64 4 12.5 12.5 12.5 0.1 1 4

debug: $(BIN)
	gdb --args ./$(BIN) VTI 16 16 16 4 12.5 12.5 12.5 1 6.0 2

print:
	@echo "Sources: $(SRCS)"
	@echo "Objects: $(OBJS)"

# need to pass the commands directly inline
test:
	$(MAKE) -C tests 

clean:
	rm -f $(BIN) $(OBJS)
