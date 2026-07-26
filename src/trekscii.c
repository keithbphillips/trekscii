/*
 * trekscii.c - Random colorized Star Trek ASCII art for your login shell
 *
 * Build:    make
 * Install:  ./install.sh
 * Usage:    trekscii [--help] [--version] [--list]
 *
 * Art directory resolution order (first hit wins):
 *   1. $TREKSCII_ART_DIR
 *   2. $XDG_DATA_HOME/trekscii_art  (default ~/.local/share/trekscii_art)
 *   3. <dir containing the executable>/../share/trekscii_art
 *   4. <dir containing the executable>/art or ../art (source checkout)
 *   5. /usr/local/share/trekscii_art
 *   6. /usr/share/trekscii_art
 */

/* strdup(), readlink() and friends under a strict -std=c11 */
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <sys/stat.h>

#define TREKSCII_VERSION "1.0.0"

#define RESET         "\033[0m"
#define MAX_FILES     1024
#define MAX_LINE      512
#define MAX_ART_LINES 512

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#ifndef NAME_MAX
#define NAME_MAX 255
#endif

/* (title_color, art_color_0, art_color_1, art_color_2) */
static const char *COLOR_SCHEMES[][4] = {
    { "\033[1;33m",  "\033[33m",  "\033[93m",  "\033[1;33m" }, /* Starfleet Gold     */
    { "\033[1;91m",  "\033[31m",  "\033[91m",  "\033[1;31m" }, /* Starfleet Red      */
    { "\033[1;94m",  "\033[34m",  "\033[94m",  "\033[1;34m" }, /* Starfleet Blue     */
    { "\033[1;91m",  "\033[91m",  "\033[93m",  "\033[31m"  }, /* Klingon Red/Gold   */
    { "\033[1;32m",  "\033[32m",  "\033[92m",  "\033[1;32m" }, /* Romulan Green      */
    { "\033[1;96m",  "\033[96m",  "\033[36m",  "\033[94m"  }, /* UFP Cyan/Blue      */
    { "\033[1;97m",  "\033[97m",  "\033[37m",  "\033[1;97m" }, /* DS9 Silver/White   */
    { "\033[1;93m",  "\033[93m",  "\033[33m",  "\033[1;93m" }, /* Ferengi Gold       */
    { "\033[1;93m",  "\033[97m",  "\033[93m",  "\033[33m"  }, /* Cardassian Tan     */
    { "\033[1;92m",  "\033[92m",  "\033[32m",  "\033[1;92m" }, /* Holodeck Green     */
};
#define NUM_SCHEMES (int)(sizeof(COLOR_SCHEMES) / sizeof(COLOR_SCHEMES[0]))

static void print_bar(const char *color, int width) {
    fputs(color, stdout);
    for (int i = 0; i < width; i++) fputs("\xe2\x94\x80", stdout); /* U+2500 ─ */
    puts(RESET);
}

static int is_dir(const char *path) {
    struct stat st;
    return path && *path && stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

/* Copy src into dst if it names an existing directory. Returns 1 on success. */
static int take_if_dir(char *dst, size_t dstsz, const char *src) {
    if (!is_dir(src)) return 0;
    snprintf(dst, dstsz, "%s", src);
    return 1;
}

/* Directory holding this executable, via /proc/self/exe. Returns 1 on success. */
static int exe_dir(char *out, size_t outsz) {
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len < 0) return 0;
    exe_path[len] = '\0';
    snprintf(out, outsz, "%s", dirname(exe_path));
    return 1;
}

/* Locate the art directory. Returns 1 and fills out on success. */
static int find_art_dir(char *out, size_t outsz) {
    /* Roomier than PATH_MAX so appending a suffix to a PATH_MAX base can't
       truncate; take_if_dir() rejects anything that doesn't resolve anyway. */
    char cand[PATH_MAX + 64];

    /* 1. Explicit override. If it is set but bogus, say so rather than
          silently rendering art from somewhere else. */
    const char *env = getenv("TREKSCII_ART_DIR");
    if (env && *env) {
        if (take_if_dir(out, outsz, env)) return 1;
        fprintf(stderr, "trekscii: TREKSCII_ART_DIR=%s is not a directory\n", env);
        return 0;
    }

    /* 2. Per-user XDG data dir - where install.sh puts the art */
    const char *xdg = getenv("XDG_DATA_HOME");
    const char *home = getenv("HOME");
    cand[0] = '\0';
    if (xdg && *xdg)
        snprintf(cand, sizeof(cand), "%s/trekscii_art", xdg);
    else if (home && *home)
        snprintf(cand, sizeof(cand), "%s/.local/share/trekscii_art", home);
    if (take_if_dir(out, outsz, cand)) return 1;

    /* 3/4. Relative to the executable: installed prefix, then source checkout */
    char dir[PATH_MAX];
    if (exe_dir(dir, sizeof(dir))) {
        snprintf(cand, sizeof(cand), "%s/../share/trekscii_art", dir);
        if (take_if_dir(out, outsz, cand)) return 1;
        snprintf(cand, sizeof(cand), "%s/art", dir);
        if (take_if_dir(out, outsz, cand)) return 1;
        snprintf(cand, sizeof(cand), "%s/../art", dir);
        if (take_if_dir(out, outsz, cand)) return 1;
    }

    /* 5/6. System-wide installs */
    if (take_if_dir(out, outsz, "/usr/local/share/trekscii_art")) return 1;
    if (take_if_dir(out, outsz, "/usr/share/trekscii_art")) return 1;

    return 0;
}

static void usage(FILE *out) {
    fprintf(out,
        "trekscii " TREKSCII_VERSION " - random colorized Star Trek ASCII art\n"
        "\n"
        "Usage: trekscii [OPTION]\n"
        "\n"
        "  -l, --list       list the available art files and exit\n"
        "  -h, --help       show this help and exit\n"
        "  -V, --version    show the version and exit\n"
        "\n"
        "Environment:\n"
        "  TREKSCII_ART_DIR   override the art directory\n"
        "                     (default: $XDG_DATA_HOME/trekscii_art,\n"
        "                      i.e. ~/.local/share/trekscii_art)\n");
}

int main(int argc, char **argv) {
    int list_only = 0;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            usage(stdout);
            return 0;
        } else if (!strcmp(a, "-V") || !strcmp(a, "--version")) {
            puts("trekscii " TREKSCII_VERSION);
            return 0;
        } else if (!strcmp(a, "-l") || !strcmp(a, "--list")) {
            list_only = 1;
        } else {
            fprintf(stderr, "trekscii: unknown option '%s'\n", a);
            usage(stderr);
            return 2;
        }
    }

    char art_dir[PATH_MAX];
    if (!find_art_dir(art_dir, sizeof(art_dir))) {
        fprintf(stderr, "trekscii: no art directory found "
                        "(looked in ~/.local/share/trekscii_art and next to the binary)\n");
        puts("Live long and prosper.");
        return 1;
    }

    /* Collect .txt filenames */
    DIR *dp = opendir(art_dir);
    if (!dp) {
        fprintf(stderr, "trekscii: cannot open %s\n", art_dir);
        puts("Live long and prosper.");
        return 1;
    }

    char *files[MAX_FILES];
    int nfiles = 0;
    struct dirent *ent;
    while ((ent = readdir(dp)) != NULL && nfiles < MAX_FILES) {
        size_t nl = strlen(ent->d_name);
        if (nl > 4 && strcmp(ent->d_name + nl - 4, ".txt") == 0)
            files[nfiles++] = strdup(ent->d_name);
    }
    closedir(dp);

    if (nfiles == 0) {
        fprintf(stderr, "trekscii: no .txt art files in %s\n", art_dir);
        puts("Live long and prosper.");
        return 1;
    }

    if (list_only) {
        for (int i = 0; i < nfiles; i++) {
            puts(files[i]);
            free(files[i]);
        }
        return 0;
    }

    /* Pick a random file and color scheme */
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    const char *chosen = files[rand() % nfiles];
    const char **scheme = COLOR_SCHEMES[rand() % NUM_SCHEMES];
    const char *title_color = scheme[0];

    char path[PATH_MAX + NAME_MAX + 2];
    snprintf(path, sizeof(path), "%s/%s", art_dir, chosen);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "trekscii: cannot read %s\n", path);
        puts("Live long and prosper.");
        return 1;
    }

    /* Read title (line 1) */
    char title[MAX_LINE] = {0};
    if (!fgets(title, sizeof(title), fp)) {
        fclose(fp);
        puts("Live long and prosper.");
        return 0;
    }
    title[strcspn(title, "\r\n")] = '\0'; /* strip newline */

    /* Read art lines, skip leading blanks */
    static char lines[MAX_ART_LINES][MAX_LINE];
    int nlines = 0;
    int started = 0;
    char buf[MAX_LINE];
    while (nlines < MAX_ART_LINES && fgets(buf, sizeof(buf), fp)) {
        buf[strcspn(buf, "\r\n")] = '\0';
        if (!started && buf[strspn(buf, " \t")] == '\0') continue; /* leading blank */
        started = 1;
        memcpy(lines[nlines++], buf, MAX_LINE);
        lines[nlines-1][MAX_LINE-1] = '\0';
    }
    fclose(fp);

    /* Strip trailing blank lines */
    while (nlines > 0 && lines[nlines-1][strspn(lines[nlines-1], " \t")] == '\0')
        nlines--;

    /* Compute bar width */
    int art_width = 40;
    for (int i = 0; i < nlines; i++) {
        int w = (int)strlen(lines[i]);
        if (w > art_width) art_width = w;
    }
    int title_width = (int)strlen(title) + 4;
    int bar_width = art_width > title_width ? art_width : title_width;
    if (bar_width < 40) bar_width = 40;
    if (bar_width > 80) bar_width = 80;

    /* Print */
    putchar('\n');
    print_bar(title_color, bar_width);
    printf("%s  %s%s\n", title_color, title, RESET);
    print_bar(title_color, bar_width);

    for (int i = 0; i < nlines; i++) {
        const char *color = scheme[1 + (i % 3)];
        printf("%s%s%s\n", color, lines[i], RESET);
    }
    putchar('\n');

    /* Cleanup */
    for (int i = 0; i < nfiles; i++) free(files[i]);
    return 0;
}
