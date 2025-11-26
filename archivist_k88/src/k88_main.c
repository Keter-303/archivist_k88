#include "../include/k88_api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void print_help(const char *program_name) {
    printf("Використання: %s [ОПЦІЯ] [ВХІДНИЙ_ФАЙЛ] [ВИХІДНИЙ_ФАЙЛ]\n\n", program_name);
    printf("Опції:\n");
    printf("  --compress, -c       Стиснути файл\n");
    printf("  --extract, -e        Розпакувати архів\n");
    printf("  --help, -h           Показати цю довідку\n\n");
    printf("Приклади:\n");
    printf("  %s --compress file.txt file.k88\n", program_name);
    printf("  %s --extract file.k88 file.txt\n", program_name);
    printf("  %s -c input.dat output.k88\n", program_name);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Помилка: не вказано аргументів.\n");
        print_help(argv[0]);
        return 1;
    }

    const char *option = argv[1];
    int result = 0;

    // Помощь
    if (strcmp(option, "--help") == 0 || strcmp(option, "-h") == 0) {
        print_help(argv[0]);
        return 0;
    }

    // Стиснення
    if (strcmp(option, "--compress") == 0 || strcmp(option, "-c") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Помилка: --compress потребує ВХІДНИЙ_ФАЙЛ та ВИХІДНИЙ_ФАЙЛ\n");
            return 1;
        }
        const char *input_file = argv[2];
        const char *output_file = argv[3];
        
        printf("Стиснення '%s' -> '%s'...\n", input_file, output_file);
        result = compress_file(input_file, output_file);
        
        if (result == 0) {
            printf("✓ Файл успішно стиснено.\n");
        } else {
            fprintf(stderr, "✗ Помилка при стисненні файлу.\n");
            return 1;
        }
        return 0;
    }

    // Розпакування (extract)
    if (strcmp(option, "--extract") == 0 || strcmp(option, "-e") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Помилка: --extract потребує ВХІДНИЙ_ФАЙЛ та ВИХІДНИЙ_ФАЙЛ\n");
            return 1;
        }
        const char *input_file = argv[2];
        const char *output_file = argv[3];
        
        printf("Розпакування '%s' -> '%s'...\n", input_file, output_file);
        result = extract_file(input_file, output_file);
        
        if (result == 0) {
            printf("✓ Архів успішно розпакований.\n");
        } else {
            fprintf(stderr, "✗ Помилка при розпакуванні архіву.\n");
            return 1;
        }
        return 0;
    }

    // Невідома опція
    fprintf(stderr, "Помилка: невідома опція '%s'\n", option);
    print_help(argv[0]);
    return 1;
}