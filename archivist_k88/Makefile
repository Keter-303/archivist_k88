TARGET_TEST = k88_test
TARGET_CLI = k88

SRC_LIB = src/compressor.c src/extractor.c
SRC_TEST = tests/tests.c
SRC_CLI = src/k88_main.c

CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = -lz

all: $(TARGET_TEST) $(TARGET_CLI)

$(TARGET_TEST): $(SRC_LIB) $(SRC_TEST)
	$(CC) $(CFLAGS) $(SRC_LIB) $(SRC_TEST) -o $(TARGET_TEST) $(LDFLAGS)
	@echo "Виконавчий файл для тесту створено: $(TARGET_TEST)"

$(TARGET_CLI): $(SRC_LIB) $(SRC_CLI)
	$(CC) $(CFLAGS) $(SRC_LIB) $(SRC_CLI) -o $(TARGET_CLI) $(LDFLAGS)
	@echo "CLI інструмент створено: $(TARGET_CLI)"

test: $(TARGET_TEST)
	@echo "Запуск тесту..."
	./$(TARGET_TEST)

run: test

help-cli: $(TARGET_CLI)
	./$(TARGET_CLI) --help

clean:
	rm -f $(TARGET_TEST) $(TARGET_CLI)

.PHONY: all run clean