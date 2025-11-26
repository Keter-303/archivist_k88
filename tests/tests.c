#include "../include/k88_api.h" 
#include <stdio.h>
#include <stdbool.h> 
#include <stdlib.h> 

#define INPUT_FILE "testdata.txt" 
#define ENCRYPTED_FILE "testdata_ch.txt" 
#define OUTPUT_ARCHIVE "testdata.k88"    
#define EXTRACTED_FILE "testdata_final.txt" 
#define TEMP_DECOMPRESSED "temp_decompressed.tmp" 
#define PASSWORD "secret_key_88"

int compare_files(const char *path1, const char *path2) {
    FILE *fp1 = fopen(path1, "rb");
    FILE *fp2 = fopen(path2, "rb");
    int ch1, ch2;

    if (!fp1 || !fp2) {
        if (fp1) fclose(fp1);
        if (fp2) fclose(fp2);
        return 1;
    }

    do {
        ch1 = fgetc(fp1);
        ch2 = fgetc(fp2);
        if (ch1 != ch2) {
            fclose(fp1);
            fclose(fp2);
            return 1;
        }
    } while (ch1 != EOF && ch2 != EOF);

    fclose(fp1);
    fclose(fp2);
    return 0;
}

int main() {
    int result = 0;
    
    if (encrypt_file_openssl(INPUT_FILE, ENCRYPTED_FILE, PASSWORD) != 0) {
        result = 1; goto cleanup;
    }

    if (compress_file(ENCRYPTED_FILE, OUTPUT_ARCHIVE, false) != 0) {
        result = 1; goto cleanup;
    }

    if (extract_file(OUTPUT_ARCHIVE, TEMP_DECOMPRESSED, false) != 0) {
        result = 1; goto cleanup;
    }
    
    if (decrypt_file_openssl(TEMP_DECOMPRESSED, EXTRACTED_FILE, PASSWORD) != 0) {
        result = 1; goto cleanup;
    }

    if (compare_files(INPUT_FILE, EXTRACTED_FILE) != 0) {
        result = 1; goto cleanup;
    }

cleanup:
    remove(ENCRYPTED_FILE);
    remove(OUTPUT_ARCHIVE);
    remove(TEMP_DECOMPRESSED);
    
    return result;
}