// ====================================================================
// src/compressor.c
// Максимально проста реалізація стиснення ОДНОГО файлу через zlib.
// ====================================================================

#include "../include/k88_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h> // Головний заголовок бібліотеки zlib

// Визначаємо розмір буфера для читання/запису (16 KB)
#define CHUNK 16384 

/**
 * @brief Виконує повне стиснення файлу за допомогою бібліотеки zlib (алгоритм Deflate).
 * * @param input_path Шлях до вхідного файлу.
 * @param output_path Шлях до вихідного архіву.
 * @param encrypt Прапорець шифрування (ігнорується в цій простій версії).
 * @return 0 у разі успіху, -1 у разі помилки.
 */
int compress_file(const char *input_path, const char *output_path, bool encrypt) {
    FILE *in_fp = NULL;
    FILE *out_fp = NULL;
    int result = Z_OK;
    int flush;
    
    // Буфери для вхідних та вихідних даних
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
    
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    
    // 1. Ініціалізація zlib (Deflate)
    if (deflateInit(&strm, Z_DEFAULT_COMPRESSION) != Z_OK) {
        fprintf(stderr, "Помилка ініціалізації zlib.\n");
        return -1;
    }

    // 2. Відкриття файлів
    in_fp = fopen(input_path, "rb");
    if (in_fp == NULL) { perror("Помилка відкриття вхідного файлу"); result = -1; goto cleanup; }

    out_fp = fopen(output_path, "wb");
    if (out_fp == NULL) { perror("Помилка відкриття вихідного файлу"); result = -1; goto cleanup; }
    
    // 3. Стиснення блоками (основний цикл)
    do {
        // Читаємо дані у вхідний буфер
        strm.avail_in = fread(in, 1, CHUNK, in_fp);
        if (ferror(in_fp)) {
            result = -1; break;
        }
        
        // Визначаємо, чи це останній блок
        flush = feof(in_fp) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in; // Встановлюємо вказівник на дані для стиснення

        // Стискаємо доти, доки є вхідні дані
        do {
            strm.avail_out = CHUNK; 
            strm.next_out = out;

            // Виклик функції стиснення
            result = deflate(&strm, flush);
            
            // Якщо сталася помилка, виходимо
            if (result == Z_STREAM_ERROR) {
                result = -1; goto cleanup;
            }

            // Запис стиснених даних у файл
            size_t have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, out_fp) != have || ferror(out_fp)) {
                result = -1; goto cleanup;
            }

        } while (strm.avail_out == 0); // Повторюємо, якщо вихідний буфер був заповнений

    } while (flush != Z_FINISH); // Продовжуємо до завершення файлу

    // Перевірка фінального результату стиснення
    if (result != Z_STREAM_END) {
        result = -1; // Фінальна помилка
    }
    
    // 4. Очищення та звільнення ресурсів
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