#include <stdio.h>   // fprintf, stdout, stderr, fopen, fgetc, fclose
#include <stdlib.h>  // exit, EXIT_FAILURE, EXIT_SUCCESS
#include <string.h>  // strerror
#include <errno.h>   // errno

// Enum filtyperne
enum file_type {
    EMPTY,
    ASCII,
    ISO_8859,
    UTF8,
    DATA
};


// Array af strings for hver filtype
const char * const FILE_TYPE_STRINGS[] = {
    "empty",
    "ASCII text",
    "ISO-8859 text",
    "UTF-8 Unicode text",
    "data",
};

// Funktion til at tjekke om en byte er ASCII
int is_ascii_byte(int c) {
    return (c == 0x07 || c == 0x08 || (c >= 0x09 && c <= 0x0D) ||
            c == 0x1B || (c >= 0x20 && c <= 0x7E));
}

// Funktion til at tjekke om en byte er ISO-8859-1
int is_iso_8859_byte(int c) {
    return is_ascii_byte(c) || (c >= 160 && c <= 255);
}

// Funktion til at tjekke om en byte er starten på en gyldig UTF-8 sekvens
int is_valid_utf8_sequence(FILE *f, int first_byte, int *bytes_read) {
    int expected_bytes;
    if ((first_byte & 0x80) == 0x00) { // 1-byte sequence (ASCII)
        *bytes_read = 1;
        return 1;
    } else if ((first_byte & 0xE0) == 0xC0) { // 2-byte sekvens
        expected_bytes = 2;
    } else if ((first_byte & 0xF0) == 0xE0) { // 3-byte sekvens
        expected_bytes = 3;
    } else if ((first_byte & 0xF8) == 0xF0) { // 4-byte sekvens
        expected_bytes = 4;
    } else {
        return 0; // Forkert valid byte
    }

    // Tjekker de efterfølgende bytes 
    for (int i = 1; i < expected_bytes; i++) {
        int next_byte = fgetc(f);
        if (next_byte == EOF || (next_byte & 0xC0) != 0x80) { // Skal starte med  10xxxxxx
            return 0; // Ugyldig eller EOF
        }
    }
    *bytes_read = expected_bytes;
    return 1;
}

// Funktion til at determinere filtyperne
enum file_type determine_file_type(FILE *f) {
    int c;
    int is_empty = 1;
    int all_ascii = 1;
    int all_iso_8859 = 1;
    int all_utf8 = 1;

    while ((c = fgetc(f)) != EOF) {
        is_empty = 0; // File is not empty

        //Tjeker binary bytes som bryder alle tekstyper
        if (c == 0x00 || (c >= 0x01 && c <= 0x06) || (c >= 0x0E && c <= 0x1F)) {
        all_ascii = 0;
        all_iso_8859 = 0;
        all_utf8 = 0;
        continue;
    }
        // Tjekker ASCII
        if (!is_ascii_byte(c)) all_ascii = 0;

        // Tjekker ISO-8859-1
        if (!is_iso_8859_byte(c)) all_iso_8859 = 0;

        // Tjekker UTF-8
        if (all_utf8) {
            int bytes_read;
            if (!is_valid_utf8_sequence(f, c, &bytes_read)) {
                all_utf8 = 0;
            } else {
                // Consume continuation bytes
                for (int i = 1; i < bytes_read; i++) fgetc(f);
            }
        }
    }

    if (is_empty) return EMPTY;
    if (all_ascii) return ASCII;
    if (all_iso_8859) return ISO_8859;
    if (all_utf8) return UTF8;
    return DATA; // Fallback for  binary/non-text filer
}

int main(int argc, char *argv[]) {
    // Tjekker før en argument
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file_path\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Åbner filen i binary mode
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        // Reported as data hvis filen ikke kan åbnes
        printf("%s: data\n", argv[1]);
        return EXIT_SUCCESS;
    }

    // Determinerer fil type
    enum file_type type = determine_file_type(f);

    // Print resultaetet
    printf("%s: %s\n", argv[1], FILE_TYPE_STRINGS[type]);

    // Lukker filen
    fclose(f);

    return EXIT_SUCCESS;
}