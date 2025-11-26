#include "../include/k88_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <limits.h>

#ifndef PATH_MAX
#define MY_PATH_MAX 4096
#else
#define MY_PATH_MAX PATH_MAX
#endif

#define CHUNK 16384
#define K88_MAGIC "K88ARC\n"

static uint32_t from_be32(uint32_t v) {
    return ((v >> 24) & 0xff) | ((v >> 8) & 0xff00) | ((v << 8) & 0xff0000) | ((v << 24) & 0xff000000);
}

static uint64_t from_be64(uint64_t v) {
    uint64_t r = 0;
    for (int i = 0; i < 8; ++i) r = (r << 8) | ((v >> (56 - i*8)) & 0xff);
    return r;
}

static int read_all(FILE *f, void *buf, size_t len) {
    return fread(buf, 1, len, f) == len ? 0 : -1;
}

int extract_file(const char *input_path, const char *output_dir) {
    // decompress input_path into a temporary file
    char tmpl[64];
    snprintf(tmpl, sizeof(tmpl), "/tmp/k88arc_in_%d.tmp", (int)getpid());
    FILE *tmp = fopen(tmpl, "wb");
    if (!tmp) { perror("fopen tmp"); return -1; }

    FILE *in_fp = fopen(input_path, "rb");
    if (!in_fp) { perror("fopen input"); fclose(tmp); unlink(tmpl); return -1; }

    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
    z_stream strm;
    strm.zalloc = Z_NULL; strm.zfree = Z_NULL; strm.opaque = Z_NULL;
    if (inflateInit(&strm) != Z_OK) { fprintf(stderr, "inflateInit failed\n"); fclose(in_fp); fclose(tmp); unlink(tmpl); return -1; }

    int result = Z_OK;
    do {
        strm.avail_in = fread(in, 1, CHUNK, in_fp);
        if (ferror(in_fp)) { result = -1; break; }
        if (strm.avail_in == 0) break;
        strm.next_in = in;
        do {
            strm.avail_out = CHUNK; strm.next_out = out;
            result = inflate(&strm, Z_NO_FLUSH);
            if (result == Z_STREAM_ERROR || result == Z_NEED_DICT || result == Z_DATA_ERROR || result == Z_MEM_ERROR) { inflateEnd(&strm); result = -1; goto cleanup; }
            size_t have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, tmp) != have || ferror(tmp)) { result = -1; goto cleanup; }
        } while (strm.avail_out == 0);
    } while (result != Z_STREAM_END);

    if (result != Z_STREAM_END) { result = -1; }
    inflateEnd(&strm);
    fflush(tmp); fclose(tmp); fclose(in_fp);

cleanup:
    if (result != Z_OK && result != Z_STREAM_END && result != 0) {
        // cleanup resources on error
        inflateEnd(&strm);
        if (in_fp) fclose(in_fp);
        if (tmp) { fflush(tmp); fclose(tmp); }
        unlink(tmpl);
        return -1;
    }

    // now read archive from tmpl and extract entries
    FILE *arc = fopen(tmpl, "rb");
    if (!arc) { unlink(tmpl); return -1; }

    // verify magic
    char magic[8] = {0};
    if (read_all(arc, magic, strlen(K88_MAGIC)) != 0) { fclose(arc); unlink(tmpl); return -1; }
    if (strncmp(magic, K88_MAGIC, strlen(K88_MAGIC)) != 0) { fprintf(stderr, "Invalid archive\n"); fclose(arc); unlink(tmpl); return -1; }

    while (1) {
        unsigned char type;
        if (fread(&type, 1, 1, arc) != 1) break; // EOF
        uint32_t be_pathlen;
        if (read_all(arc, &be_pathlen, sizeof(be_pathlen)) != 0) { fclose(arc); unlink(tmpl); return -1; }
        uint32_t pathlen = from_be32(be_pathlen);
        if (pathlen >= MY_PATH_MAX) { fclose(arc); unlink(tmpl); return -1; }
        char pathbuf[MY_PATH_MAX+1];
        memset(pathbuf, 0, sizeof(pathbuf));
        if (read_all(arc, pathbuf, pathlen) != 0) { fclose(arc); unlink(tmpl); return -1; }
        pathbuf[pathlen] = '\0';
        uint64_t be_fsize;
        if (read_all(arc, &be_fsize, sizeof(be_fsize)) != 0) { fclose(arc); unlink(tmpl); return -1; }
        uint64_t fsize = from_be64(be_fsize);
        uint32_t be_mode;
        if (read_all(arc, &be_mode, sizeof(be_mode)) != 0) { fclose(arc); unlink(tmpl); return -1; }
        uint32_t mode = from_be32(be_mode);

        char outpath[MY_PATH_MAX];
        snprintf(outpath, MY_PATH_MAX, "%s/%s", output_dir, pathbuf);

        if (type == 'D') {
            // create directory
            if (mkdir(outpath, mode) != 0) {
                // ignore EEXIST
            }
        } else if (type == 'F') {
            // ensure parent dir exists
            char parent[MY_PATH_MAX]; strncpy(parent, outpath, MY_PATH_MAX); char *p = strrchr(parent, '/');
            if (p) { *p = '\0'; mkdir(parent, 0755); }
            FILE *of = fopen(outpath, "wb");
            if (!of) { fclose(arc); unlink(tmpl); return -1; }
            uint64_t remaining = fsize;
            unsigned char buf[CHUNK];
            while (remaining > 0) {
                size_t toread = remaining > CHUNK ? CHUNK : (size_t)remaining;
                if (read_all(arc, buf, toread) != 0) { fclose(of); fclose(arc); unlink(tmpl); return -1; }
                if (fwrite(buf, 1, toread, of) != toread) { fclose(of); fclose(arc); unlink(tmpl); return -1; }
                remaining -= toread;
            }
            fclose(of);
            chmod(outpath, mode);
        } else {
            // unknown type, skip
            if (fseek(arc, fsize, SEEK_CUR) != 0) { fclose(arc); unlink(tmpl); return -1; }
        }
    }

    fclose(arc);
    unlink(tmpl);
    return 0;
}
