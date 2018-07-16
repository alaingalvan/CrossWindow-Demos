# CrossWindow Demos

[![cmake-img]][cmake-url]
[![License][license-img]][license-url]

A variety of demos showcasing how to use CrossWindow to build cross platform applications.

## Getting Started

First install [Git](https://git-scm.com/downloads), then open any terminal such as [Hyper](https://hyper.is/) in any folder and type:

```bash
# 🐑 Clone the repo
git clone https://github.com/alaingalvan/crosswindow-demos --recurse-submodules

# 💿 go inside the folder
cd crosswindow-demos

# 👯 If you forget to `recurse-submodules` you can always run:
git submodule update --init

# 🔼 Go inside any demo, like for instance the Hello Triangle folder:
cd src/04-cross-platform-hello-triangle/

```

## Demos

### Window Creation

<!--![Window Creation Cover Image](src/01-window-creation/assets/cover.jpg)-->

A basic example showing how to create a window.

<!--
### Alert Message

![Alert Image](src/03-alert-message/assets/cover.jpg)

Creating alert messages for warnings, errors, or confirmations in your application.

## Open / Save Dialog

![Open/Save Dialog Image](src/03-open-save-dialog/assets/cover.jpg)

Creating open / save dialogs for grabbing files.

-->

### Cross Platform Hello Triangle

![Hello Triangle Cover Image](src/04-cross-platform-hello-triangle/assets/cover.jpg)

An example showcasing the rendering of a simple triangle in every modern graphics API (Vulkan / DirectX 12 / DirectX 11 / OpenGL / Metal) and operating system.

[cmake-img]: https://img.shields.io/badge/cmake-3.6-1f9948.svg?style=flat-square
[cmake-url]: https://cmake.org/
[license-img]: https://img.shields.io/:license-mit-blue.svg?style=flat-square
[license-url]: https://opensource.org/licenses/MIT