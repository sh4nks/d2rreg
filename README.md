# d2rreg

This is a companion CLI tool for [sh4nks/d2rloader](http://github.com/sh4nks/d2rloader) when using the Linux version of the loader.

This will allow the loader to set the registry token from within *Wine* (it will be called with ``wine d2rreg.exe --update-registry="token"``).

The reasoning behind this is that there is no way to protect the authenticator tokens using ``CryptProtectData`` natively from Linux. However, it works when Wine calls it and updates the registry with the protected token.

This tool makes it possible for the [d2rloader](http://github.com/sh4nks/d2rloader) to support the authentication method "Token"!


This is the first time me writing C++ - Please be kind :D

## Building

### Linux (Cross-Compilation for Windows)

To build this project on Linux for Windows, you will need to have the MinGW-w64 toolchain installed.

**1. Install MinGW-w64:**

On Arch Linux, you can install it with:

```bash
sudo pacman -S mingw-w64-gcc
```

**2. Build the project:**

```bash
mkdir build
cmake -B build -DCMAKE_TOOLCHAIN_FILE=./toolchain-mingw.cmake
cmake --build build/
```

### Windows (Native)

To build this project on Windows, you will need to have Visual Studio and CMake installed.

**1. Build with CMake:**

Open a command prompt or PowerShell and run the following commands from the project's root directory:

```powershell
mkdir build
cmake -B build
cmake --build build/
```

# License

MIT License
