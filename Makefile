TARGET = k88_test
# змінюй помінімуму і не вписуй сюди логіку

SRC_FILES = src/compressor.c tests/tests.c


CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = -lz


all: $(TARGET)

$(TARGET): $(SRC_FILES)
	$(CC) $(CFLAGS) $(SRC_FILES) -o $(TARGET) $(LDFLAGS)
	@echo "Виконавчий файл створено: $(TARGET)"

# Ціль для запуску тесту
run: $(TARGET)
	@echo "Запуск тесту..."
	./$(TARGET)


clean:
	rm -f $(TARGET)

.PHONY: all run clean