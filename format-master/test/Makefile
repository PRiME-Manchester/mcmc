CFLAGS += -I../src -std=c99 -Wall -pedantic -Wunused -Wstrict-prototypes -Wmissing-prototypes
LDFLAGS += 

all: testharness perftest my_test
	./testharness

format.o: ../src/format.c
	$(CC) $(CFLAGS) -c $< -o $@
	
testharness: testharness.o format.o
	$(CC) $(LDFLAGS) testharness.o format.o -o testharness

perftest: performance.o format.o
	$(CC) $(LDFLAGS) performance.o format.o -o perftest

my_test: my_test.o format.o
	$(CC) $(LDFLAGS) my_test.o format.o -o my_test

clean:
	rm -f testharness
	rm -f perftest
	rm -f my_test
	rm -f *.o

what:
	@echo "make targets:"
	@echo "   all         -- all known targets (the default), then runs the test harness"
	@echo "   testharness -- the format test program"
	@echo "   perftest    -- runs some float performance tests"
	@echo "   clean       -- deletes all build artifacts"

