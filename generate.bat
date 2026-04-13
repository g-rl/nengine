@echo off
git submodule update --init --recursive

set /p COPYTO="MW2019 copy-to directory (leave blank to skip): "
set /p COPYTO_MWII="MWII copy-to directory (leave blank to skip): "

set ARGS=vs2022

if not "%COPYTO%"=="" (
    set ARGS=%ARGS% --copy-to="%COPYTO%"
)

if not "%COPYTO_MWII%"=="" (
    set ARGS=%ARGS% --copy-to-mw2="%COPYTO_MWII%"
)

tools\premake5 %ARGS%
