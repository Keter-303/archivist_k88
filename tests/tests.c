#define _XOPEN_SOURCE 700 // <--- Виправлення: для коректного оголошення lstat та POSIX функцій

#include "../include/k88_api.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>

#define TEMP_DIR_PREFIX "tmp_test_"
#define ARCHIVE_EXT ".k88"
#define MAX_PATH_LEN 4096 
#define PATH_CONTENT "Long path test OK\n"

// --- ДОПОМІЖНІ ФУНКЦІЇ ---

static int write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    if (fwrite(content, 1, strlen(content), f) != strlen(content)) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

static int file_equals(const char *a, const char *b) {
    FILE *fa = fopen(a, "rb");
    FILE *fb = fopen(b, "rb");
    if (!fa || !fb) { if (fa) fclose(fa); if (fb) fclose(fb); return 0; }
    
    int ra = 0, rb = 0;
    while (1) {
        ra = fgetc(fa);
        rb = fgetc(fb);
        if (ra != rb) { fclose(fa); fclose(fb); return 0; }
        if (ra == EOF) break;
    }
    
    fclose(fa); fclose(fb); return 1;
}

static bool ensure_dir(const char *path) {
    char tmp[MAX_PATH_LEN]; // Використовуємо MAX_PATH_LEN для буфера
    strncpy(tmp, path, sizeof(tmp));
    tmp[sizeof(tmp)-1] = '\0';
    size_t len = strlen(tmp);
    if (len == 0) return false;
    if (tmp[len-1] == '/') tmp[len-1] = '\0';
    for (char *p = tmp + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return false;
            *p = '/';
        }
    }
    if (mkdir(tmp, 0755) != 0) {
        struct stat st;
        if (stat(tmp, &st) == 0 && S_ISDIR(st.st_mode)) return true;
        return false;
    }
    return true;
}

/**
 * Рекурсивно видаляє директорію та її вміст (Використовує lstat для надійності).
 */
static int remove_directory_recursively(const char *path) {
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = 0;

    if (!d) return 0;

    struct dirent *p;
    while (!r && (p = readdir(d))) {
        int r2 = -1;
        char *buf;
        size_t len;

        if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) continue;

        len = path_len + strlen(p->d_name) + 2; 
        buf = (char*)malloc(len);

        if (buf) {
            snprintf(buf, len, "%s/%s", path, p->d_name);
            
            struct stat statbuf;
            if (lstat(buf, &statbuf) == 0) {
                if (S_ISDIR(statbuf.st_mode)) { 
                    r2 = remove_directory_recursively(buf);
                } else {
                    r2 = unlink(buf);
                }
            } else {
                r2 = -1; // Помилка lstat
            }
            
            free(buf);
        }
        r = r2;
    }

    closedir(d);

    if (!r) r = rmdir(path);

    return r;
}

static void cleanup_all(void) {
    /* Видаляємо всі тимчасові файли та директорії */
    remove_directory_recursively(TEMP_DIR_PREFIX "single");
    remove_directory_recursively(TEMP_DIR_PREFIX "single_out");
    unlink(TEMP_DIR_PREFIX "single" ARCHIVE_EXT);
    
    remove_directory_recursively(TEMP_DIR_PREFIX "nested");
    remove_directory_recursively(TEMP_DIR_PREFIX "nested_out");
    unlink(TEMP_DIR_PREFIX "nested" ARCHIVE_EXT);
    
    remove_directory_recursively(TEMP_DIR_PREFIX "empty");
    remove_directory_recursively(TEMP_DIR_PREFIX "empty_out");
    unlink(TEMP_DIR_PREFIX "empty" ARCHIVE_EXT);
    
    remove_directory_recursively(TEMP_DIR_PREFIX "uni");
    remove_directory_recursively(TEMP_DIR_PREFIX "uni_out");
    unlink(TEMP_DIR_PREFIX "uni" ARCHIVE_EXT);
    
    remove_directory_recursively(TEMP_DIR_PREFIX "encrypt");
    remove_directory_recursively(TEMP_DIR_PREFIX "encrypt_out");
    unlink(TEMP_DIR_PREFIX "encrypt" ARCHIVE_EXT);
    
    // Очищення для НОВИХ ТЕСТІВ
    remove_directory_recursively(TEMP_DIR_PREFIX "longpath");
    remove_directory_recursively(TEMP_DIR_PREFIX "longpath_out");
    unlink(TEMP_DIR_PREFIX "longpath" ARCHIVE_EXT);

    remove_directory_recursively(TEMP_DIR_PREFIX "corrupted");
    remove_directory_recursively(TEMP_DIR_PREFIX "corrupted_out");
    unlink(TEMP_DIR_PREFIX "corrupted" ARCHIVE_EXT);
}

// --- ТЕСТИ СТИСНЕННЯ/РОЗПАКУВАННЯ ---

static int test_single_file(void) {
    const char *in_dir = TEMP_DIR_PREFIX "single";
    const char *out_dir = TEMP_DIR_PREFIX "single_out";
    const char *archive = TEMP_DIR_PREFIX "single" ARCHIVE_EXT;
    printf("Test: 1. single file...\n");
    
    ensure_dir(in_dir);
    write_file("tmp_test_single/a.txt", "hello world\n");
    
    if (compress_file("tmp_test_single/a.txt", archive) != 0) return 1;
    if (extract_file(archive, out_dir) != 0) return 1;
    
    if (!file_equals("tmp_test_single/a.txt", "tmp_test_single_out/a.txt")) return 1;
    printf("  ok\n");
    return 0;
}

static int test_nested_dirs(void) {
    const char *in_dir = TEMP_DIR_PREFIX "nested";
    const char *out_dir = TEMP_DIR_PREFIX "nested_out";
    const char *archive = TEMP_DIR_PREFIX "nested" ARCHIVE_EXT;
    printf("Test: 2. nested directories...\n");
    
    ensure_dir("tmp_test_nested/dirA/dirB");
    write_file("tmp_test_nested/a.txt", "Data A\n");
    write_file("tmp_test_nested/dirA/b.txt", "Data B\n");
    write_file("tmp_test_nested/dirA/dirB/c.txt", "Data C\n");
    
    if (compress_file(in_dir, archive) != 0) return 1;
    if (extract_file(archive, out_dir) != 0) return 1;
    
    if (!file_equals("tmp_test_nested/a.txt", "tmp_test_nested_out/a.txt")) return 1;
    if (!file_equals("tmp_test_nested/dirA/b.txt", "tmp_test_nested_out/dirA/b.txt")) return 1;
    if (!file_equals("tmp_test_nested/dirA/dirB/c.txt", "tmp_test_nested_out/dirA/dirB/c.txt")) return 1;
    printf("  ok\n");
    return 0;
}

static int test_empty_dir(void) {
    const char *in_dir = TEMP_DIR_PREFIX "empty";
    const char *out_dir = TEMP_DIR_PREFIX "empty_out";
    const char *archive = TEMP_DIR_PREFIX "empty" ARCHIVE_EXT;
    printf("Test: 3. empty directory...\n");
    
    ensure_dir("tmp_test_empty/emptydir");
    if (compress_file(in_dir, archive) != 0) return 1;
    if (extract_file(archive, out_dir) != 0) return 1;
    
    struct stat st;
    if (stat("tmp_test_empty_out/emptydir", &st) != 0) return 1;
    if (!S_ISDIR(st.st_mode)) return 1;
    printf("  ok\n");
    return 0;
}

static int test_unicode_names(void) {
    const char *in_dir = TEMP_DIR_PREFIX "uni";
    const char *out_dir = TEMP_DIR_PREFIX "uni_out";
    const char *archive = TEMP_DIR_PREFIX "uni" ARCHIVE_EXT;
    printf("Test: 4. unicode filenames...\n");
    
    ensure_dir(in_dir);
    write_file("tmp_test_uni/файл_юнікод.txt", "unic\n");
    
    if (compress_file(in_dir, archive) != 0) return 1;
    if (extract_file(archive, out_dir) != 0) return 1;
    
    if (!file_equals("tmp_test_uni/файл_юнікод.txt", "tmp_test_uni_out/файл_юнікод.txt")) return 1;
    printf("  ok\n");
    return 0;
}

// --- ТЕСТ ШИФРУВАННЯ/ДЕШИФРУВАННЯ ---

static int test_encryption(void) {
    const char *in_dir = TEMP_DIR_PREFIX "encrypt";
    const char *out_dir = TEMP_DIR_PREFIX "encrypt_out";
    const char *archive = TEMP_DIR_PREFIX "encrypt" ARCHIVE_EXT;
    const char *password = "SuperSecret123";
    char temp_enc_path[MAX_PATH_LEN];

    printf("Test: 5. encryption/decryption...\n");

    // 1. Створення вхідних даних
    ensure_dir(in_dir);
    write_file("tmp_test_encrypt/secret.txt", "This is a secret message.\n");
    
    // 2. Стиснення
    if (compress_file(in_dir, archive) != 0) return 1;

    // 3. Шифрування
    snprintf(temp_enc_path, MAX_PATH_LEN, "%s.enc", archive);
    if (encrypt_file_openssl(archive, temp_enc_path, password) != 0) return 1;
    
    // Заміна незашифрованого архіву на зашифрований
    if (unlink(archive) != 0) return 1;
    if (rename(temp_enc_path, archive) != 0) return 1;

    // 4. Дешифрування та Розпакування (правильний пароль)
    char temp_dec_path[MAX_PATH_LEN];
    snprintf(temp_dec_path, MAX_PATH_LEN, "%s.dec", archive);
    
    // Дешифрування
    if (decrypt_file_openssl(archive, temp_dec_path, password) != 0) return 1;
    
    // Розпакування
    if (extract_file(temp_dec_path, out_dir) != 0) {
        unlink(temp_dec_path);
        return 1;
    }
    unlink(temp_dec_path);
    
    // 5. Перевірка вмісту
    if (!file_equals("tmp_test_encrypt/secret.txt", "tmp_test_encrypt_out/secret.txt")) return 1;
    
    // 6. Тест неправильного пароля (повинен повернути помилку)
    remove_directory_recursively(out_dir); 

    if (decrypt_file_openssl(archive, temp_dec_path, "WrongPassword") == 0) {
         unlink(temp_dec_path);
         return 1;
    }
    
    printf("  ok\n");
    return 0;
}

// --- НОВИЙ ТЕСТ: Довгі шляхи ---

static int test_long_path(void) {
    const char *in_dir = TEMP_DIR_PREFIX "longpath";
    const char *out_dir = TEMP_DIR_PREFIX "longpath_out";
    const char *archive = TEMP_DIR_PREFIX "longpath" ARCHIVE_EXT;
    printf("Test: 6. long path name...\n");

    // 1. Створення дуже довгого шляху (близько 1000 символів)
    char long_subdir[MAX_PATH_LEN];
    memset(long_subdir, 'A', 1000);
    long_subdir[1000] = '\0'; // Обмежуємо для безпеки

    char long_path[MAX_PATH_LEN];
    snprintf(long_path, MAX_PATH_LEN, "%s/%s/test.txt", in_dir, long_subdir);
    
    // Перевіряємо, що ми не вийшли за межі буфера перед створенням директорії
    if (strlen(long_path) >= MAX_PATH_LEN - 1) {
        fprintf(stderr, "  (Пропущено: Згенерований шлях занадто довгий)\n");
        return 0; 
    }

    if (!ensure_dir(long_path)) return 1;
    
    if (write_file(long_path, PATH_CONTENT) != 0) return 1;

    // 2. Стиснення та розпакування
    if (compress_file(in_dir, archive) != 0) return 1;
    if (extract_file(archive, out_dir) != 0) return 1;

    // 3. Перевірка відновленого шляху
    char out_long_path[MAX_PATH_LEN];
    snprintf(out_long_path, MAX_PATH_LEN, "%s/%s/test.txt", out_dir, long_subdir);

    if (!file_equals(long_path, out_long_path)) return 1;

    printf("  ok\n");
    return 0;
}

// --- НОВИЙ ТЕСТ: Пошкоджений архів ---

static int test_corrupted_archive(void) {
    const char *in_dir = TEMP_DIR_PREFIX "corrupted";
    const char *out_dir = TEMP_DIR_PREFIX "corrupted_out";
    const char *archive = TEMP_DIR_PREFIX "corrupted" ARCHIVE_EXT;
    printf("Test: 7. corrupted archive handling...\n");

    // 1. Створення нормального архіву
    ensure_dir(in_dir);
    write_file("tmp_test_corrupted/data.txt", "Original data.\n");
    if (compress_file(in_dir, archive) != 0) return 1;

    // 2. Пошкодження архіву (зміна одного байта)
    FILE *f = fopen(archive, "r+b");
    if (!f) return 1;
    
    // Переходимо на 100-й байт (якщо файл існує і достатньо великий)
    if (fseek(f, 100, SEEK_SET) == 0) {
        char corrupt_byte = 0xFF;
        fwrite(&corrupt_byte, 1, 1, f);
    }
    fclose(f);

    // 3. Спроба розпакування пошкодженого архіву
    // Очікуємо, що extract_file поверне помилку (ненульовий код)
    int result = extract_file(archive, out_dir);
    
    // Очищаємо, оскільки розпакування могло створити часткову директорію
    remove_directory_recursively(out_dir);

    if (result == 0) {
        // Помилка: Розпакування пошкодженого архіву успішне
        printf("  fail (Corrupted archive was extracted successfully)\n");
        return 1;
    }

    printf("  ok (Extraction failed as expected)\n");
    return 0;
}

// --- ГОЛОВНА ФУНКЦІЯ ---

int main(void) {
    int rc = 0;
    
    // Очищення перед початком
    cleanup_all(); 
    
    printf("Запуск комплексних тестів...\n");
    
    // Існуючі базові тести
    rc += test_single_file();
    rc += test_nested_dirs();
    rc += test_empty_dir();
    rc += test_unicode_names();
    rc += test_encryption();

    // НОВІ ТЕСТИ НА НАДІЙНІСТЬ
    rc += test_long_path();         
    rc += test_corrupted_archive();  

    if (rc == 0) printf("\n✓ All tests passed.\n");
    else printf("\n✗ Some tests failed (rc=%d).\n", rc);

    /* завжди очищаємо тимчасові файли */
    cleanup_all();

    return rc == 0 ? 0 : 1;
}