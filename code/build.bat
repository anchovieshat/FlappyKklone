@echo off

pushd ..\build

SET LIB=%LIB%;"..\libs\glfw\lib-vc2013";"..\libs\glew\lib\Release\Win32"

cl -nologo -Zi "..\code\source.cpp" -DGLEW_STATIC ..\code\glew.c glfw3dll.lib opengl32.lib winmm.lib

popd