
export STARPU_CFLAGS := $(shell pkg-config --cflags starpu-1.4)
export STARPU_LDLIBS := $(shell pkg-config --libs starpu-1.4)

CFLAGS := $(STARPU_CFLAGS) -g -pthread -O0 -Wall -lm
LDLIBS += $(STARPU_LDLIBS) 


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
	./$(BIN) VTI 144 144 144 4 12.5 12.5 12.5 1 50.0 10

debug: $(BIN)
	gdb --args ./$(BIN) VTI 144 144 144 4 12.5 12.5 12.5 1 50.0 10

print:
	@echo "Sources: $(SRCS)"
	@echo "Objects: $(OBJS)"

# need to pass the commands directly inline
test:
	$(MAKE) -C tests 

clean:
	rm -f $(BIN) *.o
