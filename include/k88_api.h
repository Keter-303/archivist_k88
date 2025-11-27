
#ifndef K88_API_H
#define K88_API_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> 
#include <stdbool.h> 



// (--compress and --extract)
int compress_file(const char *input_path, const char *output_path, bool encrypt);
int extract_file(const char *input_path, const char *output_path, bool decrypt);

// (--compress-dir and --extract-dir)
int compress_directory(const char *input_path, const char *output_path, bool encrypt);
int extract_directory(const char *input_path, const char *output_path, bool decrypt);


// OpenSSL
int encrypt_file_openssl(const char *in_path, const char *out_path, const char *key_phrase);
int decrypt_file_openssl(const char *in_path, const char *out_path, const char *key_phrase);

// (-h --help)
void print_help(const char *program_name);

#endif // K88_API_H