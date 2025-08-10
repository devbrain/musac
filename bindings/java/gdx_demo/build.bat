@echo off
setlocal enabledelayedexpansion

:: Musac libGDX Demo - Unified Build Script for Windows

:: Default values
set BUILD_DIR=build-cmake
set PLATFORMS=
set CMAKE_ARGS=
set BUILD_TYPE=Release
set VERBOSE=false
set CLEAN=false
set RUN_PLATFORM=

:: Parse arguments
:parse_args
if "%~1"=="" goto :check_platforms
if /i "%~1"=="-h" goto :usage
if /i "%~1"=="--help" goto :usage
if /i "%~1"=="-d" (
    set BUILD_TYPE=Debug
    shift
    goto :parse_args
)
if /i "%~1"=="--debug" (
    set BUILD_TYPE=Debug
    shift
    goto :parse_args
)
if /i "%~1"=="-c" (
    set CLEAN=true
    shift
    goto :parse_args
)
if /i "%~1"=="--clean" (
    set CLEAN=true
    shift
    goto :parse_args
)
if /i "%~1"=="-r" (
    set RUN_PLATFORM=%~2
    shift
    shift
    goto :parse_args
)
if /i "%~1"=="--run" (
    set RUN_PLATFORM=%~2
    shift
    shift
    goto :parse_args
)
if /i "%~1"=="-v" (
    set VERBOSE=true
    set CMAKE_ARGS=!CMAKE_ARGS! -DCMAKE_VERBOSE_MAKEFILE=ON
    shift
    goto :parse_args
)
if /i "%~1"=="--verbose" (
    set VERBOSE=true
    set CMAKE_ARGS=!CMAKE_ARGS! -DCMAKE_VERBOSE_MAKEFILE=ON
    shift
    goto :parse_args
)
if /i "%~1"=="desktop" (
    set PLATFORMS=!PLATFORMS! desktop
    shift
    goto :parse_args
)
if /i "%~1"=="android" (
    set PLATFORMS=!PLATFORMS! android
    shift
    goto :parse_args
)
if /i "%~1"=="web" (
    set PLATFORMS=!PLATFORMS! web
    shift
    goto :parse_args
)
if /i "%~1"=="all" (
    set PLATFORMS=desktop android web
    shift
    goto :parse_args
)
echo Unknown option: %~1
goto :usage

:check_platforms
if "%PLATFORMS%"=="" (
    echo No platforms specified
    goto :usage
)

:: Clean if requested
if "%CLEAN%"=="true" (
    echo Cleaning previous builds...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
    call gradlew.bat clean 2>nul
)

:: Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

:: Prepare CMake arguments
set CMAKE_CONFIG_ARGS=-DCMAKE_BUILD_TYPE=%BUILD_TYPE%

:: Configure platforms
for %%p in (%PLATFORMS%) do (
    if /i "%%p"=="desktop" (
        set CMAKE_CONFIG_ARGS=!CMAKE_CONFIG_ARGS! -DBUILD_DESKTOP=ON
    )
    if /i "%%p"=="android" (
        set CMAKE_CONFIG_ARGS=!CMAKE_CONFIG_ARGS! -DBUILD_ANDROID=ON
        :: Check Android SDK
        if "%ANDROID_SDK_ROOT%"=="" if "%ANDROID_HOME%"=="" (
            echo Error: ANDROID_SDK_ROOT or ANDROID_HOME must be set
            exit /b 1
        )
    )
    if /i "%%p"=="web" (
        set CMAKE_CONFIG_ARGS=!CMAKE_CONFIG_ARGS! -DBUILD_WEB=ON
        :: Check Emscripten
        if "%EMSDK%"=="" (
            echo Error: EMSDK must be set. Please install and source Emscripten SDK
            echo Visit: https://emscripten.org/docs/getting_started/downloads.html
            exit /b 1
        )
    )
)

:: Configure
echo Configuring build...
echo Platforms: %PLATFORMS%
echo Build type: %BUILD_TYPE%

if "%VERBOSE%"=="true" (
    cmake %CMAKE_CONFIG_ARGS% %CMAKE_ARGS% ..
) else (
    cmake %CMAKE_CONFIG_ARGS% %CMAKE_ARGS% .. >nul 2>&1
)

:: Build each platform
for %%p in (%PLATFORMS%) do (
    echo Building %%p...
    
    if /i "%%p"=="desktop" (
        if "%VERBOSE%"=="true" (
            cmake --build . --target desktop_dist
        ) else (
            cmake --build . --target desktop_dist >nul 2>&1
        )
        echo Desktop build complete
    )
    if /i "%%p"=="android" (
        if "%VERBOSE%"=="true" (
            cmake --build . --target android_apk
            cmake --build . --target android_bundle
        ) else (
            cmake --build . --target android_apk >nul 2>&1
            cmake --build . --target android_bundle >nul 2>&1
        )
        echo Android build complete
    )
    if /i "%%p"=="web" (
        if "%VERBOSE%"=="true" (
            cmake --build . --target web_dist
        ) else (
            cmake --build . --target web_dist >nul 2>&1
        )
        echo Web build complete
    )
)

:: Package all
echo Packaging builds...
if "%VERBOSE%"=="true" (
    cmake --build . --target package_all
) else (
    cmake --build . --target package_all >nul 2>&1
)

:: Show output location
echo.
echo Build complete!
echo Output files: %BUILD_DIR%\dist\
echo.
dir dist 2>nul

:: Run if requested
if not "%RUN_PLATFORM%"=="" (
    echo Running %RUN_PLATFORM%...
    if /i "%RUN_PLATFORM%"=="desktop" (
        cmake --build . --target run_desktop
    )
    if /i "%RUN_PLATFORM%"=="android" (
        cmake --build . --target run_android
    )
    if /i "%RUN_PLATFORM%"=="web" (
        echo Starting web server at http://localhost:8000
        cmake --build . --target serve_web
    )
)

goto :end

:usage
echo Usage: %~nx0 [OPTIONS] [PLATFORMS]
echo.
echo PLATFORMS:
echo   desktop    Build desktop version (Windows/Linux/Mac)
echo   android    Build Android version (APK and AAB)
echo   web        Build Web/HTML5 version
echo   all        Build all platforms
echo.
echo OPTIONS:
echo   -d, --debug       Build debug version
echo   -c, --clean       Clean before building
echo   -r, --run PLATFORM Run specified platform after build
echo   -v, --verbose     Verbose output
echo   -h, --help        Show this help
echo.
echo EXAMPLES:
echo   %~nx0 desktop                  # Build desktop only
echo   %~nx0 desktop android           # Build desktop and Android
echo   %~nx0 all                       # Build all platforms
echo   %~nx0 -c -r desktop desktop     # Clean, build and run desktop
echo   %~nx0 -d android                # Build Android debug version
exit /b 0

:end
endlocal