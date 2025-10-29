CFLAGS += $$(pkg-config --cflags starpu-1.4) -g -pthread -O0 -Wall
LDLIBS += $$(pkg-config --libs starpu-1.4)

BIN = main

SRCDIR = src
OBJDIR = objs
INCLUDEDIR = src/includes

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o,$(SRCS))

.PHONY: all clean run print

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(LDLIBS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ -I $(INCLUDEDIR)

run: $(BIN)
	@echo "Runing $(BIN)."
	./$(BIN) 4 80 10

print:
	@echo "Sources: $(SRCS)"
	@echo "Objects: $(OBJS)"

clean:
	rm -f $(TARGETS) *.o
