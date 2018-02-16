@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set current-path=%~dp0
rem // remove trailing slash
set current-path=%current-path:~0,-1%

set repo-root=%current-path%\..
rem // resolve to fully qualified path
for %%i in ("%repo-root%") do set repo-root=%%~fi

set cmake_folder=%repo-root%\cmake\analysis

echo Repo Root: %repo-root%
echo CMake Dir: %cmake_folder%

if EXIST %cmake_folder% (
    rmdir /s/q %cmake_folder%
    rem no error checking
)

mkdir %cmake_folder%
rem no error checking
pushd %cmake_folder%

cmake %repo-root% -Dskip_samples:BOOL=ON -Duse_prov_client:BOOL=ON -Ddont_use_uploadtoblob:BOOL=OFF
if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!

echo ***Building configurations***
msbuild /m azure_iot_c_analysis.sln /p:Configuration=Release
if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!

rem Run the analysis applications
%cmake_folder%\binary_info\Release\binary_info.exe -c %cmake_folder%

%cmake_folder%\memory\telemetry_memory\release\telemetry_memory.exe -c %IOTHUB_CONNECTION_STRING%
