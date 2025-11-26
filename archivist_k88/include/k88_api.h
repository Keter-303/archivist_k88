
#ifndef K88_API_H
#define K88_API_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> 


// Функції для стиснення та розпакування файлів (--compress та --extract)
int compress_file(const char *input_path, const char *output_path);
int extract_file(const char *input_path, const char *output_path);

// Функції для стиснення та розпакування директорій (--compress-dir та --extract-dir)
int compress_directory(const char *input_path, const char *output_path);
int extract_directory(const char *input_path, const char *output_path);

// Функції для криптографії (--encrypt та --decrypt)
int encrypt_data(uint8_t *data, size_t len, const char *password, uint8_t **out_data, size_t *out_len);
int decrypt_data(uint8_t *data, size_t len, const char *password, uint8_t **out_data, size_t *out_len);

// Функції для k88_main.c (-h та інші)
void print_help(const char *program_name);

#endif // K88_API_H