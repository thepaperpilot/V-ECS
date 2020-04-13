cmake_minimum_required(VERSION 3.13)

# Default to debug build if not specified
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif()

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#### Setup Libraries ####

# ensure we have git
find_package(Git REQUIRED)

# when finding packages we'll sometimes need some FindXXXX.cmake files that may or may not exist,
# so we'll include them in our repo in the cmake directory
# TODO I've heard this is deprecated and there may be a better way to do this?
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Set up Hunter
include("cmake/HunterGate.cmake")
HunterGate(
    URL "https://github.com/cpp-pm/hunter/archive/v0.23.249.tar.gz"
    SHA1 "d45d77d8bba9da13e9290a180e0477e90accd89b"
)

#### Setup Project ####

project(V-ECS)
add_executable(vecs "")
add_subdirectory(src)

#### Add Hunter packages ####

# I like the ease of Hunter, but it seems to have an issue with packages becoming out of date
# I'm going to continue using it moving forward, since none of the out of date changes
# make too much of a difference for this project, but I hope that improves moving forward

# glslang https://hunter.readthedocs.io/en/latest/packages/pkg/glslang.html
# I was getting unresolved external symbol errors, so I'm just going to include the git repo
#hunter_add_package(glslang)
#find_package(glslang CONFIG REQUIRED)
#target_link_libraries(vecs PUBLIC glslang::glslang)

# glfw https://hunter.readthedocs.io/en/latest/packages/pkg/glfw.html
hunter_add_package(glfw)
find_package(glfw3 REQUIRED)
target_link_libraries(vecs PRIVATE glfw)

# glm https://hunter.readthedocs.io/en/latest/packages/pkg/glm.html
hunter_add_package(glm)
find_package(glm REQUIRED)
target_link_libraries(vecs PRIVATE glm)

# stb https://hunter.readthedocs.io/en/latest/packages/pkg/stb.html
hunter_add_package(stb)
find_package(stb CONFIG REQUIRED)
target_link_libraries(vecs PUBLIC stb::stb)

# HastyNoise https://hunter.readthedocs.io/en/latest/packages/pkg/HastyNoise.html
hunter_add_package(HastyNoise)
find_package(HastyNoise CONFIG REQUIRED)
target_link_libraries(vecs PRIVATE HastyNoise::hastyNoise)

# imgui https://hunter.readthedocs.io/en/latest/packages/pkg/imgui.html
# imgui was messing up so I'm using git to get the latest version (that doesn't mess up)
#hunter_add_package(imgui)
#find_package(imgui CONFIG REQUIRED)
#target_link_libraries(vecs PRIVATE imgui::imgui)

#### Add other packages ####

# Vulkan. This one must be installed by the user manually
# reference: https://vulkan.lunarg.com/doc/view/1.1.70.0/windows/getting_started.html#user-content-using-vulkan-in-cmake-projects
# I noticed Hunter has a package for vulkan headers, but 
find_package(Vulkan REQUIRED)
#target_compile_definitions(vecs PRIVATE VK_USE_PLATFORM_WIN32_KHR)
target_link_libraries(vecs PRIVATE ${Vulkan_LIBRARIES})
target_include_directories(vecs PRIVATE ${Vulkan_INCLUDE_DIRS})

# TODO use ExternalProject_Add for all these git repos?

# glslang
if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/glslang)
	execute_process(COMMAND git clone https://github.com/KhronosGroup/glslang ${CMAKE_CURRENT_BINARY_DIR}/glslang)
endif()
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/glslang)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/glslang)
target_link_libraries(vecs PRIVATE glslang SPIRV)

# LuaJIT
find_package(LuaJIT QUIET)
if (NOT LUAJIT_FOUND OR NOT LUAJIT_VERSION_STRING VERSION_EQUAL "2.1.0-beta3")
	include(ExternalProject)
	if (WIN32)
		ExternalProject_Add(luajit
			URL http://luajit.org/download/LuaJIT-2.1.0-beta3.tar.gz
			PREFIX ${CMAKE_CURRENT_BINARY_DIR}/luajit-2.1.0-beta3
			CONFIGURE_COMMAND ""
			BUILD_COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_BINARY_DIR}/luajit-2.1.0-beta3/src/luajit/src msvcbuild.bat static
			INSTALL_COMMAND ""
			BUILD_IN_SOURCE 1
		)
	else()
		ExternalProject_Add(luajit
			URL http://luajit.org/download/LuaJIT-2.1.0-beta3.tar.gz
			PREFIX ${CMAKE_CURRENT_BINARY_DIR}/luajit-2.1.0-beta3
			CONFIGURE_COMMAND ""
			BUILD_COMMAND make
			INSTALL_COMMAND make install PREFIX=${CMAKE_CURRENT_BINARY_DIR}/luajit-2.1.0-beta3
			BUILD_IN_SOURCE 1
		)
	endif()
	ExternalProject_Get_Property(luajit install_dir)

	target_include_directories(vecs PRIVATE ${install_dir}/src/luajit/src)
	#target_link_directories(vecs PRIVATE ${install_dir}/src/luajit/src)
	target_link_libraries(vecs PRIVATE ${install_dir}/src/luajit/src/lua51.lib)
else()
	target_link_libraries(vecs PRIVATE ${LUA_LIBRARIES})
	target_include_directories(vecs PRIVATE ${LUA_INCLUDE_DIR})
endif()

# sol2
if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/sol2)
	execute_process(COMMAND git clone https://github.com/ThePhD/sol2.git ${CMAKE_CURRENT_BINARY_DIR}/sol2)
endif()
target_include_directories(vecs PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/sol2/include)

# rectpack2D
if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/rectpack2D)
	execute_process(COMMAND git clone https://github.com/TeamHypersomnia/rectpack2D ${CMAKE_CURRENT_BINARY_DIR}/rectpack2D)
endif()
target_include_directories(vecs PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/rectpack2D/src)

# imgui
if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/imgui)
	execute_process(COMMAND git clone https://github.com/ocornut/imgui ${CMAKE_CURRENT_BINARY_DIR}/imgui)
endif()
target_include_directories(vecs PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/imgui)
target_sources(vecs PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/imgui/imgui.cpp ${CMAKE_CURRENT_BINARY_DIR}/imgui/imgui_demo.cpp ${CMAKE_CURRENT_BINARY_DIR}/imgui/imgui_draw.cpp ${CMAKE_CURRENT_BINARY_DIR}/imgui/imgui_widgets.cpp ${CMAKE_CURRENT_BINARY_DIR}/imgui/examples/imgui_impl_glfw.cpp ${CMAKE_CURRENT_BINARY_DIR}/imgui/misc/cpp/imgui_stdlib.cpp)

# tinyobj
if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/tinyobj)
	execute_process(COMMAND git clone https://github.com/tinyobjloader/tinyobjloader ${CMAKE_CURRENT_BINARY_DIR}/tinyobj)
endif()
target_include_directories(vecs PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/tinyobj)

#### Copy non-script files to our output directory ####

include("cmake/TargetCopyFiles.cmake")
add_custom_target(copyResources ALL)

# Debug files
# resources folder
add_copy_directory(copyResources default-resources DESTINATION build/Debug/resources)
# hastyNoised.dll
add_copy_files(copyResources ${HASTYNOISE_ROOT}/bin/hastyNoised.dll DESTINATION build/Debug)
# simd folder
add_copy_directory(copyResources ${HASTYNOISE_ROOT}/bin DESTINATION build/Debug/simd GLOB hastyNoise_*d.dll)

# Release files
# resources folder
add_copy_directory(copyResources default-resources DESTINATION build/Release/resources)
# hastyNoise.dll
add_copy_files(copyResources ${HASTYNOISE_ROOT}/bin/hastyNoise.dll DESTINATION build/Release)
# simd folder
add_copy_directory(copyResources ${HASTYNOISE_ROOT}/bin DESTINATION build/Release/simd GLOB hastyNoise_*[!d].dll)

#### Set Visual Studio specific properties ####

set_property(TARGET vecs PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BUILD_TYPE}")
