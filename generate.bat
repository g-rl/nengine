@echo off
git submodule update --init --recursive

set /p COPYTO="Copy-to directory (leave blank to skip): "

if "%COPYTO%"=="" (
    tools\premake5 vs2022
) else (
    tools\premake5 vs2022 --copy-to="%COPYTO%"
)