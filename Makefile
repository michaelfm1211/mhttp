CFLAGS = -Wall -Wextra -Werror -pedantic -ansi

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

.PHONY: all
all: CFLAGS += -O3
all: mhttp

.PHONY: dev
dev: CFLAGS += -O0 -g -fsanitize=address -fsanitize=undefined
dev: mhttp

.PHONY: clean
clean:
	rm -rf $(OBJS) http *.dSYM

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

mhttp: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
