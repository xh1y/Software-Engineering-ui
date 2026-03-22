#!/bin/bash

OUTPUT="${1:-extracted_files.txt}"   # first argument or default

# Clear the output file (or create it)
> "$OUTPUT"

# Find matching files, skip excluded directories
find . -type d \( -name "cmake-build-debug" -o -name "build" -o -name "3rdparty" \) -prune -o \
       -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" \) -print0 | while IFS= read -r -d '' file; do
    # Write relative path (strip leading "./")
    relative="${file#./}"
    echo "$relative" >> "$OUTPUT"
    # Append file content
    cat "$file" >> "$OUTPUT"
done

echo "All files have been extracted to $OUTPUT"