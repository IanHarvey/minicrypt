VPATH= .:../src
CC=gcc
CPPFLAGS= -I../src
CFLAGS= -Wall -Werror -Os $(CPPFLAGS)

default: all
TARGETS=

# Individual targets

sha2_mini_test: sha2_mini.c sha2_mini.h
	$(CC) $(CFLAGS) -DTEST_HARNESS -o $@ $<
TARGETS += sha2_mini_test

all: $(TARGETS)

# Running tests

.PHONY: test

test: $(TARGETS)
	./sha2_mini_test
