CRITERION_CFLAGS = $(shell pkg-config --cflags criterion)
CRITERION_LDLIBS = $(shell pkg-config --libs criterion)


ALL_TESTS := $(patsubst $(CURDIR)/%.c, %.out, $(wildcard $(CURDIR)/*.c))
CFLAGS = -g3 -O0 -Wno-cpp -lm

# se o criterion não largar a porta depois de ser finalizado, roda:
# fuser -k 1234/tcp
all: $(ALL_TESTS)
	$(foreach test, $^, ./$(test) $(ARGS);)
	
deriv-tests.out: $(CURDIR)/deriv-tests.c $(PARENT_DIR)/src/derivatives.c $(PARENT_DIR)/src/includes/derivatives.h 
	$(CC) -o $@ $^ $(CRITERION_CFLAGS) $(CRITERION_LDLIBS) -I $(PARENT_DIR)/$(INCLUDEDIR) $(CFLAGS)


MEDIUM_TEST_DEP := $(CURDIR)/medium-tests.c $(PARENT_DIR)/src/medium.c $(PARENT_DIR)/src/includes/medium.h 
MEDIUM_TEST_DEP += $(PARENT_DIR)/src/argparse.c $(PARENT_DIR)/src/includes/argparse.h 
MEDIUM_TEST_DEP += $(PARENT_DIR)/src/includes/macros.h $(PARENT_DIR)/src/includes/floatingpoint.h

medium-tests.out: $(MEDIUM_TEST_DEP)
	$(CC) -o $@ $^ $(CRITERION_CFLAGS) $(CRITERION_LDLIBS) -I $(PARENT_DIR)/$(INCLUDEDIR) $(CFLAGS)


KERNEL_TEST_DEP := $(CURDIR)/kernel-tests.c $(PARENT_DIR)/src/derivatives.c $(PARENT_DIR)/src/includes/derivatives.h 
KERNEL_TEST_DEP += $(PARENT_DIR)/src/kernel.c $(PARENT_DIR)/src/mem.c $(PARENT_DIR)/src/vector.c
KERNEL_TEST_DEP += $(PARENT_DIR)/src/medium.c $(PARENT_DIR)/src/argparse.c

kernel-tests.out: $(KERNEL_TEST_DEP)
	$(CC) -o $@ $^ $(CRITERION_CFLAGS) $(CRITERION_LDLIBS) $(STARPU_CFLAGS) $(STARPU_LDLIBS) -I $(PARENT_DIR)/$(INCLUDEDIR) $(CFLAGS)