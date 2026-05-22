
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

## Donwload laptop something about keys
```bash
sudo apt install libc++-22-dev libc++abi-22-dev
```
```bash
# 1. Clean up duplicate LLVM repo files
sudo rm -f /etc/apt/sources.list.d/archive_uri-http_apt_llvm_org_noble_-noble.list \
           /etc/apt/sources.list.d/archive_uri-https_apt_llvm_org_noble_-noble.list \
           /etc/apt/sources.list.d/llvm-22.list

# 2. Import the GPG key (--no-check-certificate is acceptable here since we're just fetching a public key)
wget --no-check-certificate -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | sudo gpg --dearmor -o /etc/apt/trusted.gpg.d/llvm.gpg

# 3. Add the repo (using http to avoid TLS issue)
echo "deb [arch=amd64] http://apt.llvm.org/noble/ llvm-toolchain-noble-22 main" | sudo tee /etc/apt/sources.list.d/llvm-22.list

# 4. Install
sudo apt-get update && sudo apt-get install clang-22

# Move 
sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-22 100
sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-22 100
```

## Ubuntu
```bash
sudo apt-get install libasound2-dev libpulse-dev libx11-dev libxext-dev \
  libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev \
  libxkbcommon-dev libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev \
  libegl1-mesa-dev libdbus-1-dev libibus-1.0-dev libudev-dev \
  libpipewire-0.3-dev libwayland-dev libdecor-0-dev
sudo apt install libharfbuzz-dev
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