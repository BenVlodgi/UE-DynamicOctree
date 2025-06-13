@echo off
setlocal

REM ======= Config =======
set PluginName=DynamicOctree
set EngineVersions=5.0 5.1 5.2 5.3 5.4 5.5 5.6
set Platforms=Win64
REM ======================

REM Convert space-separated platforms into + separated for UAT
set "TargetPlatforms="
for %%p in (%Platforms%) do (
    if defined TargetPlatforms (
        set "TargetPlatforms=%TargetPlatforms%+%%p"
    ) else (
        set "TargetPlatforms=%%p"
    )
)

REM Extract VersionName from the .uplugin JSON file
for /f "delims=" %%v in ('powershell -nologo -command ^
    "$j = Get-Content %PluginName%.uplugin -Raw | ConvertFrom-Json; $j.VersionName"') do set "PluginVersionName=%%v"


echo Building %PluginName% v%PluginVersionName% for Engine Versions: %EngineVersions%.

md Build
md "%temp%\%PluginName%"

for %%a in (%EngineVersions%) do call :build %%a

rd /s /q "%temp%\%PluginName%"
goto :end

:build
echo.
echo ==== Building for UE %1 ====

REM Try to locate the engine install directory
set "UnrealEngineVersionPath="
for /f "tokens=2* skip=2" %%x in ('reg query "HKLM\SOFTWARE\EpicGames\Unreal Engine\%1" /v "InstalledDirectory" 2^>nul') do set "UnrealEngineVersionPath=%%y"
if not defined UnrealEngineVersionPath (
    echo Engine version %1 not found. Skipping.
    goto :Failure
)

REM Prepare temp folder
md "%temp%\%PluginName%\%1" >nul 2>nul
md "%temp%\%PluginName%\%1\Config" >nul 2>nul
copy "Config\FilterPlugin.ini" "%temp%\%PluginName%\%1\Config\FilterPlugin.ini" >nul

REM Build the plugin
echo Building...
call "%UnrealEngineVersionPath%\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin="%CD%\%PluginName%.uplugin" -Package="%temp%\%PluginName%\%1" -TargetPlatforms=%TargetPlatforms% -Rocket >"Build\BuildLog-%1.txt"

REM Clean up extra files
rd /s /q "%temp%\%PluginName%\%1\Intermediate"

REM Check if build succeeded
if not exist "%temp%\%PluginName%\%1\%PluginName%.uplugin" (
    echo Build failed or incomplete for UE %1.
    goto :Failure
)

REM Zip the result
echo Zipping...
tar -a -c -f "Build\%PluginName%-%PluginVersionName%-UE%1.zip" -C "%temp%\%PluginName%\%1" *
goto :Success

:Success
echo Build Success.
goto :eof

:Failure
echo Build Fail.
goto :eof

:end
pause
