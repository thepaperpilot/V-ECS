# Setting up Development Environment

The code is provided in a Visual Studio Code project already setup. It was made using VS2019. You'll need to do the following to start development:

- Install Vulkan SDK
- Download glm and glfw
- Download stb_image.h
- Open project in Visual Studio
- Go to Project->properties and configure the following properties:
	- In C/C++, Additional Include Directories should point to the Include directories of glm, glfw, stb, and vulkan
	- In Linker, Additional Library Directories should point to the libraries folders for Vulkan and glfw (use current settings as reference)
- You may also need to install the Windows SDK

It is recommended to also install the GLSL extension for Visual Studio and configure it (Tools->Options->GLSL Language Integration) so that the external compiler is set to glslc.exe (full file path, e.g. `C:\VulkanSDK\1.1.130.0\Bin\glslc.exe`)

Additionally, you can follow the instructions [here](https://stackoverflow.com/questions/57538385/running-spir-v-compiler-as-pre-build-event-in-vs2017-if-only-the-shader-code-was/57540462#57540462) to make it compile your shader files on build if they have been modified. I set the command to `glslc.exe "%(FullPath)" -o "%(RootDir)%(Directory)%(Filename)-vert.spv"` and the output to `%(RootDir)%(Directory)%(Filename)-vert.spv` and -frag respective to the file extension, to avoid collisions.
