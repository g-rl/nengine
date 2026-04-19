@echo off
git submodule update --init --recursive

set /p COPYTO="MW2019 copy-to directory (leave blank to skip): "
set /p COPYTO_VG="VG copy-to directory (leave blank to skip): "
set /p COPYTO_MWII="MWII copy-to directory (leave blank to skip): "

set ARGS=vs2022

if not "%COPYTO%"=="" (
    set ARGS=%ARGS% --copy-to="%COPYTO%"
)

if not "%COPYTO_MWII%"=="" (
    set ARGS=%ARGS% --copy-to-mw2="%COPYTO_MWII%"
)

if not "%COPYTO_VG%"=="" (
    set ARGS=%ARGS% --copy-to-vg="%COPYTO_VG%"
)

tools\premake5 %ARGS%
