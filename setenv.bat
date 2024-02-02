@echo off

if "%OLDPATH%"=="" set OLDPATH=%PATH%

if "%QT32PATH%"=="" set QT32PATH=C:\Qt\5.15.2\msvc2019_86\bin
if "%QT64PATH%"=="" set QT64PATH=C:\Qt\5.15.2\msvc2019_64\bin
if "%QTCREATORPATH%"=="" set QTCREATORPATH=c:\Qt\qtcreator-4.3.1\bin
if "%VSVARSALLPATH%"=="" set VSVARSALLPATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat
if "%COVERITYPATH%"=="" set COVERITYPATH=c:\coverity\bin

if "%1"=="x32" (
    goto x32
) else if "%1"=="x64" (
    goto x64
) else if "%1"=="coverity" (
    goto coverity
) else (
    echo "Usage: setenv x32/x64/coverity"
    goto :eof
)

:x32
echo Setting Qt in PATH
set PATH=%PATH%;%QT32PATH%
set PATH=%PATH%;%QTCREATORPATH%
echo Setting VS in PATH
call "%VSVARSALLPATH%"
goto :eof

:x64
echo Setting Qt in PATH
set PATH=%PATH%;%QT64PATH%
set PATH=%PATH%;%QTCREATORPATH%
echo Setting VS in PATH
call "%VSVARSALLPATH%" amd64
goto :eof

:coverity
echo Setting Coverity in PATH
set PATH=%PATH%;%COVERITYPATH%
goto :eof