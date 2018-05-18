# CrossWindow Demos

A variety of demos showcasing how to use CrossWindow to build cross platform applications.

## Getting started

```bash
git clone git@github.com/alaingalvan/CrossWindow-Demos --recurse-submodules
# If you forget to clone submodules you can always run
git submodule update --init

# On Windows
mkdir visualstudio
cd visualstudio
cmake ..

# On MacOS / iOS
mkdir xcode
cd xcode
cmake -G Xcode ..

# On Linux
mkdir make
cd make
cmake ..

# For Android
cmake -G "Android Gradle - Ninja" -DXWIN_OS=ANDROID

# For WebAssembly
cmake -DXWIN_OS=WASM
```

## Demos

### Cross Platform Cross API Hello Triangle

An example showcasing the rendering of a simple triangle in every supported graphics API and operating system. 

### Resizable UI Windows

How to create UIs that have resizable windows similar to image editors like Photoshop.


