
# Download sdl3 
```shell
./setup.sh
```
```bash
sudo apt-get remove --purge clang-format-18
sudo apt-get autoremove

sudo add-apt-repository "deb http://apt.llvm.org/$(lsb_release -cs)/ llvm-toolchain-$(lsb_release -cs)-22 main"
wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | sudo tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc
sudo apt-get update
sudo apt-get install -y clang-format-22
sudo update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-22 100

clang-format --version

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