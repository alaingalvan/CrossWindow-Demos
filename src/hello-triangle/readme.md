# Hello Triangle

Uses [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross) to convert GLSL to HLSL, GLSL ES, and Metal.

## Setup

First we're going to need to build our tools to compile our shader, GLSLangValidator (comes with the Vulkan SDK, but is also bundled as a submodule here) and SPIRV-Cross.

```bash
# 🔨 Let's build SPIRV-Cross and GLSLangValidator to compile our shaders
# These were included in the `/external/` folder of this repo:
cd ../../external/spirv-cross
mkdir spirv-cross
cd spirv-cross
cmake ..
cmake --build . --config Release

cd ../glslangvalidator
mkdir build
cd build
cmake ..
cmake --build . --config Release

cd ../../demos/hello-triangle/assets/shaders
mkdir build
```

Now we can compile our shaders for the example:

```bash
# 🌋 Compile shaders to SPIR-V binary
../../../../external/glslang/build/StandAlone/Release/glslangValidator -V triangle.vert -o build/triangle.vert.spv
../../../../external/glslang/build/StandAlone/Release/glslangValidator -V triangle.frag -o build/triangle.frag.spv

# ❎ HLSL
../../../../external/spirv-cross/spirv-cross/Release/spirv-cross build/triangle.vert.spv --hlsl --output build/triangle.vert.hlsl
../../../../external/spirv-cross/spirv-cross/Release/spirv-cross build/triangle.frag.spv --hlsl --output build/triangle.frag.hlsl

# ⚪ OpenGL ES 3.1
../../../../external/spirv-cross/spirv-cross/Release/spirv-cross build/triangle.vert.spv --version 310 --es --output build/triangle.vert.glsl
../../../../external/spirv-cross/spirv-cross/Release/spirv-cross build/triangle.frag.spv --version 310 --es --output build/triangle.frag.glsl

# 🤖 Metal
../../../../external/spirv-cross/spirv-cross/Release/spirv-cross build/triangle.vert.spv --msl --output build/triangle.vert.msl
../../../../external/spirv-cross/spirv-cross/Release/spirv-cross build/triangle.frag.spv --msl --output build/triangle.frag.msl
```

Finally we need to build our actual example:

```bash
# 🖼️ To build your Visual Studio solution on Windows x64
mkdir visualstudio
cd visualstudio
cmake .. -A x64

# 🍎 To build your XCode project On Mac OS for Mac OS / iOS
mkdir xcode
cd xcode
cmake .. -G Xcode

# 🐧 To build your .make file on Linux
mkdir make
cd make
cmake ..
```