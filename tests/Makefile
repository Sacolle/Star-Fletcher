CRITERION_CFLAGS = $(shell pkg-config --cflags criterion)
CRITERION_LDLIBS = $(shell pkg-config --libs criterion)


ALL_TESTS := $(patsubst $(CURDIR)/%.c, %,$(wildcard $(CURDIR)/*.c))


all: $(ALL_TESTS)
	$(foreach test, $^, ./$(test) --verbose=1)
	
deriv-tests: $(CURDIR)/deriv-tests.c $(PARENT_DIR)/src/derivatives.c $(PARENT_DIR)/src/includes/derivatives.h
	$(CC) -o $@ $^ $(CRITERION_CFLAGS) $(CRITERION_LDLIBS) -I $(PARENT_DIR)/$(INCLUDEDIR)