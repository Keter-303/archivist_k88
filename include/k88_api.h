
#ifndef K88_API_H
#define K88_API_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> 
#include <stdbool.h> 



// Функції для стиснення та розпакування файлів (--compress та --extract)
int compress_file(const char *input_path, const char *output_path, bool encrypt);
int extract_file(const char *input_path, const char *output_path, bool decrypt);

// Функції для стиснення та розпакування директорій (--compress-dir та --extract-dir)
int compress_directory(const char *input_path, const char *output_path, bool encrypt);
int extract_directory(const char *input_path, const char *output_path, bool decrypt);


// Функції шифрування/дешифрування OpenSSL
int encrypt_file_openssl(const char *in_path, const char *out_path, const char *key_phrase);
int decrypt_file_openssl(const char *in_path, const char *out_path, const char *key_phrase);

// Функції для k88_main.c (-h та інші)
void print_help(const char *program_name);

#endif // K88_API_H