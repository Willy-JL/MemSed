# MemSed
MEMory Search and EDit for Linux. NOT YET FUNCTIONAL!

## System Requirements

- Linux (other platforms might be supported in the future) with `/proc` fs
- OpenGL 3.0+ capable GPU driver

## Building

### Build Requirements

- Clang
- CMake 3.20+
- Python 3.10+ (for dear_bindings and GLAD generation)

### Build Instructions

```console
git clone -j $(nproc) --recursive https://github.com/Willy-JL/MemSed
cd MemSed
cmake --preset release
cmake --build -j $(nproc) --preset release
```
The executable will be located at `./build/release/memsed`

### Development Tips

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

Configuration files are provided for VS Code

You will need to run a full build process atleast once before `clangd` picks up the compile DB

Press `Ctrl+Shift+P` and run `clangd: Restart language server` if things ever go wrong
