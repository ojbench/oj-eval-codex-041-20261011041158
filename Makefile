CC = gcc
CFLAGS = -O3 -Wall -Wextra -Werror -DDRIVER
COMMON = mm.o memlib.o fsecs.o fcyc.o clock.o ftimer.o driverlib.o
OBJS = mdriver-oj.o $(COMMON)

all: code
code: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)
mdriver: mdriver.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^
mdriver-oj.o: mdriver.c
	$(CC) $(CFLAGS) -DOJ -Wno-unused-variable -Wno-unused-parameter -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-result -c $< -o $@
$(OBJS) mdriver.o: mm.h memlib.h config.h
# The unmodified course driver predates these compiler diagnostics.
mdriver.o: CFLAGS += -Wno-unused-but-set-variable -Wno-unused-result
test:
	$(CC) -O1 -g -Wall -Wextra -Werror -DDRIVER -fsanitize=address,undefined -I. tests/allocator_test.c mm.c memlib.c -o allocator_test
	./allocator_test
clean:
	rm -f $(OBJS) mdriver.o mdriver code allocator_test
.PHONY: all clean test
