#include "../include/k88_api.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    fwrite(content, 1, strlen(content), f);
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
    // create directories recursively (mkdir -p)
    char tmp[1024];
    strncpy(tmp, path, sizeof(tmp));
    tmp[sizeof(tmp)-1] = '\0';
    size_t len = strlen(tmp);
    if (len == 0) return false;
    if (tmp[len-1] == '/') tmp[len-1] = '\0';
    for (char *p = tmp + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
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

static void cleanup_all(void) {
    /* remove all temp dirs and archives created by tests */
    system("rm -rf tmp_single tmp_single_out tmp_single.k88 tmp_nested tmp_nested_out tmp_nested.k88 tmp_empty tmp_empty_out tmp_empty.k88 tmp_uni tmp_uni_out tmp_uni.k88 2>/dev/null");
}

static int test_single_file(void) {
    printf("Test: single file...\n");
    system("rm -rf tmp_single tmp_single_out tmp_single.k88 2>/dev/null");
    ensure_dir("tmp_single");
    write_file("tmp_single/a.txt", "hello\n");
    if (compress_file("tmp_single/a.txt", "tmp_single.k88") != 0) return 1;
    if (extract_file("tmp_single.k88", "tmp_single_out") != 0) return 1;
    if (!file_equals("tmp_single/a.txt", "tmp_single_out/a.txt")) return 1;
    printf("  ok\n");
    return 0;
}

static int test_nested_dirs(void) {
    printf("Test: nested directories...\n");
    system("rm -rf tmp_nested tmp_nested_out tmp_nested.k88 2>/dev/null");
    ensure_dir("tmp_nested/dirA/dirB");
    write_file("tmp_nested/a.txt", "A\n");
    write_file("tmp_nested/dirA/b.txt", "B\n");
    write_file("tmp_nested/dirA/dirB/c.txt", "C\n");
    if (compress_file("tmp_nested", "tmp_nested.k88") != 0) return 1;
    if (extract_file("tmp_nested.k88", "tmp_nested_out") != 0) return 1;
    if (!file_equals("tmp_nested/a.txt", "tmp_nested_out/a.txt")) return 1;
    if (!file_equals("tmp_nested/dirA/b.txt", "tmp_nested_out/dirA/b.txt")) return 1;
    if (!file_equals("tmp_nested/dirA/dirB/c.txt", "tmp_nested_out/dirA/dirB/c.txt")) return 1;
    printf("  ok\n");
    return 0;
}

static int test_empty_dir(void) {
    printf("Test: empty directory...\n");
    system("rm -rf tmp_empty tmp_empty_out tmp_empty.k88 2>/dev/null");
    ensure_dir("tmp_empty/emptydir");
    if (compress_file("tmp_empty", "tmp_empty.k88") != 0) return 1;
    if (extract_file("tmp_empty.k88", "tmp_empty_out") != 0) return 1;
    struct stat st;
    if (stat("tmp_empty_out/emptydir", &st) != 0) return 1;
    if (!S_ISDIR(st.st_mode)) return 1;
    printf("  ok\n");
    return 0;
}

static int test_unicode_names(void) {
    printf("Test: unicode filenames...\n");
    system("rm -rf tmp_uni tmp_uni_out tmp_uni.k88 2>/dev/null");
    ensure_dir("tmp_uni");
    write_file("tmp_uni/файл.txt", "unic\n");
    if (compress_file("tmp_uni", "tmp_uni.k88") != 0) return 1;
    if (extract_file("tmp_uni.k88", "tmp_uni_out") != 0) return 1;
    if (!file_equals("tmp_uni/файл.txt", "tmp_uni_out/файл.txt")) return 1;
    printf("  ok\n");
    return 0;
}

int main(void) {
    int rc = 0;
    printf("Запуск тестів: стиснення та розпакування (smoke).\n");
    rc += test_single_file();
    rc += test_nested_dirs();
    rc += test_empty_dir();
    rc += test_unicode_names();
    if (rc == 0) printf("All tests passed.\n");
    else printf("Some tests failed (rc=%d).\n", rc);

    /* always attempt to clean up temps */
    cleanup_all();

    return rc == 0 ? 0 : 1;
}