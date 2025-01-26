# Rendering Shaders as Your Desktop Wallpaper

Recently, I found an [article](https://www.codeproject.com/articles/856020/draw-behind-desktop-icons-in-windows-plus) about drawing behind the icons on your desktop. This inspired me to set up a basic DirectX 12 renderer and attach it to the specific window mentioned in
the article to see what would happen. It actually turned out pretty well, so I decided to grab some fancy animated shaders from [Shadertoy](https://www.shadertoy.com/), translate them from GLSL to HLSL, and use them as dynamic desktop backgrounds.

## Build Requirements

If you want to build the project, ensure you have the following installed:
- **[CMake](https://cmake.org/)** (version >= 3.29)
- **[Ninja](https://ninja-build.org/)**
- **DirectX 12**

Building steps are the following:

1. Clone this repository to your local machine.
2. Run the `build.bat` script from the directory, where the repository resides.

This will compile the project and create a folder named `ready`, which contains:
- Two executables - `wp.exe` and `wp_manager.exe`
- Several shader (`.hlsl`) files

## Usage

To start rendering a shader as your desktop wallpaper, run the following command:

```bash
wp_manager.exe start <shader_file.hlsl> <frame rate limit>
```

For example:
```bash
wp_manager.exe start snap.hlsl 24
```

The command for stopping the rendering and restoring your standard wallpaper is:

```bash
wp_manager.exe stop
```

If you want to add your custom shaders, you can use the template file `shaders/pixel.hlsl` and place the finished shaders in the same folder as `wp.exe` and `wp_manager.exe`
