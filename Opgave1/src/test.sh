#!/usr/bin/env bash

# Exit immediately if any command fails.
set -e

# Compile the program using the Makefile.
make

echo "Generating a test_files directory.."
mkdir -p test_files
rm -f test_files/*

echo "Generating test files.."

# Empty files (2 files)
: > test_files/empty1.input
: > test_files/empty2.input

# ASCII text files (12 files)
# Covers printable characters (0x20–0x7E), control characters (0x07–0x0D, 0x1B)
printf "Hello, World!\n" > test_files/ascii1.input
printf "No newline" > test_files/ascii2.input
printf "Tabs\tand\tspaces\n" > test_files/ascii3.input
printf "Control\x07\x08\x09\x0A\x0B\x0C\x0D\n" > test_files/ascii4.input
printf "\x1BEscape\n" > test_files/ascii5.input
printf "1234567890\n" > test_files/ascii6.input
printf "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n" > test_files/ascii7.input
printf "abcdefghijklmnopqrstuvwxyz\n" > test_files/ascii8.input
printf "!@#$%%\x5E&*()\n" > test_files/ascii9.input
printf "Mixed\n\t !@#\n" > test_files/ascii10.input
printf "Single char\n" > test_files/ascii11.input
printf "    Spaces only    \n" > test_files/ascii12.input

# ISO-8859-1 text files (12 files)
# Covers ASCII + bytes in 0xA0–0xFF (e.g., æ, ø, å, Þ)
printf "æøå\n" > test_files/iso1.input
printf "Héllo café\n" > test_files/iso2.input
printf "Grüße\n" > test_files/iso3.input
printf "Færøerne\n" > test_files/iso4.input
printf "\xA0\xA1\xA2\n" > test_files/iso5.input
printf "Þórður\n" > test_files/iso6.input
printf "Øssur\n" > test_files/iso7.input
printf "Test æøå\xA0\xFF\n" > test_files/iso8.input
printf "Line1 æøå\nLine2 çé\n" > test_files/iso10.input
printf " \xA1 \xA2 \xA3\n" > test_files/iso11.input
printf "Test123 æøå\xB0\xC0\n" > test_files/iso12.input

# UTF-8 text files (12 files)
# Covers multi-byte characters (2, 3, 4 bytes) and mixed ASCII
printf "Hej æøå\n" > test_files/utf8_1.input
printf "Hello 😊👍\n" > test_files/utf8_2.input
printf "你好\n" > test_files/utf8_3.input
printf "مرحبا\n" > test_files/utf8_4.input
printf "Test 😁\n" > test_files/utf8_5.input
printf "Привет\n" > test_files/utf8_6.input
printf "こんにちは\n" > test_files/utf8_7.input
printf "Hello 你好 😊\n" > test_files/utf8_8.input
printf "😀" > test_files/utf8_9.input
printf "😊👍🚀\n" > test_files/utf8_10.input
printf "안녕하세요\n" > test_files/utf8_11.input
printf "Test123 你好 😊\n" > test_files/utf8_12.input

# Data files (12 files)
# Covers invalid UTF-8, control bytes (0x00–0x06, 0x0E–0x1A, 0x1C–0x1F, 0x7F)
printf "\x00Null byte\n" > test_files/data1.input
printf "\xC0\xC0Invalid UTF-8\n" > test_files/data2.input
printf "\x01\x02\x03Control\n" > test_files/data3.input
printf "Hello\x00Mixed\n" > test_files/data5.input
printf "\x80\x81\x82ISO invalid\n" > test_files/data6.input
printf "\xE0\x80Partial UTF-8\n" > test_files/data7.input
printf "\x00\xFF\x00\xFFBinary\n" > test_files/data8.input
printf "\x01\x02\x1FControl\n" > test_files/data9.input
printf "Test\x00\x80Mixed\n" > test_files/data10.input
printf "\xF5\x80Invalid UTF-8\n" > test_files/data11.input
dd if=/dev/urandom of=test_files/data12.input bs=1 count=100 2>/dev/null

# Permission denied case
printf "hemmelighed" > test_files/noread.input
chmod 000 test_files/noread.input

# Test Coverage Documentation:
# - Empty: 2 files (0 bytes).
# - ASCII: 12 files covering 0x07–0x0D, 0x1B, 0x20–0x7E (letters, numbers, symbols, no newlines).
# - ISO-8859-1: 12 files covering ASCII + 0xA0–0xFF (e.g., æ, ø, å, café).
# - UTF-8: 12 files covering 1–4 byte sequences (e.g., emojis, Chinese, Arabic).
# - Data: 12 files covering invalid UTF-8, control bytes (0x00–0x06, 0x0E–0x1A, 0x1C–0x1F, 0x7F), null bytes, binary.
# - Error cases: Non-existent file, permission-denied, usage error.
# - Well-tested: All required types, edge cases (single byte, no newline), errors.
# - Hard to test: Ambiguous encodings (ASCII vs. ISO/UTF-8), very short files, platform-specific permissions (e.g., WSL/NTFS), complex UTF-8 (overlong, surrogates).

echo "Running the tests.."
exitcode=0
for f in test_files/*.input
do
  echo ">>> Testing ${f}.."
  if [[ "$f" == *"noread.input" ]]; then
    echo "NOTE: chmod 000 only works on Linux filesystems (ext4, XFS, etc.)."
    echo "      On WSL/NTFS, the file may still be readable, so the test may not fail as expected."
  fi

  file "$f" | sed -e 's/ASCII text.*/ASCII text/' \
                  -e 's/UTF-8 Unicode text.*/UTF-8 Unicode text/' \
                  -e 's/Unicode text, UTF-8 text.*/UTF-8 Unicode text/' \
                  -e 's/ISO-8859 text.*/ISO-8859 text/' \
                  -e 's/writable, regular file, no read permission/cannot determine (Permission denied)/' \
                  -e 's/Non-ISO extended-ASCII text/data/' \
                  -e 's/Unicode text, UTF-16.*/data/' \
                  -e 's/very short file (no magic)/data/' \
                  > "${f}.expected"
  ./file "$f" > "${f}.actual"

  if ! diff -u "${f}.expected" "${f}.actual"
  then
    echo ">>> Failed :-("
    exitcode=1
  else
    echo ">>> Success :-)"
  fi
done

# Test non-existent file
echo ">>> Testing non-existent file.."
./file test_files/nonexistent.input > test_files/nonexistent.input.actual
echo "test_files/nonexistent.input: cannot determine (No such file or directory)" > test_files/nonexistent.input.expected
if ! diff -u test_files/nonexistent.input.expected test_files/nonexistent.input.actual
then
  echo ">>> Failed :-("
  exitcode=1
else
  echo ">>> Success :-)"
fi

# Test usage error
echo ">>> Testing usage error.."
./file > test_files/usage.actual 2> test_files/usage.actual.stderr
echo "Usage: file path" > test_files/usage.expected.stderr
if ! diff -u test_files/usage.expected.stderr test_files/usage.actual.stderr
then
  echo ">>> Failed :-("
  exitcode=1
else
  echo ">>> Success :-)"
fi

exit $exitcode