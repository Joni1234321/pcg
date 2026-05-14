#!/usr/bin/env bash
# Apply clang-format to every C/C++ source file under src/
# Usage:  ./format-apply.sh           (formats in-place)
#         ./format-apply.sh --check   (dry-run, exits non-zero on diffs)

set -euo pipefail

CHECK=0
[[ "${1:-}" == "--check" ]] && CHECK=1

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$SCRIPT_DIR/src"

# Prefer the MSYS2 clang-format matching the toolchain
CLANG_FORMAT="/d/tools/msys64/clang64/bin/clang-format"
if [[ ! -x "$CLANG_FORMAT" ]]; then
    CLANG_FORMAT="$(command -v clang-format 2>/dev/null)" \
        || { echo "error: clang-format not found"; exit 1; }
fi

mapfile -d '' files < <(find "$SRC_DIR" -type f \
    \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.c" -o -name "*.cc" -o -name "*.cxx" \) \
    -print0)

echo "clang-format : $CLANG_FORMAT"
echo "files        : ${#files[@]}"

if (( CHECK )); then
    bad=()
    for f in "${files[@]}"; do
        "$CLANG_FORMAT" --dry-run --Werror "$f" 2>/dev/null || bad+=("$f")
    done
    if (( ${#bad[@]} > 0 )); then
        echo -e "\nFiles needing formatting:"
        printf '  %s\n' "${bad[@]}"
        exit 1
    fi
    echo "All files already formatted."
    exit 0
fi

for f in "${files[@]}"; do
    "$CLANG_FORMAT" -i "$f"
done
echo "Formatted ${#files[@]} files."
