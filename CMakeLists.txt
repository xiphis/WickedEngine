cmake_minimum_required(VERSION 3.8)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

option(WICKED_DYNAMIC_LIBRARY "Build WickedEngine as a dynamic library" OFF)

option(WICKED_EDITOR "Build WickedEngine editor" ON)
option(WICKED_TESTS "Build WickedEngine tests" ON)
option(WICKED_IMGUI_EXAMPLE "Build WickedEngine imgui example" ON)
option(WICKED_LINUX_TEMPLATE "Build WickedEngine Linux template" ON)
option(WICKED_WINDOWS_TEMPLATE "Build WickedEngine Windows template" ON)

# Configure CMake global variables
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use solution folders to organize projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(WickedEngine)

if (WIN32)
    set(PLATFORM "Windows")
    add_compile_definitions(WIN32=1)
    # add_compile_definitions(_WIN32=1) this is a given from the compiler
    set(DXC_TARGET "${CMAKE_CURRENT_SOURCE_DIR}/WickedEngine/dxc.exe")
elseif(UNIX)
    set(PLATFORM "SDL2")
    add_compile_definitions(SDL2=1)
    set(DXC_TARGET "dxc")
endif()

add_subdirectory(WickedEngine)

#add_subdirectory(gamerpc)

if (WICKED_EDITOR)
    add_subdirectory(Editor)
endif()

if (WICKED_TESTS)
    add_subdirectory(Tests)
endif()

if (WICKED_IMGUI_EXAMPLE)
    add_subdirectory(Example_ImGui)
    add_subdirectory(Example_ImGui_Docking)
endif()

if (WIN32)
    if (WICKED_WINDOWS_TEMPLATE)
        add_subdirectory(Template_Windows)
    endif()
else()
    if (WICKED_LINUX_TEMPLATE)
        add_subdirectory(Template_Linux)
    endif()
endif()
