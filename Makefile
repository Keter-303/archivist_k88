TARGET = k88_test

SRC_FILES = src/compressor.c src/extractor.c tests/tests.c src/crypto.c


CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = -lz -lcrypto


all: $(TARGET)

$(TARGET): $(SRC_FILES)
	$(CC) $(CFLAGS) $(SRC_FILES) -o $(TARGET) $(LDFLAGS)
	@echo "Executable created: $(TARGET)"


run: $(TARGET)
	@echo "Running test..."
	./$(TARGET)



clean:
	rm -f $(TARGET)

.PHONY: all run clean