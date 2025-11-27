#include "../include/k88_api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h> // Для функції access(), unlink()
#include <sys/stat.h> // Для перевірки існування

// Максимальна довжина, яку ми підтримуємо для шляхів
#define MAX_PATH_LEN 4096

// --- Функція для парсингу аргументів (-s ПАРОЛЬ) ---

/**
 * Шукає прапорець -s або --secure у командному рядку після argv[2]
 * і повертає відповідний пароль.
 */
static bool parse_secure_args(int argc, char *argv[], const char **password_out) {
    // Починаємо пошук одразу після імені вхідного файлу (argv[2])
    for (int i = 3; i < argc; i++) {
        
        // Знайшли прапорець -s або --secure
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--secure") == 0) {
            
            // Перевіряємо, чи є наступний аргумент (пароль)
            if (i + 1 < argc) {
                *password_out = argv[i + 1];
                return true;
            } else {
                 // Знайшли -s, але немає пароля
                 fprintf(stderr, "Помилка: Опція -s потребує ПАРОЛЬ.\n");
                 return false;
            }
        }
    }
    return false; // -s не знайдено
}


void print_help(const char *program_name) {
    printf("Використання: %s [ОПЦІЯ] [ВХІДНИЙ_ФАЙЛ] [-s ПАРОЛЬ]\n\n", program_name);
    printf("Опції:\n");
    printf("  --compress, -c       Стиснути файл/директорію (Вихід: ВХІДНИЙ_ФАЙЛ.k88)\n");
    printf("  --extract, -e        Розпакувати архів (ВХІДНИЙ_ФАЙЛ повинен бути *.k88)\n");
    printf("  -s, --secure         Використовується для шифрування/дешифрування. Розташовується після ВХІДНОГО_ФАЙЛУ.\n");
    printf("  --help, -h           Показати цю довідку\n\n");
    printf("Приклади:\n");
    printf("  %s -c my_files -s secretkey\n", program_name);
    printf("  %s -e my_files.k88 -s secretkey\n", program_name);
}

// -----------------------------------------------------------
// --- ГЕНЕРАЦІЯ ШЛЯХУ (Без перевірки існування) ---
// -----------------------------------------------------------

/**
 * Генерує базовий шлях для виводу: додає .k88 або видаляє .k88.
 */
char *generate_path_name(const char *input_path, bool is_compress) {
    char *new_path = (char *)malloc(MAX_PATH_LEN);
    if (new_path == NULL) {
        perror("Помилка виділення пам'яті");
        return NULL;
    }

    if (is_compress) {
        // Додаємо .k88
        snprintf(new_path, MAX_PATH_LEN, "%s.k88", input_path);
    } else {
        // Видаляємо .k88
        size_t len = strlen(input_path);
        const char *ext = ".k88";
        size_t ext_len = strlen(ext);

        if (len >= ext_len && strcmp(input_path + len - ext_len, ext) == 0) {
            // Шлях закінчується на .k88, обрізаємо
            snprintf(new_path, len - ext_len + 1, "%s", input_path); 
        } else {
            // Шлях не закінчується на .k88, додаємо суфікс _extracted
            snprintf(new_path, MAX_PATH_LEN, "%s_extracted", input_path);
        }
    }
    return new_path;
}

// -----------------------------------------------------------
// --- ПЕРЕВІРКА УНІКАЛЬНОСТІ ШЛЯХУ (З суфіксом (N)) ---
// -----------------------------------------------------------

/**
 * Перевіряє існування файлу та додає суфікс (N) доки шлях не стане унікальним.
 */
char *get_unique_output_path(const char *base_path) {
    char *final_path = (char *)malloc(MAX_PATH_LEN);
    if (!final_path) {
        perror("malloc error");
        return NULL;
    }
    
    strncpy(final_path, base_path, MAX_PATH_LEN);
    final_path[MAX_PATH_LEN - 1] = '\0';

    // F_OK перевіряє, чи існує файл/директорія
    if (access(final_path, F_OK) != 0) {
        // Файл/Директорія не існує. Шлях унікальний.
        return final_path;
    }

    // --- Діагностичний вивід ---
    fprintf(stderr, "Попередження: Об'єкт '%s' вже існує. Шукаю унікальне ім'я...\n", base_path);
    // ---------------------------

    int counter = 1;
    char temp_path[MAX_PATH_LEN];
    size_t base_len = strlen(base_path);

    while (counter < 1000) { 
        if (base_len + 10 >= MAX_PATH_LEN) {
             fprintf(stderr, "Помилка: Шлях занадто довгий для додавання суфікса.\n");
             free(final_path);
             return NULL;
        }

        snprintf(temp_path, MAX_PATH_LEN, "%s(%d)", base_path, counter);

        if (access(temp_path, F_OK) != 0) {
            strncpy(final_path, temp_path, MAX_PATH_LEN);
            final_path[MAX_PATH_LEN - 1] = '\0';
            return final_path;
        }

        counter++;
    }

    fprintf(stderr, "Помилка: Не вдалося знайти унікальне ім'я файлу для розпакування.\n");
    free(final_path);
    return NULL;
}

// -----------------------------------------------------------
// --- ОСНОВНА ФУНКЦІЯ (main) ---
// -----------------------------------------------------------

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Помилка: не вказано аргументів.\n");
        print_help(argv[0]);
        return 1;
    }

    const char *option = argv[1];
    int result = 0;
    
    const char *password = NULL;
    
    // Помощь
    if (strcmp(option, "--help") == 0 || strcmp(option, "-h") == 0) {
        print_help(argv[0]);
        return 0;
    }

    // ----------------------
    // --- Стиснення (-c) ---
    // ----------------------
    if (strcmp(option, "--compress") == 0 || strcmp(option, "-c") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Помилка: --compress потребує ВХІДНИЙ_ФАЙЛ.\n");
            return 1;
        }
        const char *input_file = argv[2];
        
        // 1. Пошук опції -s у решті аргументів
        bool secure_mode = parse_secure_args(argc, argv, &password);
        
        if (secure_mode && password == NULL) return 1;

        // Генеруємо вихідний шлях (архів)
        char *archive_path = generate_path_name(input_file, true); 
        if (!archive_path) return 1;

        printf("Стиснення '%s' -> '%s'...\n", input_file, archive_path);
        
        // ЕТАП 1: Стиснення
        result = compress_file(input_file, archive_path); 
        
        if (result != 0) {
            fprintf(stderr, "✗ Помилка при стисненні файлу.\n");
            free(archive_path);
            return 1;
        }

        printf("✓ Файл успішно стиснено.\n");

        // ЕТАП 2: Шифрування
        if (secure_mode) {
            char *encrypted_path = (char *)malloc(MAX_PATH_LEN);
            if (!encrypted_path) {
                perror("malloc error");
                free(archive_path);
                return 1;
            }
            snprintf(encrypted_path, MAX_PATH_LEN, "%s.enc", archive_path);
            
            printf("Шифрування '%s' (пароль: %s) -> (Тимчасовий файл)...\n", archive_path, password);
            
            result = encrypt_file_openssl(archive_path, encrypted_path, password);
            
            if (result != 0) {
                fprintf(stderr, "✗ Помилка при шифруванні архіву.\n");
                free(encrypted_path);
                free(archive_path);
                return 1;
            }
            
            // Видаляємо незашифрований архів
            if (unlink(archive_path) != 0) {
                perror("Попередження: Не вдалося видалити незашифрований архів");
            }
            
            // Перейменовуємо зашифрований файл
            if (rename(encrypted_path, archive_path) != 0) {
                perror("Помилка: Не вдалося перейменувати зашифрований файл");
                result = 1;
            } else {
                printf("✓ Архів успішно зашифровано та збережено як '%s'.\n", archive_path);
            }
            
            free(encrypted_path);
        }
        
        free(archive_path);
        return result;
    }

    // ----------------------
    // --- Розпакування (-e) ---
    // ----------------------
    if (strcmp(option, "--extract") == 0 || strcmp(option, "-e") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Помилка: --extract потребує ВХІДНИЙ_ФАЙЛ.\n");
            return 1;
        }
        const char *input_file = argv[2];
        
        // Перевірка, чи вхідний файл - це архів (.k88)
        size_t len = strlen(input_file);
        if (len < 4 || strcmp(input_file + len - 4, ".k88") != 0) {
            fprintf(stderr, "Помилка: Вхідний файл для розпакування ('%s') повинен мати розширення '.k88'.\n", input_file);
            return 1;
        }

        // 1. Пошук опції -s у решті аргументів
        bool secure_mode = parse_secure_args(argc, argv, &password);
        
        if (secure_mode && password == NULL) return 1;

        char *temp_archive_path = NULL;
        const char *input_archive_path = input_file;
        int cleanup_needed = 0;

        // ЕТАП 1: Дешифрування (якщо secure_mode = true)
        if (secure_mode) {
            temp_archive_path = (char *)malloc(MAX_PATH_LEN);
            if (!temp_archive_path) {
                perror("malloc error");
                return 1;
            }
            snprintf(temp_archive_path, MAX_PATH_LEN, "/tmp/k88arc_dec_%d.tmp", (int)getpid());
            cleanup_needed = 1;
            
            printf("Дешифрування '%s' (пароль: %s) -> (Тимчасовий файл)...\n", input_file, password);
            
            result = decrypt_file_openssl(input_file, temp_archive_path, password);
            
            if (result != 0) {
                fprintf(stderr, "✗ Помилка при дешифруванні архіву (невірний пароль?).\n");
                goto cleanup_extract;
            }
            printf("✓ Архів успішно розшифровано.\n");
            input_archive_path = temp_archive_path;
        }

        // 2. Генеруємо БАЗОВИЙ вихідний шлях (видаляємо .k88)
        // !!! ВИПРАВЛЕНО: Використовуємо argv[2] (оригінальний шлях) для генерації імені, 
        // !!! незалежно від того, чи використовували ми тимчасовий файл для дешифрування.
        char *base_output_file = generate_path_name(argv[2], false); 
        if (!base_output_file) {
             result = 1;
             goto cleanup_extract;
        }
        
        // 3. Перевіряємо унікальність та отримуємо ФІНАЛЬНИЙ шлях (з (N) якщо потрібно)
        char *output_file = get_unique_output_path(base_output_file);
        
        free(base_output_file); 
        if (!output_file) {
             result = 1;
             goto cleanup_extract;
        }
        
        // ЕТАП 2: Розпакування
        printf("Розпакування '%s' -> '%s'...\n", input_archive_path, output_file);
        
        result = extract_file(input_archive_path, output_file); 
        
        if (result == 0) {
            printf("✓ Архів успішно розпакований.\n");
        } else {
            fprintf(stderr, "✗ Помилка при розпакуванні архіву.\n");
            result = 1;
        }
        
        free(output_file);

    cleanup_extract:
        // Очищення: видалення тимчасового розшифрованого файлу
        if (cleanup_needed) {
             if (unlink(temp_archive_path) != 0) {
                 perror("Попередження: Не вдалося видалити тимчасовий розшифрований файл");
             }
             free(temp_archive_path);
        }
        
        return result;
    }

    // Невідома опція
    fprintf(stderr, "Помилка: невідома опція '%s'\n", option);
    print_help(argv[0]);
    return 1;
}