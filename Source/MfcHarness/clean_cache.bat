@echo off
echo Cleaning Visual Studio and Build cache...

REM 递归删除 .pch 和 .ipch 文件
echo Deleting PCH/IPCH files...
del /s /q /f "*.pch"
del /s /q /f "*.ipch"

REM 递归删除 .vs 缓存文件夹 (Visual Studio 会自动重建)
echo Deleting .vs directory...
IF EXIST ".vs" (
    rmdir /s /q ".vs"
)



echo Cleanup complete.
pause