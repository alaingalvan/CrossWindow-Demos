# Hello Triangle

Uses [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross) to convert GLSL to HLSL, GLSL ES, and Metal.

```bash
# 🔨 Let's build SPIRV-Cross and GLSLangValidator to compile our shaders
# These were included in the `/external/` folder of this repo:
cd ../../external/spirv-cross
mkdir build
cmake ..
cmake --build . --config Release

cd ../glslangvalidator
mkdir build
cmake ..
cmake --build . --config Release

cd ../../demos/hello-triangle/assets/shaders
mkdir build

# 🌈 Compile shaders to SPIR-V binary
../../../../external/glslang/build/StandAlone/Release/glslangValidator -V triangle.vert -o build/triangle.spv

# HLSL
../../../../external/spirv-cross/build/Release/spirv-cross --hlsl build/triangle.spv -o build/triangle.hlsl

# OpenGL ES 3.1
../../../../external/spirv-cross/build/Release/spirv-cross --version 310 --es build/triangle.spv -o build/triangle.glsl

# Metal
../../../../external/spirv-cross/build/Release/spirv-cross --msl build/triangle.spv -o build/triangle.msl
```

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