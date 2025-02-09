# MemSed
MEMory Search and EDit for Linux. NOT YET FUNCTIONAL!

## System Requirements

- Linux (other platforms might be supported in the future)
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
cmake -B build
cmake --build build -j $(nproc)
```
The executable will be located at `./build/memsed`

### Development Tips

You can specify a target in the last `cmake` command:
```console
cmake --build build -j $(nproc) -t run
```
Perform a clean build by adding `--clean-first` too
