#!/usr/bin/env bash
# Setup script - downloads and extracts SDL3 libraries into 3rdparty/
# Run once after cloning: chmod +x setup.sh && ./setup.sh
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
THIRD_PARTY="$SCRIPT_DIR/3rdparty"
mkdir -p "$THIRD_PARTY"

download_lib() {
    local name="$1"
    local url="$2"
    local archive="$3"
    local extracted="$4"

    local dest="$THIRD_PARTY/$name"
    if [ -d "$dest" ]; then
        echo "$name already exists, skipping."
        return
    fi

    echo "Downloading $name..."
    curl -L "$url" -o "$THIRD_PARTY/$archive"

    echo "Extracting $name..."
    unzip -q "$THIRD_PARTY/$archive" -d "$THIRD_PARTY"

    mv "$THIRD_PARTY/$extracted" "$dest"
    rm "$THIRD_PARTY/$archive"
    echo "$name ready."
}

download_lib "SDL3"       \
    "https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.4.8.zip" \
    "SDL-release-3.4.8.zip" "SDL-release-3.4.8"

download_lib "SDL3_ttf"   \
    "https://github.com/libsdl-org/SDL_ttf/archive/refs/tags/release-3.2.2.zip" \
    "SDL_ttf-release-3.2.2.zip" "SDL_ttf-release-3.2.2"

download_lib "SDL3_image" \
    "https://github.com/libsdl-org/SDL_image/archive/refs/tags/release-3.4.4.zip" \
    "SDL_image-release-3.4.4.zip" "SDL_image-release-3.4.4"

echo ""
echo "All libraries set up. You can now build the project."
