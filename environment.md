# Setting up Development Environment

The code is provided in a Visual Studio Code project already setup. It was made using VS2019. You'll need to do the following to start development:

- Install Vulkan SDK
- Download glm
- Download glfw
- Download stb_image.h
- Download LuaJIT (technically Lua would also work)
- Follow LuaJIT's [Install Instructions](https://luajit.org/install.html) - Make sure to use windows 64-bit! You have to run `Developer Command Prompt for VS [year]` and inside that run the command `VsDevCmd -arch=amd64`. You may have to add the word "static" when calling `msvcbuild.bat`.
- Download LuaBridge
- Open project in Visual Studio
- Go to Project->properties and configure the following properties:
	- In C/C++, Additional Include Directories should point to the Include directories of glm, glfw, stb (root folder), luajit (src folder), luabridge (Source folder) and vulkan
	- In Linker, Additional Library Directories should point to the libraries folders for Vulkan, glfw, and the folder you installed LuaJIT into (it should have lua51.dll in it)
- You may also need to install the Windows SDK

It is recommended to also install the GLSL extension for Visual Studio and configure it (Tools->Options->GLSL Language Integration) so that the external compiler is set to glslc.exe (full file path, e.g. `C:\VulkanSDK\1.1.130.0\Bin\glslc.exe`)

Additionally, you can follow the instructions [here](https://stackoverflow.com/questions/57538385/running-spir-v-compiler-as-pre-build-event-in-vs2017-if-only-the-shader-code-was/57540462#57540462) to make it compile your shader files on build if they have been modified. If adding another shader you should set the build command for it to `glslc.exe "%(FullPath)" -o "%(DefiningProjectDirectory)resources\%(RelativeDir)%(Filename)-vert.spv"` and the output to `%(DefiningProjectDirectory)resources\%(RelativeDir)%(Filename)-vert.spv` (and -frag respective to the file extension, to avoid collisions).

# Libraries Used

Library versions are just what I used. If no version mentioned, it was the latest one available

## Not included in the project

These libraries need to be downloaded and configured to be included manually, as per the "Setting up Development Environment" above

- Vulkan (1.1.130.0)
- GLM (0.9.9.7)
- GLFW (3.3.1)
- stb
- LuaJIT (2.1.0-beta3)
- LuaBridge

## Included inside `/libs` folder in the project

Decisions to include these in the repo have been arbitrarily made by the author.

- Simon Perreault's Octree
- Stanislav's Frustum Cull
