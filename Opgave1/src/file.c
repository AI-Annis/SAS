#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Provided print_error function
int print_error(char *path, int errnum) {
    return fprintf(stdout, "%s: cannot determine (%s)\n",
                   path, strerror(errnum));
}

// Check if byte is a valid ASCII character according to spec
int is_ascii_char(unsigned char c) {
    return (c >= 0x07 && c <= 0x0D) || c == 0x1B || (c >= 0x20 && c <= 0x7E);
}

// Check if byte is valid ISO-8859-1 character according to spec
int is_iso_8859_char(unsigned char c) {
    return is_ascii_char(c) || (c >= 160);
}

// Check if buffer is valid UTF-8
int is_valid_utf8(const unsigned char *buf, size_t len) {
    size_t i = 0;
    while (i < len) {
        if (buf[i] <= 0x7F) {          // 1-byte ASCII
            i++;
        } else if (i + 1 < len && (buf[i] & 0xE0) == 0xC0 &&
                   (buf[i+1] & 0xC0) == 0x80) { // 2-byte
            i += 2;
        } else if (i + 2 < len && (buf[i] & 0xF0) == 0xE0 &&
                   (buf[i+1] & 0xC0) == 0x80 && (buf[i+2] & 0xC0) == 0x80) { // 3-byte
            i += 3;
        } else if (i + 3 < len && (buf[i] & 0xF8) == 0xF0 &&
                   (buf[i+1] & 0xC0) == 0x80 && (buf[i+2] & 0xC0) == 0x80 &&
                   (buf[i+3] & 0xC0) == 0x80) { // 4-byte
            i += 4;
        } else {
            return 0; // invalid UTF-8
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: file path\n");
        exit(EXIT_FAILURE);
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        print_error(argv[1], errno);
        exit(EXIT_SUCCESS);
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        print_error(argv[1], errno);
        fclose(fp);
        exit(EXIT_SUCCESS);
    }

    long file_size = ftell(fp);
    if (file_size < 0) {
        print_error(argv[1], errno);
        fclose(fp);
        exit(EXIT_SUCCESS);
    }

    if (file_size == 0) {
        fprintf(stdout, "%s: empty\n", argv[1]);
        fclose(fp);
        exit(EXIT_SUCCESS);
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        print_error(argv[1], errno);
        fclose(fp);
        exit(EXIT_SUCCESS);
    }

    unsigned char *buffer = malloc(file_size);
    if (!buffer) {
        print_error(argv[1], ENOMEM);
        fclose(fp);
        exit(EXIT_SUCCESS);
    }

    size_t bytes_read = fread(buffer, 1, file_size, fp);
    if (bytes_read != (size_t)file_size) {
        print_error(argv[1], errno);
        free(buffer);
        fclose(fp);
        exit(EXIT_SUCCESS);
    }
    fclose(fp);

    int all_ascii = 1;
    int all_iso = 1;

    for (size_t i = 0; i < bytes_read; i++) {
        if (!is_ascii_char(buffer[i])) all_ascii = 0;
        if (!is_iso_8859_char(buffer[i])) all_iso = 0;
    }

    // Determine type in priority order
    if (all_ascii) {
    fprintf(stdout, "%s: ASCII text\n", argv[1]);
} else if (all_iso) {
    fprintf(stdout, "%s: ISO-8859 text\n", argv[1]);
} else {
    fprintf(stdout, "%s: data\n", argv[1]);
}


    free(buffer);
    return EXIT_SUCCESS;
}
