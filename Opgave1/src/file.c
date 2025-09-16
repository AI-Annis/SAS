#include <stdio.h>   // fprintf, stdout, stderr
#include <stdlib.h>  // exit, EXIT_FAILURE, EXIT_SUCCESS
#include <string.h>  // strerror
#include <errno.h>   // errno

// Filtyper
enum file_type {
    DATA,
    EMPTY,
    ASCII,
    ISO_8859,
    UTF8
};

// Strings til output
const char * const FILE_TYPE_STRINGS[] = {
    "data",
    "empty",
    "ASCII text",
    "ISO-8859 text",
    "UTF-8 Unicode text",
};

// Bruges til fejlbeskeder (skal printe til STDOUT og stadig EXIT_SUCCESS i 2.3)
int print_error(char *path, int errnum) {
    return fprintf(stdout, "%s: cannot determine (%s)\n", path, strerror(errnum));
}

// ASCII-lignende pr. opgaven
static int is_ascii_like(unsigned char b) {
    return ((b >= 0x07 && b <= 0x0D) || b == 0x1B || (b >= 0x20 && b <= 0x7E));
}

// Tjek første byte for hvor mange UTF-8 bytes vi forventer
static int utf8_expected_bytes(unsigned char b) {
    if ((b & 0x80) == 0x00) return 1;        // 0xxxxxxx
    else if ((b & 0xE0) == 0xC0) return 2;   // 110xxxxx
    else if ((b & 0xF0) == 0xE0) return 3;   // 1110xxxx
    else if ((b & 0xF8) == 0xF0) return 4;   // 11110xxx
    else return 0; // ulovlig startbyte
}

// Find filtype
int detect_filetype(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        print_error((char *)path, errno);  // 2.3: én linje til stdout
        return -1;                         // marker fejl (main printer ikke type)
    }

    int c = fgetc(f);
    if (c == EOF) {   // tom fil
        fclose(f);
        return EMPTY;
    }

    int is_ascii = 1;
    int is_iso = 1;
    int is_utf8 = 1;
    int saw_multibyte_utf8 = 0;

    do {
        unsigned char b = (unsigned char)c;

        // --- ASCII check (per byte) ---
        if (!is_ascii_like(b)) is_ascii = 0;

        // --- ISO-8859-1 check (per byte) ---
        if (!(is_ascii_like(b) || b >= 0xA0) || (b >= 0x80 && b <= 0x9F)) {
            is_iso = 0;
        }

        // --- UTF-8 check ---
        if (is_utf8) {
            int expected = utf8_expected_bytes(b);
            if (expected == 0) {
                is_utf8 = 0;
            } else if (expected > 1) {
                saw_multibyte_utf8 = 1;
                // Tjek continuation-bytes og opdatér også ASCII/ISO på dem
                for (int i = 1; i < expected; i++) {
                    int d = fgetc(f);
                    if (d == EOF || ((d & 0xC0) != 0x80)) {
                        is_utf8 = 0;
                        break;
                    }
                    unsigned char cont = (unsigned char)d;
                    if (!is_ascii_like(cont)) is_ascii = 0;
                    if (!(is_ascii_like(cont) || cont >= 0xA0) || (cont >= 0x80 && cont <= 0x9F)) {
                        is_iso = 0;
                    }
                }
            }
        }

        // videre til næste lead byte
    } while ((c = fgetc(f)) != EOF);

    fclose(f);

    if (is_ascii) return ASCII;
    if (is_utf8 && saw_multibyte_utf8) return UTF8; // undgå at kalde binære/ASCII+NUL for UTF-8
    if (is_iso) return ISO_8859;
    return DATA;
}

// --- main ---
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: file path\n"); // 2.2: stderr + EXIT_FAILURE
        return EXIT_FAILURE;
    }

    int t = detect_filetype(argv[1]);
    if (t >= 0) {
        printf("%s: %s\n", argv[1], FILE_TYPE_STRINGS[t]); // 2.1: stdout + EXIT_SUCCESS
    }
    // Ved I/O-fejl har detect_filetype allerede printet korrekt linje, og vi skal stadig EXIT_SUCCESS
    return EXIT_SUCCESS;
}