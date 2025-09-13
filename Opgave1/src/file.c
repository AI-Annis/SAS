#include <stdio.h> // fprintf, stdout, stderr.
#include <stdlib.h>  // exit, EXIT_FAILURE, EXIT_SUCCESS
#include <string.h>  // strcmp
#include <errno.h>  // errno

enum file_type {   
    DATA,
    EMPTY,
    ASCII,
    ISO_8859,
    UTF8
};

//Array med strings for hver filtype
const char * const FILE_TYPE_STRINGS[] = {
  "data",
  "empty",
  "ASCII text",
  "ISO-8859 text",
  "UTF-8 Unicode text",
};

//Hjælpe funktioner til at tjekke byte typer
int is_valid_utf8(FILE *f, int first_byte, int *bytes_read) {
    int expected_bytes;

    if ((first_byte & 0x80) == 0x00) { // 1-byte ASCII
        *bytes_read = 1;
        return 1;
    } else if ((first_byte & 0xE0) == 0xC0) { // 2-byte
        expected_bytes = 2;
    } else if ((first_byte & 0xF0) == 0xE0) { // 3-byte
        expected_bytes = 3;
    } else if ((first_byte & 0xF8) == 0xF0) { // 4-byte
        expected_bytes = 4;
    } else {
        return 0; // ulovlig startbyte
    }

    for (int i = 1; i < expected_bytes; i++) {
        int c = fgetc(f);
        if (c == EOF || (c & 0xC0) != 0x80) {
            return 0;
        }
    }

    *bytes_read = expected_bytes;
    return 1;
}

int print_error(char *path, int errnum) {
    return fprintf(stdout, "%s: cannot determine (%s)\n", path, strerror(errnum));
}

// Ser efter om den er empty eller data
// Hvis data, så tjekker den ASCII/ISO/UTF-8

enum file_type detect_filetype(const char *path) {
    FILE *f = fopen(path, "rb");
    // 2.3 Error handling
    if (!f) {
        print_error((char *)path, errno);  // brug helper
        return DATA; // stadig klassificeret som data
    }

    // 3 Filtyper (15%) 
    int c = fgetc(f);
    if (c == EOF) {
        fclose(f);
        return EMPTY; // tom fil
    }

    int is_ascii = 1; // antag at filen er ASCII indtil vi finder andet
    int is_iso = 1;// antag at filen er ISO-8859 indtil vi finder andet
    int is_utf8 = 1;// antag at filen er UTF-8 indtil vi finder andet
    int saw_multibyte_utf8 = 0;

    do {
        unsigned char b = (unsigned char)c;

        // ASCII check
        if (!((b >= 0x07 && b <= 0x0D) ||
              (b == 0x1B) ||
              (b >= 0x20 && b <= 0x7E))) {
            is_ascii = 0; // fundet et ikke-ASCII tegn
        }

        // ISO-8859 check
        if (!(is_ascii || (b >= 0xA0)) || (b >= 0x80 && b <= 0x9F)) {
            is_iso = 0; // ulovlige bytes i ISO
        }

        // UTF-8 check
        if (is_utf8) {
            int bytes_read = 0;
            if (!is_valid_utf8(f, b, &bytes_read)) {
                is_utf8 = 0;
            } else {    
                if (bytes_read > 1) { 
                    saw_multibyte_utf8 = 1;
                }

                // spring continuation bytes over
                for (int i = 1; i < bytes_read; i++) {
                    c = fgetc(f);
                    if (c == EOF) {
                        is_utf8 = 0;
                        break;
                    }
                }
            }
        }

    } while ((c = fgetc(f)) != EOF);
    
    fclose(f);

    if (is_ascii) return ASCII; // kun ASCII tegn
    if (is_iso) return ISO_8859;
    if (is_utf8 && saw_multibyte_utf8)  return UTF8;
    return DATA;
}

// Opgave 2.1  
int main(int argc, char *argv[]) {
    // Accept exactly one argument
    if (argc != 2) {
      // 2.2: ingen argumenter -> usage + EXIT_FAILURE
      fprintf(stderr, "Usage: file path\n");
      return EXIT_FAILURE;
    }

    enum file_type t = detect_filetype(argv[1]);        //
    printf("%s: %s\n", argv[1], FILE_TYPE_STRINGS[t]);  //
   
    return EXIT_SUCCESS;                                
} 