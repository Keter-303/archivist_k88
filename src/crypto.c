#include "../include/k88_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>

#define SALT_SIZE 8     // Розмір Salt (8 байт)
#define KEY_SIZE 32     // AES-256 ключ
#define IV_SIZE 16      // AES-256-CBC Вектор ініціалізації
#define ITERATIONS 1    // Ітерації для EVP_BytesToKey


static void handleErrors(void) {
    ERR_print_errors_fp(stderr);
}

static int do_crypt_block(uint8_t *in_data, size_t in_len, const char *password, 
                          uint8_t **out_data, size_t *out_len, int encrypt_mode) {
    
    EVP_CIPHER_CTX *ctx = NULL;
    uint8_t key[KEY_SIZE];
    uint8_t iv[IV_SIZE];
    uint8_t salt[SALT_SIZE];
    size_t data_offset = 0; 
    
    int len, final_len;
    size_t alloc_size;
    int result = -1; 
    
    *out_data = NULL;
    *out_len = 0;


    if (encrypt_mode) {

        if (!RAND_bytes(salt, SALT_SIZE)) {
            fprintf(stderr, "Помилка генерації Salt.\n"); goto cleanup;
        }

        alloc_size = SALT_SIZE + in_len + EVP_MAX_BLOCK_LENGTH;
        data_offset = SALT_SIZE;
    } else {

        if (in_len < SALT_SIZE) {
            fprintf(stderr, "Дані занадто короткі, немає місця для Salt.\n"); goto cleanup;
        }
        memcpy(salt, in_data, SALT_SIZE);
        in_data += SALT_SIZE; 
        in_len -= SALT_SIZE;

        alloc_size = in_len + EVP_MAX_BLOCK_LENGTH; 
        data_offset = 0;
    }
    

    if (!EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha256(), salt, 
                       (unsigned char *)password, strlen(password), 
                       ITERATIONS, key, iv)) {
        fprintf(stderr, "Помилка EVP_BytesToKey (Невірний пароль?).\n"); goto cleanup;
    }

    *out_data = (uint8_t *)malloc(alloc_size);
    if (*out_data == NULL) {
        perror("Помилка malloc"); goto cleanup;
    }


    if (encrypt_mode) {
        memcpy(*out_data, salt, SALT_SIZE);
    }


    if (!(ctx = EVP_CIPHER_CTX_new())) { handleErrors(); goto cleanup; }

    if (EVP_CipherInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv, encrypt_mode) != 1) { 
        handleErrors(); goto cleanup; 
    }

    if (EVP_CipherUpdate(ctx, *out_data + data_offset, &len, in_data, in_len) != 1) { 
        handleErrors(); goto cleanup; 
    }
    *out_len = len;


    if (EVP_CipherFinal_ex(ctx, *out_data + data_offset + len, &final_len) != 1) { 
        fprintf(stderr, "Помилка фіналізації OpenSSL (Невірний пароль/дані?).\n");
        goto cleanup;
    }
    *out_len += final_len;


    if (encrypt_mode) {
        *out_len += SALT_SIZE;
    }
    
    result = 0; 

cleanup:
    if (ctx) EVP_CIPHER_CTX_free(ctx);
    if (result != 0 && *out_data != NULL) {
        free(*out_data);
        *out_data = NULL;
        *out_len = 0;
    }
    return result;
}


int encrypt_data(uint8_t *data, size_t len, const char *password, uint8_t **out_data, size_t *out_len) {
    return do_crypt_block(data, len, password, out_data, out_len, 1 /* ENCRYPT */);
}

int decrypt_data(uint8_t *data, size_t len, const char *password, uint8_t **out_data, size_t *out_len) {
    return do_crypt_block(data, len, password, out_data, out_len, 0 /* DECRYPT */);
}

static uint8_t *read_file_to_buffer(const char *path, size_t *len) {
    FILE *fp = fopen(path, "rb");
    if (!fp) { perror("Помилка відкриття файлу для читання"); return NULL; }
    fseek(fp, 0, SEEK_END);
    *len = ftell(fp);
    rewind(fp);
    uint8_t *buffer = (uint8_t *)malloc(*len);
    if (!buffer) { fclose(fp); perror("Помилка malloc"); return NULL; }
    if (fread(buffer, 1, *len, fp) != *len) {
        free(buffer); fclose(fp); perror("Помилка читання файлу"); return NULL;
    }
    fclose(fp);
    return buffer;
}

static int write_buffer_to_file(const char *path, uint8_t *data, size_t len) {
    FILE *fp = fopen(path, "wb");
    if (!fp) { perror("Помилка відкриття файлу для запису"); return -1; }
    if (fwrite(data, 1, len, fp) != len) {
        fclose(fp); perror("Помилка запису файлу"); return -1;
    }
    fclose(fp);
    return 0;
}

int encrypt_file_openssl(const char *in_path, const char *out_path, const char *key_phrase) {
    uint8_t *in_buffer = NULL, *out_buffer = NULL;
    size_t in_len = 0, out_len = 0;
    int result = -1;

    in_buffer = read_file_to_buffer(in_path, &in_len);
    if (!in_buffer) return -1;

    if (encrypt_data(in_buffer, in_len, key_phrase, &out_buffer, &out_len) != 0) {
        fprintf(stderr, "Помилка блокового шифрування.\n");
        goto cleanup;
    }

    if (write_buffer_to_file(out_path, out_buffer, out_len) != 0) {
        goto cleanup;
    }

    result = 0;

cleanup:
    if (in_buffer) free(in_buffer);
    if (out_buffer) free(out_buffer);
    return result;
}

int decrypt_file_openssl(const char *in_path, const char *out_path, const char *key_phrase) {
    uint8_t *in_buffer = NULL, *out_buffer = NULL;
    size_t in_len = 0, out_len = 0;
    int result = -1;

    in_buffer = read_file_to_buffer(in_path, &in_len);
    if (!in_buffer) return -1;

    if (decrypt_data(in_buffer, in_len, key_phrase, &out_buffer, &out_len) != 0) {
        
        fprintf(stderr, "Помилка блокового дешифрування (Неправильний пароль?).\n");
        goto cleanup;
    }

    if (write_buffer_to_file(out_path, out_buffer, out_len) != 0) {
        goto cleanup;
    }

    result = 0;

cleanup:
    if (in_buffer) free(in_buffer);
    if (out_buffer) free(out_buffer);
    return result;
}