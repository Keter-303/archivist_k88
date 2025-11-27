#include "../include/k88_api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_PATH_LEN 4096

static bool parse_secure_args(int argc, char *argv[], const char **password_out) {
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--secure") == 0) {
            if (i + 1 < argc) {
                *password_out = argv[i + 1];
                return true;
            } else {
                fprintf(stderr, "Error: option -s requires a PASSWORD.\n");
                return false;
            }
        }
    }
    return false;
}

void print_help(const char *program_name) {
    printf("Usage: %s [OPTION] [INPUT_FILE] [-s PASSWORD]\n\n", program_name);
    printf("Options:\n");
    printf("  --compress, -c       Compress file/directory (Output: INPUT_FILE.k88)\n");
    printf("  --extract, -e        Extract archive (INPUT_FILE must be *.k88)\n");
    printf("  -s, --secure         Use for encryption/decryption. Specify after INPUT_FILE.\n");
    printf("  --help, -h           Show this help\n\n");
    printf("Examples:\n");
    printf("  %s -c my_files -s secretkey\n", program_name);
    printf("  %s -e my_files.k88 -s secretkey\n", program_name);
}

char *generate_path_name(const char *input_path, bool is_compress) {
    char *new_path = (char *)malloc(MAX_PATH_LEN);
    if (new_path == NULL) {
        perror("Memory allocation error");
        return NULL;
    }

    if (is_compress) {
        snprintf(new_path, MAX_PATH_LEN, "%s.k88", input_path);
    } else {
        size_t len = strlen(input_path);
        const char *ext = ".k88";
        size_t ext_len = strlen(ext);

        if (len >= ext_len && strcmp(input_path + len - ext_len, ext) == 0) {
            snprintf(new_path, len - ext_len + 1, "%s", input_path);
        } else {
            snprintf(new_path, MAX_PATH_LEN, "%s_extracted", input_path);
        }
    }
    return new_path;
}

char *get_unique_output_path(const char *base_path) {
    char *final_path = (char *)malloc(MAX_PATH_LEN);
    if (!final_path) {
        perror("malloc error");
        return NULL;
    }

    strncpy(final_path, base_path, MAX_PATH_LEN);
    final_path[MAX_PATH_LEN - 1] = '\0';

    if (access(final_path, F_OK) != 0) {
        return final_path;
    }

    fprintf(stderr, "Warning: object '%s' already exists. Searching for a unique name...\n", base_path);

    int counter = 1;
    char temp_path[MAX_PATH_LEN];
    size_t base_len = strlen(base_path);

    while (counter < 1000) {
        if (base_len + 10 >= MAX_PATH_LEN) {
            fprintf(stderr, "Error: path too long to add suffix.\n");
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

    fprintf(stderr, "Error: could not find a unique filename for extraction.\n");
    free(final_path);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Error: no arguments provided.\n");
        print_help(argv[0]);
        return 1;
    }

    const char *option = argv[1];
    int result = 0;

    const char *password = NULL;

    if (strcmp(option, "--help") == 0 || strcmp(option, "-h") == 0) {
        print_help(argv[0]);
        return 0;
    }

    if (strcmp(option, "--compress") == 0 || strcmp(option, "-c") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: --compress requires INPUT_FILE.\n");
            return 1;
        }
        const char *input_file = argv[2];

        bool secure_mode = parse_secure_args(argc, argv, &password);

        if (secure_mode && password == NULL) return 1;

        char *archive_path = generate_path_name(input_file, true);
        if (!archive_path) return 1;

        printf("Compressing '%s' -> '%s'...\n", input_file, archive_path);

        result = compress_file(input_file, archive_path);

        if (result != 0) {
            fprintf(stderr, "Error: failed to compress file.\n");
            free(archive_path);
            return 1;
        }

        printf("File compressed successfully.\n");

        if (secure_mode) {
            char *encrypted_path = (char *)malloc(MAX_PATH_LEN);
            if (!encrypted_path) {
                perror("malloc error");
                free(archive_path);
                return 1;
            }
            snprintf(encrypted_path, MAX_PATH_LEN, "%s.enc", archive_path);

            printf("Encrypting '%s' with password '%s' -> temporary file...\n", archive_path, password);

            result = encrypt_file_openssl(archive_path, encrypted_path, password);

            if (result != 0) {
                fprintf(stderr, "Error: failed to encrypt archive.\n");
                free(encrypted_path);
                free(archive_path);
                return 1;
            }

            if (unlink(archive_path) != 0) {
                perror("Warning: failed to remove unencrypted archive");
            }

            if (rename(encrypted_path, archive_path) != 0) {
                perror("Error: failed to rename encrypted file");
                result = 1;
            } else {
                printf("Archive encrypted and saved as '%s'.\n", archive_path);
            }

            free(encrypted_path);
        }

        free(archive_path);
        return result;
    }

    if (strcmp(option, "--extract") == 0 || strcmp(option, "-e") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: --extract requires INPUT_FILE.\n");
            return 1;
        }
        const char *input_file = argv[2];

        size_t len = strlen(input_file);
        if (len < 4 || strcmp(input_file + len - 4, ".k88") != 0) {
            fprintf(stderr, "Error: input file for extraction ('%s') must have extension '.k88'.\n", input_file);
            return 1;
        }

        bool secure_mode = parse_secure_args(argc, argv, &password);
        if (secure_mode && password == NULL) return 1;

        char *temp_archive_path = NULL;
        const char *input_archive_path = input_file;
        int cleanup_needed = 0;

        if (secure_mode) {
            temp_archive_path = (char *)malloc(MAX_PATH_LEN);
            if (!temp_archive_path) {
                perror("malloc error");
                return 1;
            }
            snprintf(temp_archive_path, MAX_PATH_LEN, "/tmp/k88arc_dec_%d.tmp", (int)getpid());
            cleanup_needed = 1;

            printf("Decrypting '%s' with password '%s' -> temporary file...\n", input_file, password);

            result = decrypt_file_openssl(input_file, temp_archive_path, password);

            if (result != 0) {
                fprintf(stderr, "Error: failed to decrypt archive.\n");
                goto cleanup_extract;
            }
            printf("Archive decrypted successfully.\n");
            input_archive_path = temp_archive_path;
        }

        char *base_output_file = generate_path_name(argv[2], false);
        if (!base_output_file) {
             result = 1;
             goto cleanup_extract;
        }

    char *output_file = get_unique_output_path(base_output_file);

    free(base_output_file);
        if (!output_file) {
             result = 1;
             goto cleanup_extract;
        }

        printf("Extracting '%s' -> '%s'...\n", input_archive_path, output_file);

        result = extract_file(input_archive_path, output_file);

        if (result == 0) {
            printf("Archive extracted successfully.\n");
        } else {
            fprintf(stderr, "Error: failed to extract archive.\n");
            result = 1;
        }

        free(output_file);

    cleanup_extract:
        if (cleanup_needed) {
             if (unlink(temp_archive_path) != 0) {
                 perror("Warning: failed to remove temporary decrypted file");
             }
             free(temp_archive_path);
        }

        return result;
    }

    fprintf(stderr, "Error: unknown option '%s'\n", option);
    print_help(argv[0]);
    return 1;
}