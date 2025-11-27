#include "../include/k88_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <limits.h>

#define CHUNK 16384

#define K88_MAGIC "K88ARC\n"

static uint32_t to_be32(uint32_t v) {
    return ((v >> 24) & 0xff) | ((v >> 8) & 0xff00) | ((v << 8) & 0xff0000) | ((v << 24) & 0xff000000);
}

static uint64_t to_be64(uint64_t v) {
    uint64_t r = 0;
    for (int i = 0; i < 8; ++i) r = (r << 8) | ((v >> (56 - i*8)) & 0xff);
    return r;
}

static int write_all(FILE *f, const void *buf, size_t len) {
    return fwrite(buf, 1, len, f) == len ? 0 : -1;
}

static int add_file_entry(FILE *archive, const char *relpath, const char *fullpath) {
    struct stat st;
    if (stat(fullpath, &st) != 0) return -1;

    unsigned char type = 'F';
    uint32_t path_len = (uint32_t)strlen(relpath);
    uint64_t fsize = (uint64_t)st.st_size;
    uint32_t mode = (uint32_t)(st.st_mode & 07777);

    if (write_all(archive, &type, 1) != 0) return -1;
    uint32_t be32 = to_be32(path_len);
    if (write_all(archive, &be32, sizeof(be32)) != 0) return -1;
    if (write_all(archive, relpath, path_len) != 0) return -1;
    uint64_t be64 = to_be64(fsize);
    if (write_all(archive, &be64, sizeof(be64)) != 0) return -1;
    uint32_t be_mode = to_be32(mode);
    if (write_all(archive, &be_mode, sizeof(be_mode)) != 0) return -1;

    FILE *in = fopen(fullpath, "rb");
    if (!in) return -1;
    unsigned char buf[CHUNK];
    size_t r;
    while ((r = fread(buf, 1, CHUNK, in)) > 0) {
        if (fwrite(buf, 1, r, archive) != r) { fclose(in); return -1; }
    }
    fclose(in);
    return 0;
}

static int add_path_recursive(FILE *archive, const char *base_dir, const char *path, const char *relbase) {
    struct stat st;
    if (stat(path, &st) != 0) return -1;

    if (S_ISDIR(st.st_mode)) {
        unsigned char type = 'D';
        size_t rel_len = strlen(relbase);
        uint32_t be32 = to_be32((uint32_t)rel_len);
        uint32_t mode = (uint32_t)(st.st_mode & 07777);
        uint32_t be_mode = to_be32(mode);

        if (write_all(archive, &type, 1) != 0) return -1;
        if (write_all(archive, &be32, sizeof(be32)) != 0) return -1;
        if (write_all(archive, relbase, rel_len) != 0) return -1;
        uint64_t be64 = to_be64(0);
        if (write_all(archive, &be64, sizeof(be64)) != 0) return -1;
        if (write_all(archive, &be_mode, sizeof(be_mode)) != 0) return -1;

        DIR *d = opendir(path);
        if (!d) return -1;
        struct dirent *e;
        while ((e = readdir(d)) != NULL) {
            if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
            char child_full[MY_PATH_MAX];
            char child_rel[MY_PATH_MAX];
            snprintf(child_full, MY_PATH_MAX, "%s/%s", path, e->d_name);
            if (strlen(relbase) == 0) snprintf(child_rel, MY_PATH_MAX, "%s", e->d_name);
            else snprintf(child_rel, MY_PATH_MAX, "%s/%s", relbase, e->d_name);
            if (add_path_recursive(archive, base_dir, child_full, child_rel) != 0) { closedir(d); return -1; }
        }
        closedir(d);
        return 0;
    } else if (S_ISREG(st.st_mode)) {
        return add_file_entry(archive, relbase, path);
    }
    return 0;
}

int compress_file(const char *input_path, const char *output_path) {
    char tmpl[64];
    snprintf(tmpl, sizeof(tmpl), "/tmp/k88arc_%d.tmp", (int)getpid());
    FILE *archive = fopen(tmpl, "wb");
    if (!archive) { perror("fopen tmp"); return -1; }

    if (write_all(archive, K88_MAGIC, strlen(K88_MAGIC)) != 0) { fclose(archive); unlink(tmpl); return -1; }

    struct stat st;
    if (stat(input_path, &st) != 0) { perror("stat"); fclose(archive); unlink(tmpl); return -1; }

    if (S_ISDIR(st.st_mode)) {
        if (add_path_recursive(archive, input_path, input_path, "") != 0) { fclose(archive); unlink(tmpl); return -1; }
    } else if (S_ISREG(st.st_mode)) {
        const char *base = strrchr(input_path, '/');
        if (base) base++;
        else base = input_path;
        if (add_file_entry(archive, base, input_path) != 0) { fclose(archive); unlink(tmpl); return -1; }
    } else {
        fclose(archive); unlink(tmpl); return -1;
    }

    fflush(archive);
    fclose(archive);

    FILE *in_fp = fopen(tmpl, "rb");
    if (!in_fp) { perror("fopen temp archive"); unlink(tmpl); return -1; }
    FILE *out_fp = fopen(output_path, "wb");
    if (!out_fp) { perror("fopen output"); fclose(in_fp); unlink(tmpl); return -1; }

    int result = Z_OK;
    int flush;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
    z_stream strm;
    strm.zalloc = Z_NULL; strm.zfree = Z_NULL; strm.opaque = Z_NULL;
    if (deflateInit(&strm, Z_DEFAULT_COMPRESSION) != Z_OK) {
        fprintf(stderr, "Error initializing zlib.\n"); fclose(in_fp); fclose(out_fp); unlink(tmpl); return -1;
    }

    do {
        strm.avail_in = fread(in, 1, CHUNK, in_fp);
        if (ferror(in_fp)) { result = -1; break; }
        flush = feof(in_fp) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;
        do {
            strm.avail_out = CHUNK; strm.next_out = out;
            result = deflate(&strm, flush);
            if (result == Z_STREAM_ERROR) { result = -1; goto cleanup; }
            size_t have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, out_fp) != have || ferror(out_fp)) { result = -1; goto cleanup; }
        } while (strm.avail_out == 0);
    } while (flush != Z_FINISH);

    if (result != Z_STREAM_END) result = -1;
    deflateEnd(&strm);

cleanup:
    fclose(in_fp);
    fclose(out_fp);
    unlink(tmpl);

    if (result != Z_OK && result != Z_STREAM_END && result != 0) {
        fprintf(stderr, "ZLIB compression error: %d\n", result);
        return -1;
    }
    return 0;
}