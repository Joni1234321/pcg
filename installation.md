
# Installing vcpkg 
https://learn.microsoft.com/en-us/vcpkg/get_started/get-started?pivots=shell-powershell
```bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg; .\bootstrap-vcpkg.bat
$env:VCPKG_ROOT = "D:\tools\vcpkg"
$env:PATH = "$env:VCPKG_ROOT;$env:PATH"
```


# Installing msys2
```bash
pacman -S \
  mingw-w64-ucrt-x86_64-toolchain \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-sdl3 \
  mingw-w64-ucrt-x86_64-sdl3-image \
  mingw-w64-ucrt-x86_64-sdl3-ttf \
  mingw-w64-ucrt-x86_64-zlib
```

```bash
pacman -S \
  mingw-w64-clang-x86_64-toolchain \
  mingw-w64-clang-x86_64-cmake \
  mingw-w64-clang-x86_64-ninja \
  mingw-w64-clang-x86_64-sdl3 \
  mingw-w64-clang-x86_64-sdl3-image \
  mingw-w64-clang-x86_64-sdl3-ttf \
  mingw-w64-clang-x86_64-zlib
```

```bash
pacman -S \
  mingw-w64-x86_64-toolchain \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-sdl3 \
  mingw-w64-x86_64-sdl3-image \
  mingw-w64-x86_64-sdl3-ttf \
  mingw-w64-x86_64-zlib
```