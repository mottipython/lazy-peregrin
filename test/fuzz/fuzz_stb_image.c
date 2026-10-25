/*
 * fuzz_stb_image.c - Standalone dual-mode fuzz harness for stb_image PNG/JPEG decoding
 * Designed for Anathem Screensavers Suite (Milestone 3).
 *
 * Supported Modes:
 * 1. LLVM libFuzzer mode: compiled with -DLIBFUZZER -fsanitize=fuzzer,address,undefined
 * 2. Standalone regression mode: compiled without -DLIBFUZZER, links standard main().
 *    Executes against file list, directory of corpus files, or built-in smoke/regression seeds.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_NO_THREAD_LOCALS
#include "stb_image.h"

/* Max input size accepted by fuzz target to prevent fuzz-engine OOM */
#define MAX_INPUT_SIZE (10 * 1024 * 1024)

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    if (Size == 0 || Size > MAX_INPUT_SIZE) return 0;

    int w = 0, h = 0, comp = 0;

    /* 1. Exercise header probing */
    stbi_info_from_memory((const stbi_uc *)Data, (int)Size, &w, &h, &comp);

    /* 2. Exercise full image decoding with 4 requested channels (RGBA),
     * matching the exact usage across all 13 Anathem screensavers:
     * sprite[i].px = stbi_load_from_memory(..., 4); */
    int dec_w = 0, dec_h = 0, dec_comp = 0;
    stbi_uc *px = stbi_load_from_memory((const stbi_uc *)Data, (int)Size, &dec_w, &dec_h, &dec_comp, 4);
    if (px) {
        stbi_image_free(px);
    }

    return 0;
}

#ifndef LIBFUZZER

/* Minimal valid 1x1 32-bit RGBA PNG */
static const uint8_t seed_1x1_png[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
    0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
    0x08, 0x06, 0x00, 0x00, 0x00, 0x1f, 0x15, 0xc4,
    0x89, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41,
    0x54, 0x78, 0x9c, 0x63, 0x00, 0x01, 0x00, 0x00,
    0x05, 0x00, 0x01, 0x0d, 0x0a, 0x2d, 0xb4, 0x00,
    0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae,
    0x42, 0x60, 0x82
};

/* Regression seed: Zero-length IDAT chunk PNG (57 bytes) triggering UBSan in unpatched stb_image.h */
static const uint8_t seed_empty_idat_png[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
    0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
    0x08, 0x06, 0x00, 0x00, 0x00, 0x1f, 0x15, 0xc4,
    0x89, 0x00, 0x00, 0x00, 0x00, 0x49, 0x44, 0x41,
    0x54, 0x38, 0xd8, 0x2c, 0xc4, 0x00, 0x00, 0x00,
    0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60,
    0x82
};

/* Corrupted truncated PNG header */
static const uint8_t seed_corrupt_png[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
    0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x01
};

static void test_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz > 0 && sz <= MAX_INPUT_SIZE) {
        uint8_t *buf = (uint8_t *)malloc(sz);
        if (buf && fread(buf, 1, sz, f) == (size_t)sz) {
            LLVMFuzzerTestOneInput(buf, sz);
        }
        free(buf);
    }
    fclose(f);
}

static void test_dir_or_file(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) return;
    if (S_ISDIR(st.st_mode)) {
        DIR *d = opendir(path);
        if (!d) return;
        struct dirent *ent;
        char subpath[1024];
        while ((ent = readdir(d)) != NULL) {
            if (ent->d_name[0] == '.') continue;
            snprintf(subpath, sizeof(subpath), "%s/%s", path, ent->d_name);
            test_file(subpath);
        }
        closedir(d);
    } else {
        test_file(path);
    }
}

int main(int argc, char **argv)
{
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            test_dir_or_file(argv[i]);
        }
    } else {
        /* Run built-in regression suite */
        printf("Running built-in stb_image regression seeds...\n");
        LLVMFuzzerTestOneInput(seed_1x1_png, sizeof(seed_1x1_png));
        LLVMFuzzerTestOneInput(seed_empty_idat_png, sizeof(seed_empty_idat_png));
        LLVMFuzzerTestOneInput(seed_corrupt_png, sizeof(seed_corrupt_png));
        printf("Built-in regression suite completed successfully.\n");
    }
    return 0;
}
#endif
