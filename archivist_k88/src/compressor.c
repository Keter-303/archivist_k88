#include "../include/k88_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h> 

// Визначаємо розмір буфера для читання/запису (16 KB)
#define CHUNK 16384 

int compress_file(const char *input_path, const char *output_path) {
    FILE *in_fp = NULL;
    FILE *out_fp = NULL;
    int result = Z_OK;
    int flush;
    
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
    
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    
    if (deflateInit(&strm, Z_DEFAULT_COMPRESSION) != Z_OK) {
        fprintf(stderr, "Помилка ініціалізації zlib.\n");
        return -1;
    }

    in_fp = fopen(input_path, "rb");
    if (in_fp == NULL) { perror("Помилка відкриття вхідного файлу"); result = -1; goto cleanup; }

    out_fp = fopen(output_path, "wb");
    if (out_fp == NULL) { perror("Помилка відкриття вихідного файлу"); result = -1; goto cleanup; }

    do {
        strm.avail_in = fread(in, 1, CHUNK, in_fp);
        if (ferror(in_fp)) {
            result = -1; break;
        }
        
        flush = feof(in_fp) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in; 

        do {
            strm.avail_out = CHUNK; 
            strm.next_out = out;

            result = deflate(&strm, flush);
            
            if (result == Z_STREAM_ERROR) {
                result = -1; goto cleanup;
            }

            size_t have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, out_fp) != have || ferror(out_fp)) {
                result = -1; goto cleanup;
            }

        } while (strm.avail_out == 0); 

    } while (flush != Z_FINISH);

    if (result != Z_STREAM_END) {
        result = -1; 
    }
    
    deflateEnd(&strm);

cleanup:
    if (in_fp) fclose(in_fp);
    if (out_fp) fclose(out_fp);

    if (result != Z_OK && result != Z_STREAM_END && result != 0) {
        fprintf(stderr, "Помилка стиснення ZLIB: %d\n", result);
        return -1;
    }
    return 0;
}