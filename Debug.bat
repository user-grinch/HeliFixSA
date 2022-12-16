@echo off
premake5.exe vs2022
cd build
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
MsBuild HeliFix.sln /property:Configuration=Debug
cd ..
