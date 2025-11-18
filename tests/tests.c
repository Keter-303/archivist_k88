#include "../include/k88_api.h" 
#include <stdio.h>
#include <stdbool.h> 



#define INPUT_FILE "testdata.txt" // додай "../ " перед назвою файлу для вінди
#define OUTPUT_ARCHIVE "testdata.k88"

/**
 * @brief 
 * @return 
 */
int main() {
    printf("🚀 Запуск мінімального тесту стиснення...\n");


    // 'false' вказує, що шифрування не використовується потім додамо
    int result = compress_file(INPUT_FILE, OUTPUT_ARCHIVE, false);

    
    if (result == 0) {
        printf("Стиснення файлу '%s' до '%s' успішно завершено.\n", 
               INPUT_FILE, OUTPUT_ARCHIVE);
        printf("   Перевірте, чи був створений файл '%s'.\n", OUTPUT_ARCHIVE);
        return 0; // Успіх
    } else {
        fprintf(stderr, "ПОМИЛКА: Стиснення файлу не вдалося. Код помилки: %d\n", result);
        return 1; // Помилка
    }
}