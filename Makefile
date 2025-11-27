TARGET_TEST = k88_test
TARGET_CLI = k88

# Додано src/crypto.c до бібліотечних файлів
SRC_LIB = src/compressor.c src/extractor.c src/crypto.c
SRC_TEST = tests/tests.c
SRC_CLI = src/k88_main.c

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude
# Додано -lcrypto для підтримки функцій OpenSSL
LDFLAGS = -lz -lcrypto

all: $(TARGET_TEST) $(TARGET_CLI)

$(TARGET_TEST): $(SRC_LIB) $(SRC_TEST)
	$(CC) $(CFLAGS) $(SRC_LIB) $(SRC_TEST) -o $(TARGET_TEST) $(LDFLAGS)
	@echo "Виконавчий файл для тесту створено: $(TARGET_TEST)"

$(TARGET_CLI): $(SRC_LIB) $(SRC_CLI)
	# Компілюємо всі вихідні файли, включаючи src/crypto.c, та лінкуємо з -lcrypto
	$(CC) $(CFLAGS) $(SRC_LIB) $(SRC_CLI) -o $(TARGET_CLI) $(LDFLAGS)
	@echo "CLI інструмент створено: $(TARGET_CLI)"

test: $(TARGET_TEST)
	@echo "Запуск тесту..."
	./$(TARGET_TEST)

run: test

help-cli: $(TARGET_CLI)
	./$(TARGET_CLI) --help


clean:
	rm -f $(TARGET_TEST) $(TARGET_CLI) *.o

.PHONY: all run test help-cli clean