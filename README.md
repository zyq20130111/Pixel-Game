# Pixel World 3D

一个使用 C++、OpenGL 3.3 和 GLFW 编写的程序生成像素风 3D 场景。

## 功能

- 第一人称摄像机
- `W` / `S`：前进、后退
- `A` / `D`：向左、向右平移
- `Esc`：退出
- 程序生成的天空盒、草地、道路、树木、房屋和岩石
- 无需外部模型、纹理或图片资源

## 构建

需要安装 CMake 3.20 或更高版本，以及支持 C++17 的编译器。配置阶段会通过 CMake `FetchContent` 下载 GLFW，因此首次构建需要网络连接。

```bash
cmake -S . -B build
cmake --build build --config Release
```

运行程序：

- Windows Visual Studio 生成器：`build/Release/Pixel-Game.exe`
- 单配置生成器：`build/Pixel-Game`

如果使用 Visual Studio，也可以打开 `build/Pixel-Game.sln` 后运行 `Pixel-Game` 目标。
