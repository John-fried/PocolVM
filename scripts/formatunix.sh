#!/bin/sh

# Usage: ./formatunix.sh <file_or_directory>
# This script ensures C source files are POSIX-compliant for GCC/Clang.

if [ $# -eq 0 ]; then
    echo "Error: No input file or directory specified."
    echo "Usage: $0 <path>"
    exit 1
fi

process_file() {
    target="$1"

    # Check if file is actually a text file to avoid corrupting binaries
    if ! file "$target" | grep -qE 'text|source'; then
        return
    fi

    # 1. Convert Encoding to UTF-8
    current_enc=$(file -b --mime-encoding "$target")

    if [ "$current_enc" != "utf-8" ] && [ "$current_enc" != "us-ascii" ]; then
        echo "[Encoding] Converting $target from $current_enc to utf-8"
        iconv -f "$current_enc" -t UTF-8 "$target" -o "$target.tmp" && mv "$target.tmp" "$target"
    fi

    # 2. Remove BOM (Byte Order Mark) if present
    # Some Windows editors add a BOM which can confuse older C preprocessors
    sed -i '1s/^\xef\xbb\xbf//' "$target"

    # 3. Convert CRLF to LF
    sed -i 's/\r$//' "$target"

    # 4. Ensure Trailing Newline
    # C standards (ISO C) require files to end with a newline to be valid translation units
    if [ -n "$(tail -c 1 "$target" 2>/dev/null)" ]; then
        echo "" >> "$target"
        echo "[Newline] Fixed missing trailing newline in $target"
    fi

    echo "Successfully sanitized: $target ✅"
}

# Recursive processing if directory, otherwise process single file
if [ -d "$1" ]; then
    find "$1" -type f \( -name "*.c" -o -name "*.h" \) | while read -r f; do
        process_file "$f"
    done
else
    process_file "$1"
fi