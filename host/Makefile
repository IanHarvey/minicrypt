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

mpiadd_mini_test: mpiadd_mini.c
	$(CC) $(CFLAGS) -DTEST_HARNESS -o $@ $^
TARGETS += mpiadd_mini_test

mpisub_mini_test: mpisub_mini.c
	$(CC) $(CFLAGS) -DTEST_HARNESS -o $@ $^
TARGETS += mpisub_mini_test

mpimul_mini_test: mpimul_mini.c
	$(CC) $(CFLAGS) -DTEST_HARNESS -o $@ $^
TARGETS += mpimul_mini_test

MPI_OBJS= mpiadd_mini.o mpisub_mini.o mpimul_mini.o mpiutil_mini.o

f25519add_mini_test: f25519add_mini.c f25519util_mini.o $(MPI_OBJS)
	$(CC) $(CFLAGS) -DTEST_HARNESS -o $@ $^
TARGETS += f25519add_mini_test

f25519sub_mini_test: f25519sub_mini.c f25519add_mini.o f25519util_mini.o $(MPI_OBJS)
	$(CC) $(CFLAGS) -DTEST_HARNESS -o $@ $^
TARGETS += f25519sub_mini_test

f25519mul_mini_test: f25519mul_mini.c f25519util_mini.o $(MPI_OBJS)
	$(CC) $(CFLAGS) -DTEST_HARNESS -o $@ $^
TARGETS += f25519mul_mini_test

F25519_OBJS= f25519add_mini.o f25519sub_mini.o f25519mul_mini.o f25519util_mini.o $(MPI_OBJS)

curve25519_mini_test: curve25519_mini.c $(F25519_OBJS)
	$(CC) $(CFLAGS) -DTEST_HARNESS -o $@ $^
TARGETS += curve25519_mini_test

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
	./mpimul_mini_test
	./mpiadd_mini_test
	./mpisub_mini_test
	./f25519add_mini_test
	./f25519sub_mini_test
	./f25519mul_mini_test
	./curve25519_mini_test

