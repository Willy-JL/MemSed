# MemSed
MEMory Search and EDit for Linux

Heavily inspired by basic Cheat Engine workflow (search value, make value change, search new value, save address and modify it)

> [!WARNING]
> This project is still a work in progress! \
> It should work for the most part, but adjust your expectations accordingly. \
> Check [TODO.md](TODO.md) for things yet to be implemented.

![Screenshot of MemSed](.github/assets/preview.png)

## System Requirements

- Linux (other platforms might be supported in the future) with `/proc` fs
- OpenGL 3.0+ capable GPU driver

## Install

### Arch Linux

Thanks to [ImperatorStorm](https://github.com/ImperatorStorm), we have an [AUR package](https://aur.archlinux.org/packages/memsed-git)! \
Install with your favorite AUR helper, for example with `yay`:
```bash
yay -S memsed-git
```
Then, run from terminal with `sudo memsed`.

### Universal Binary

Will work on most Linux distros, needs to be updated manually.
```bash
curl -Lo memsed https://github.com/WillyJL/MemSed/releases/latest/download/memsed
sudo install memsed -D -t /usr/local/bin/
rm memsed
```
Then, run from terminal with `sudo memsed`. \
If your distro does not support `/usr/local/bin/` in PATH, try `/usr/bin/` instead.

## Building

### Build Requirements

- GCC
- CMake 3.20+
- GNU Make
- Python 3.10+ (for dear_bindings and GLAD generation)

### Build Instructions

```console
git clone -j $(nproc) --recursive https://github.com/WillyJL/MemSed
cd MemSed
cmake --preset release
cmake --build -j $(nproc) --preset release
```
The executable will be located at `./build/release/memsed`

## Development Tips

While developing, it is best to first configure with the `debug` preset:
```console
cmake --preset debug
```
Only need to run this (configure step) the first time, and when creating/deleting files

You can specify a target in the `cmake` build command:
```console
cmake --build -j $(nproc) --preset debug -t run
```

Perform a clean build by adding `--clean-first` to the build command too

Quickly start attached to debugger with:
```console
cmake --build -j $(nproc) --preset debug -t gdb
```

### IDE Support

Configuration files are provided for VS Code and they rely on `clangd`

You will need to run a full build process atleast once before `clangd` picks up the compile DB

Press `Ctrl+Shift+P` and run `clangd: Restart language server` if things ever go wrong

### Project Structure

- `build/`: output and working directory for compilation
- `build/current/`: symlink to `build/debug/` or `build/release/`
- `lib/`: mostly generated code that the project depends on
- `lib/vendor/`: submodules used to generate code in `lib/`, or compiled as is
- `resources/`: assets used by the program, converted to C code in `lib/`
- `src/`: code specific to this project
- `src/process/`: platform-specific implementation of basic process primitives
- `src/thread/`: platform-specific implementation of basic thread primitives
- everything else in `src/` should be fairly platform-agnostic

## ❤️ Support
If you enjoy this program please __**spread the word!**__ And if you really love it, maybe consider donating? :D

> **[Ko-fi](https://ko-fi.com/willyjl)**: One-off or Recurring, No signup required

> **[PayPal](https://paypal.me/willyjl1)**: One-off, Signup required

> **BTC**: `1EnCi1HF8Jw6m2dWSUwHLbCRbVBCQSyDKm`

**Thank you <3**
