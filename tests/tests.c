#define _XOPEN_SOURCE 700 

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
#include <openssl/evp.h> 
#include <openssl/err.h>
#include <openssl/rand.h>

#define TEMP_DIR_PREFIX "tmp_test_"
#define ARCHIVE_EXT ".k88"
#define PATH_CONTENT "Long path test OK\n"

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
    char tmp[MY_PATH_MAX];
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
                r2 = -1; 
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
    
    remove_directory_recursively(TEMP_DIR_PREFIX "longpath");
    remove_directory_recursively(TEMP_DIR_PREFIX "longpath_out");
    unlink(TEMP_DIR_PREFIX "longpath" ARCHIVE_EXT);

    remove_directory_recursively(TEMP_DIR_PREFIX "corrupted");
    remove_directory_recursively(TEMP_DIR_PREFIX "corrupted_out");
    unlink(TEMP_DIR_PREFIX "corrupted" ARCHIVE_EXT);
}


static int test_single_file(void) {
    const char *in_dir = TEMP_DIR_PREFIX "single";
    const char *out_dir = TEMP_DIR_PREFIX "single_out";
    const char *archive = TEMP_DIR_PREFIX "single" ARCHIVE_EXT;
    printf("Test: 1. single file...\n");
    
    ensure_dir(in_dir);
    if (write_file("tmp_test_single/a.txt", "hello world\n") != 0) return 1;
    
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
    if (write_file("tmp_test_nested/a.txt", "Data A\n") != 0) return 1;
    if (write_file("tmp_test_nested/dirA/b.txt", "Data B\n") != 0) return 1;
    if (write_file("tmp_test_nested/dirA/dirB/c.txt", "Data C\n") != 0) return 1;
    
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
    if (write_file("tmp_test_uni/файл_юнікод.txt", "unic\n") != 0) return 1;
    
    if (compress_file(in_dir, archive) != 0) return 1;
    if (extract_file(archive, out_dir) != 0) return 1;
    
    if (!file_equals("tmp_test_uni/файл_юнікод.txt", "tmp_test_uni_out/файл_юнікод.txt")) return 1;
    printf("  ok\n");
    return 0;
}

static int test_corrupted_archive(void) {
    const char *in_dir = TEMP_DIR_PREFIX "corrupted";
    const char *out_dir = TEMP_DIR_PREFIX "corrupted_out";
    const char *archive = TEMP_DIR_PREFIX "corrupted" ARCHIVE_EXT;
    printf("Test: 5. corrupted archive handling...\n");

    ensure_dir(in_dir);
    if (write_file("tmp_test_corrupted/data.txt", "Original data.\n") != 0) return 1;
    if (compress_file(in_dir, archive) != 0) return 1;

    FILE *f = fopen(archive, "r+b");
    if (!f) return 1;
    
    if (fseek(f, 0, SEEK_SET) == 0) { 
        char corrupt_byte = 0x00; 
        fwrite(&corrupt_byte, 1, 1, f);
    }
    fclose(f);

    int result = extract_file(archive, out_dir);
    
    remove_directory_recursively(out_dir);

    if (result == 0) {
        printf("  fail (Corrupted archive was extracted successfully)\n");
        return 1;
    }

    printf("  ok (Extraction failed as expected)\n");
    return 0;
}


int main(void) {
    int rc = 0;
    
    cleanup_all(); 
    
    printf("Running full test suite...\n");
    
    rc += test_single_file();
    rc += test_nested_dirs();
    rc += test_empty_dir();
    rc += test_unicode_names();
    rc += test_corrupted_archive();  

    if (rc == 0) printf("\nAll tests passed.\n");
    else printf("\nSome tests failed (rc=%d).\n", rc);

    cleanup_all();

    return rc == 0 ? 0 : 1;
}