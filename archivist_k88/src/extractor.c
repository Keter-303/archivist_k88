#include "../include/k88_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h> 

#define CHUNK 16384 

int extract_file(const char *input_path, const char *output_path) {
    FILE *in_fp = NULL;
    FILE *out_fp = NULL;
    int result = Z_OK;
    
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
    
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    
    if (inflateInit(&strm) != Z_OK) {
        fprintf(stderr, "Помилка ініціалізації zlib (inflate).\n");
        return -1;
    }

    in_fp = fopen(input_path, "rb");
    if (in_fp == NULL) { perror("Помилка відкриття вхідного архіву"); result = -1; goto cleanup; }

    out_fp = fopen(output_path, "wb");
    if (out_fp == NULL) { perror("Помилка відкриття вихідного файлу"); result = -1; goto cleanup; }
    
    do {
        strm.avail_in = fread(in, 1, CHUNK, in_fp);
        if (ferror(in_fp)) {
            result = -1; break;
        }
        
        if (strm.avail_in == 0) {
            break; 
        }
        
        strm.next_in = in; 


        do {
            strm.avail_out = CHUNK; 
            strm.next_out = out;


            result = inflate(&strm, Z_NO_FLUSH);
            

            switch (result) {
                case Z_STREAM_ERROR:
                case Z_NEED_DICT:
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    inflateEnd(&strm);
                    result = -1; goto cleanup;
            }


            size_t have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, out_fp) != have || ferror(out_fp)) {
                result = -1; goto cleanup;
            }

        } while (strm.avail_out == 0); 

    } while (result != Z_STREAM_END); 

    if (result != Z_STREAM_END) {
        result = -1;
    } else {
        result = 0; 
    }

    inflateEnd(&strm);

cleanup:
    if (in_fp) fclose(in_fp);
    if (out_fp) fclose(out_fp);

    if (result != 0) {
        fprintf(stderr, "Помилка декомпресії ZLIB: %d\n", result);
        return -1;
    }
    return 0;
}
