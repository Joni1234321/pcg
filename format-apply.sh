#!/usr/bin/env bash
# Apply clang-format to every C/C++ source file under src/
# Usage:  ./format-apply.sh           (formats in-place)
#         ./format-apply.sh --check   (dry-run, exits non-zero on diffs)

set -euo pipefail

CHECK=0
[[ "${1:-}" == "--check" ]] && CHECK=1

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$SCRIPT_DIR/src"

# Prefer $CLANG_FORMAT, then the MSYS2 build matching the toolchain, then whatever
# is on PATH. Debian/Ubuntu only ship versioned names, so glob those too.
find_clang_format() {
    local candidate
    for candidate in "${CLANG_FORMAT:-}" \
                     /d/tools/msys64/clang64/bin/clang-format \
                     "$(command -v clang-format 2>/dev/null || true)" \
                     /usr/lib/llvm-*/bin/clang-format \
                     /usr/bin/clang-format-[0-9]*; do
        [[ -n "$candidate" && -x "$candidate" ]] && { echo "$candidate"; return 0; }
    done
    return 1
}

CLANG_FORMAT="$(find_clang_format)" \
    || { echo "error: clang-format not found (tried PATH, /usr/lib/llvm-*/bin, /usr/bin/clang-format-*)"; exit 1; }

mapfile -d '' files < <(find "$SRC_DIR" -type f \
    \( -name "*.cpp" -o -name "*.cppm" -o -name "*.ixx" -o -name "*.hpp" -o -name "*.h" -o -name "*.c" -o -name "*.cc" -o -name "*.cxx" \) \
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
