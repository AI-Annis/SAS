#include <stdio.h>   // fprintf, stdout, stderr, fopen, fgetc, fclose
#include <stdlib.h>  // exit, EXIT_FAILURE, EXIT_SUCCESS
#include <string.h>  // strerror
#include <errno.h>   // errno

// Enum for file types
enum file_type {
    EMPTY,
    ASCII,
    ISO_8859,
    UTF8,
    DATA
};


// Array of user-friendly strings for each file type
const char * const FILE_TYPE_STRINGS[] = {
    "empty",
    "ASCII text",
    "ISO-8859 text",
    "UTF-8 Unicode text",
    "data",
};

// Function to check if a byte is ASCII-like
int is_ascii_byte(int c) {
    return (c == 0x07 || c == 0x08 || (c >= 0x09 && c <= 0x0D) ||
            c == 0x1B || (c >= 0x20 && c <= 0x7E));
}

// Function to check if a byte is ISO-8859-1-like (includes ASCII and 160-255)
int is_iso_8859_byte(int c) {
    return is_ascii_byte(c) || (c >= 160 && c <= 255);
}

// Function to check if a sequence of bytes forms a valid UTF-8 character
int is_valid_utf8_sequence(FILE *f, int first_byte, int *bytes_read) {
    int expected_bytes;
    if ((first_byte & 0x80) == 0x00) { // 1-byte sequence (ASCII)
        *bytes_read = 1;
        return 1;
    } else if ((first_byte & 0xE0) == 0xC0) { // 2-byte sequence
        expected_bytes = 2;
    } else if ((first_byte & 0xF0) == 0xE0) { // 3-byte sequence
        expected_bytes = 3;
    } else if ((first_byte & 0xF8) == 0xF0) { // 4-byte sequence
        expected_bytes = 4;
    } else {
        return 0; // Invalid first byte
    }

    // Check subsequent bytes
    for (int i = 1; i < expected_bytes; i++) {
        int next_byte = fgetc(f);
        if (next_byte == EOF || (next_byte & 0xC0) != 0x80) { // Must start with 10xxxxxx
            return 0; // Invalid or incomplete
        }
    }
    *bytes_read = expected_bytes;
    return 1;
}

// Function to determine the file type
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
        // Check ASCII
        if (!is_ascii_byte(c)) all_ascii = 0;

        // Check ISO-8859-1
        if (!is_iso_8859_byte(c)) all_iso_8859 = 0;

        // Check UTF-8
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
    return DATA; // Fallback for binary/non-text files
}

int main(int argc, char *argv[]) {
    // Check for exactly one argument
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file_path\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Open the file in binary mode
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        // If file cannot be opened, report as data
        printf("%s: data\n", argv[1]);
        return EXIT_SUCCESS;
    }

    // Determine file type
    enum file_type type = determine_file_type(f);

    // Print result
    printf("%s: %s\n", argv[1], FILE_TYPE_STRINGS[type]);

    // Close the file
    fclose(f);

    return EXIT_SUCCESS;
}