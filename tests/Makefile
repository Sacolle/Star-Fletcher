CRITERION_CFLAGS = $(shell pkg-config --cflags criterion)
CRITERION_LDLIBS = $(shell pkg-config --libs criterion)


ALL_TESTS := $(patsubst $(CURDIR)/%.c, %,$(wildcard $(CURDIR)/*.c))

# se o criterion não largar a porta depois de ser finalizado, roda:
# fuser -k 1234/tcp
all: $(ALL_TESTS)
	$(foreach test, $^, ./$(test).out $(ARGS))
	
deriv-tests: $(CURDIR)/deriv-tests.c $(PARENT_DIR)/src/derivatives.c $(PARENT_DIR)/src/includes/derivatives.h
	$(CC) -o $@.out $^ $(CRITERION_CFLAGS) $(CRITERION_LDLIBS) -I $(PARENT_DIR)/$(INCLUDEDIR) -g -O0