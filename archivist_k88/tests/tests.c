#include "../include/k88_api.h" 
#include <stdio.h>
#include <stdbool.h> 

#define INPUT_FILE "testdata.txt" 
#define OUTPUT_ARCHIVE "testdata.k88"
#define EXTRACTED_FILE "testdata2.txt" 

int main() {
    int result = 0;
    
    printf("Запуск тесту: Стиснення та Декомпресія.\n");
    printf("--- Крок 1: Стиснення ---\n");

    // 1. СТИСНЕННЯ (з testdata.txt до testdata.k88)
    result = compress_file(INPUT_FILE, OUTPUT_ARCHIVE); 

    if (result != 0) {
        fprintf(stderr, "ПОМИЛКА стиснення: Файл не вдалося архівувати. Код: %d\n", result);
        return 1; 
    }
    printf("Стиснення файлу '%s' до '%s' успішно завершено.\n", 
           INPUT_FILE, OUTPUT_ARCHIVE);

    printf("--- Крок 2: Декомпресія ---\n");

    // 2. ДЕКОМПРЕСІЯ (з testdata.k88 до testdata2.txt)
    result = extract_file(OUTPUT_ARCHIVE, EXTRACTED_FILE);

    if (result != 0) {
        fprintf(stderr, "ПОМИЛКА декомпресії: Архів не вдалося розпакувати. Код: %d\n", result);
        return 1; 
    }

    printf("Декомпресія архіву '%s' до '%s' успішно завершена.\n", 
           OUTPUT_ARCHIVE, EXTRACTED_FILE);      
    
    return 0; 
}