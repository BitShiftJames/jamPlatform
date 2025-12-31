## Purpose

This library if you could call it that, has five main purposes.
That it will seek to provide all while maintaining a declarative approach 
on multiple, if not all major operatoring systems.

1. Provides a way to ask for a graphics context I.E opengl, vulkan, direct3D

2. Provides audio buffer, without gurrentee of how it is written too.

3. Provides a list of events such that is easier to parse input.

4. Provides a clean way to open a window without any regards to how
   it will be drawn too decided by the graphics API of the users
   choosing.

5. Handle very elementary filesystem task like checking if a directory exist
   creating a file and loading a file in per byte form.

## Building

This project includes a CMake build file, which can generate either a Make file
or an Msbuild file depending on your platform or generator.

__Library build__[^1]

- git clone repo-here

- mkdir build && cd build

- cmake ..

- make --makefile=Makefile OR msbuild jamPlatform.sln

and then link against the output.

__To build the example.__

- cd jamPlatform/src

- mkdir build && cd build

- cmake ..

- make --makefile=Makefile OR msbuild jamPlatform.sln
    
[^1]: Currently only static library builds are supported.
