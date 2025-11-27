TARGET_TEST = k88_test
TARGET_CLI = k88

SRC_LIB = src/compressor.c src/extractor.c src/crypto.c
SRC_TEST = tests/tests.c
SRC_CLI = src/k88_main.c

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude

LDFLAGS = -lz -lcrypto

all: $(TARGET_TEST) $(TARGET_CLI)

$(TARGET_TEST): $(SRC_LIB) $(SRC_TEST)
	$(CC) $(CFLAGS) $(SRC_LIB) $(SRC_TEST) -o $(TARGET_TEST) $(LDFLAGS)
	@echo "Test executable created: $(TARGET_TEST)"

$(TARGET_CLI): $(SRC_LIB) $(SRC_CLI)
	# Compile all sources including src/crypto.c and link with -lcrypto
	$(CC) $(CFLAGS) $(SRC_LIB) $(SRC_CLI) -o $(TARGET_CLI) $(LDFLAGS)
	@echo "CLI tool created: $(TARGET_CLI)"

test: $(TARGET_TEST)
	@echo "Running tests..."
	./$(TARGET_TEST)

run: test

help-cli: $(TARGET_CLI)
	./$(TARGET_CLI) --help


clean:
	rm -f $(TARGET_TEST) $(TARGET_CLI) *.o

.PHONY: all run test help-cli clean