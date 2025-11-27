
#ifndef K88_API_H
#define K88_API_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> 

#define MY_PATH_MAX 4099

int compress_file(const char *input_path, const char *output_path);
int extract_file(const char *input_path, const char *output_path);


int compress_directory(const char *input_path, const char *output_path);
int extract_directory(const char *input_path, const char *output_path);


int encrypt_data(uint8_t *data, size_t len, const char *password, uint8_t **out_data, size_t *out_len);
int decrypt_data(uint8_t *data, size_t len, const char *password, uint8_t **out_data, size_t *out_len);


int encrypt_file_openssl(const char *in_path, const char *out_path, const char *key_phrase);
int decrypt_file_openssl(const char *in_path, const char *out_path, const char *key_phrase);
void print_help(const char *program_name);

#endif