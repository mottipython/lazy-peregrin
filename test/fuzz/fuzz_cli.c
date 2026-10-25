/*
 * fuzz_cli.c - Standalone fuzz target for Anathem screensavers CLI argument parsing.
 * Dual-mode: LLVM libFuzzer (-DLIBFUZZER) or standalone runner with ASan/UBSan.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_ARGS 64
#define MAX_BUF_LEN 4096

/* Mocked option variables */
static int opt_window = 0;
static double opt_speed = 1.0;
static uint64_t opt_seed = 0;
static const char *opt_snapshot = NULL;
static double opt_time = 0.0;
static int opt_w = 640;
static int opt_h = 400;

/* Anathem specialized options */
static int opt_day = 0;       /* --date MM-DD (analemma, page-trees) */
static double opt_hours = 0;  /* --at HH:MM (analemma) */
static double opt_shift = 0;  /* --at HH:MM (edhar-clock) */
static int opt_lapse = 1;     /* --no-lapse (edhar-clock) */
static int opt_min = 10;      /* --maths / --min / --side (lazy-peregrin, teglon) */
static int opt_max = 15;      /* --maths / --max / --side (lazy-peregrin, teglon) */
static int opt_strict = 0;    /* --strict (teglon) */

static void reset_options(void)
{
    opt_window = 0;
    opt_speed = 1.0;
    opt_seed = 0;
    opt_snapshot = NULL;
    opt_time = 0.0;
    opt_w = 640;
    opt_h = 400;

    opt_day = 0;
    opt_hours = 0;
    opt_shift = 0;
    opt_lapse = 1;
    opt_min = 10;
    opt_max = 15;
    opt_strict = 0;
}

/* Parse CLI options with robust bounds checking */
static int parse_args(int argc, char **argv)
{
    static const int BEFORE[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    static const int DAYS_IN_MO[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!a) continue;

        /* Windows screensaver flags */
        if ((a[0] == '/' || a[0] == '-') && (a[1] == 's' || a[1] == 'S') && !a[2]) continue;
        else if ((a[0] == '/' || a[0] == '-') && (a[1] == 'p' || a[1] == 'P') && (!a[2] || a[2] == ':')) return 0;
        else if ((a[0] == '/' || a[0] == '-') && (a[1] == 'c' || a[1] == 'C') && (!a[2] || a[2] == ':')) return 0;

        /* Standard screensaver flags */
        else if (!strcmp(a, "--window")) opt_window = 1;
        else if (!strcmp(a, "--fullscreen")) opt_window = 0;
        else if (!strcmp(a, "--help") || !strcmp(a, "-h")) return 1; /* Usage requested */
        else if (!strcmp(a, "--speed") && i + 1 < argc) {
            opt_speed = atof(argv[++i]);
            if (opt_speed < 0.05) opt_speed = 0.05;
            if (opt_speed > 100.0) opt_speed = 100.0;
        }
        else if (!strcmp(a, "--seed") && i + 1 < argc) {
            opt_seed = (uint64_t)strtoull(argv[++i], NULL, 10);
        }
        else if (!strcmp(a, "--snapshot") && i + 1 < argc) {
            opt_snapshot = argv[++i];
        }
        else if (!strcmp(a, "--time") && i + 1 < argc) {
            opt_time = atof(argv[++i]);
            if (opt_time < 0.0) opt_time = 0.0;
            if (opt_time > 86400.0) opt_time = 86400.0;
        }
        else if (!strcmp(a, "--size") && i + 1 < argc) {
            const char *arg = argv[++i];
            const char *x = strchr(arg, 'x');
            int w = atoi(arg);
            int h = x ? atoi(x + 1) : 0;
            if (w < 64 || h < 64 || w > 8192 || h > 8192) return 1;
            opt_w = w;
            opt_h = h;
        }

        /* Specialized flags */
        else if (!strcmp(a, "--date") && i + 1 < argc) {
            const char *arg = argv[++i];
            const char *dash = strchr(arg, '-');
            if (!dash) return 1;
            int mo = atoi(arg);
            int dy = atoi(dash + 1);
            if (mo < 1 || mo > 12 || dy < 1 || dy > DAYS_IN_MO[mo - 1]) return 1;
            opt_day = BEFORE[mo - 1] + dy - 1;
        }
        else if (!strcmp(a, "--at") && i + 1 < argc) {
            const char *arg = argv[++i];
            const char *colon = strchr(arg, ':');
            if (!colon) return 1;
            int hh = atoi(arg);
            int mm = atoi(colon + 1);
            if (hh < 0 || hh > 23 || mm < 0 || mm > 59) return 1;
            opt_hours = hh + mm / 60.0;
            opt_shift = hh * 3600.0 + mm * 60.0;
        }
        else if (!strcmp(a, "--no-lapse")) {
            opt_lapse = 0;
        }
        else if (!strcmp(a, "--maths") && i + 1 < argc) {
            int m = atoi(argv[++i]);
            if (m < 6) m = 6;
            if (m > 18) m = 18;
            opt_min = opt_max = m;
        }
        else if (!strcmp(a, "--strict")) {
            opt_strict = 1;
        }
        else if (!strcmp(a, "--side") && i + 1 < argc) {
            int s = atoi(argv[++i]);
            if (s < 2) s = 2;
            if (s > 48) s = 48;
            opt_min = opt_max = s;
        }
        else if (!strcmp(a, "--min") && i + 1 < argc) {
            opt_min = atoi(argv[++i]);
            if (opt_min < 2) opt_min = 2;
            if (opt_min > 48) opt_min = 48;
        }
        else if (!strcmp(a, "--max") && i + 1 < argc) {
            opt_max = atoi(argv[++i]);
            if (opt_max < 2) opt_max = 2;
            if (opt_max > 48) opt_max = 48;
        }
        else {
            return 1; /* Unknown or malformed flag */
        }
    }

    if (opt_max < opt_min) opt_max = opt_min;
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    if (Size == 0 || Size > MAX_BUF_LEN) return 0;

    char buf[MAX_BUF_LEN];
    memcpy(buf, Data, Size);
    buf[Size < sizeof(buf) ? Size : sizeof(buf) - 1] = '\0';

    char *argv[MAX_ARGS];
    int argc = 0;
    argv[argc++] = "fuzz_target";

    /* Tokenize by whitespace */
    char *p = buf;
    while (*p && argc < MAX_ARGS - 1) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
        if (*p) {
            *p = '\0';
            p++;
        }
    }
    argv[argc] = NULL;

    reset_options();
    parse_args(argc, argv);
    return 0;
}

#ifndef LIBFUZZER
int main(int argc, char **argv)
{
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            FILE *f = fopen(argv[i], "rb");
            if (!f) continue;
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            if (sz > 0 && sz <= MAX_BUF_LEN) {
                uint8_t *data = malloc(sz);
                if (data && fread(data, 1, sz, f) == (size_t)sz) {
                    LLVMFuzzerTestOneInput(data, sz);
                }
                free(data);
            }
            fclose(f);
        }
    } else {
        /* Built-in comprehensive regression smoke corpus */
        const char *tests[] = {
            "--window --speed 2.5",
            "--fullscreen",
            "--help",
            "-h",
            "--size 1920x1080 --time 10.0",
            "--size 64x64",
            "--size 8192x8192",
            "--size 100000x100000",
            "--size 0x0",
            "--size -100x-100",
            "--size abcxdef",
            "--time 0.0",
            "--time 86400.0",
            "--time -5.0",
            "--time 100000000",
            "--seed 18446744073709551615",
            "--snapshot /tmp/out.bmp",
            "--date 06-21",
            "--date 02-29",
            "--date 02-31",
            "--date 13-01",
            "--date 00-15",
            "--date invalid",
            "--at 14:30",
            "--at 00:00",
            "--at 23:59",
            "--at 24:00",
            "--at 25:99",
            "--at -1:00",
            "--at invalid",
            "--no-lapse",
            "--maths 12",
            "--maths 2",
            "--maths 50",
            "--strict --side 8",
            "--min 4 --max 16",
            "/s",
            "/S",
            "/p:12345",
            "/c",
            "--unknown-flag",
            "--size",
            "--time",
            "--date",
            "--at",
            "--speed",
            "%s%s%s%s%s%n"
        };
        for (size_t i = 0; i < sizeof(tests)/sizeof(tests[0]); i++) {
            LLVMFuzzerTestOneInput((const uint8_t *)tests[i], strlen(tests[i]));
        }
    }
    printf("All fuzz CLI smoke tests completed successfully.\n");
    return 0;
}
#endif
