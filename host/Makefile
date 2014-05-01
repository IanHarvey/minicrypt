# Setup -------------------------------

VPATH= .:../src
CC=gcc
CPPFLAGS= -I../src
CFLAGS= -Wall -Werror -Os $(CPPFLAGS)

default: all
TARGETS=

# Individual targets ------------------

sha2_mini_test: sha2_mini.c sha2_mini.h
	$(CC) $(CFLAGS) -DTEST_HARNESS -o $@ $<
TARGETS += sha2_mini_test

aes_mini_test: aes_mini.c aes_mini.h
	$(CC) $(CFLAGS) -DTEST_HARNESS -o $@ $<
TARGETS += aes_mini_test

aes_mini_enc_test: aes_mini.c aes_mini.h
	$(CC) $(CFLAGS) -DAESMINI_ENCRYPT_ONLY -DTEST_HARNESS -o $@ $<
TARGETS += aes_mini_enc_test

aes_mini_enc128_test: aes_mini.c aes_mini.h
	$(CC) $(CFLAGS) -DAESMINI_ENCRYPT_ONLY -DAESMINI_128BIT_ONLY -DTEST_HARNESS -o $@ $<
TARGETS += aes_mini_enc128_test

aesccm_mini_test: aesccm_mini.c aes_mini.o
	$(CC) $(CFLAGS) -DTEST_HARNESS -o $@ $^
TARGETS += aesccm_mini_test

# -------------------------------------

all: $(TARGETS)

clean:
	rm -f $(TARGETS) *.o

# Running tests

.PHONY: test

test: $(TARGETS)
	./sha2_mini_test
	./aes_mini_test
	./aes_mini_enc_test
	./aes_mini_enc128_test
	./aesccm_mini_test
