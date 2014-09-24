@echo off
setlocal ENABLEDELAYEDEXPANSION

set RESULT=0

:START
cls
echo *********************
echo * Release erstellen *
echo *********************
echo.
goto SETUP


:OLD
set directory="%cfgDirectory%"
for /f "tokens=1* delims=." %%a in ("%cfg%") do (
  set directory=%directory%\%%a\%%b
)
set batch=%directory%\%cfg%.bat
if not exist %batch% echo "batch problem"
if not exist %directory% echo "dir problem"
goto VERIFY


:SETUP
set /a nrarguments=0
for %%A in (%*) do (
  set /a nrarguments=nrarguments+1
)
if not 2==!nrarguments! (
  echo %0 [library] [project]
  goto END
)
set library=%1
set project=%2
set target=release
set directory=..\..\..\software\!library!\projects\!project!
set cwd=%CD%
set logfile=!cwd!\release.log

echo - šberprfe Verzeichnisse und Dateien...
echo   LIBRARY:   !library!
echo   PROJECT:   !project!
echo   TARGET:    !target!
echo   DIRECTORY: !directory!
echo   CWD:       !cwd!
if not exist !directory! goto ERROR

call :VERIFYAPP "uv4.exe"
if not !RESULT!==0 goto ERROR
echo | set /p=Timestamp: %DATE% >!logfile!
echo %TIME% >>!logfile!
FOR /F "tokens=*" %%i in ('git rev-parse --verify HEAD') do set HASH=%%i
echo GIT Hash: %HASH% >>!logfile!
echo | set /p=uVision Version= >>!logfile!
..\..\tools\show_verrsc.exe !APPDIR! >>!logfile!
echo   uVISION:   !APPDIR!

goto RELEASE


:RELEASE
echo.
echo - Erstelle Release
set OFFICIAL_RELEASE=1
cd !directory!
call setup.bat >NUL
echo. >>!logfile!
echo !APPDIR! -r !project!.uvproj -t"!target!" -j0 >>!logfile!
!APPDIR! -r !project!.uvproj -t"!target!" -j0 -o"!logfile!.uvision"
type !logfile!.uvision >>!logfile!
if exist !logfile!.uvision del !logfile!.uvision
IF NOT !errorlevel!==0 (
  echo   RELEASE:   * fehlerhaft *
  GOTO ERROR
)
echo   RELEASE:   * erstellt *
goto END


:ERROR
echo.
echo *********
echo * ERROR *
echo *********
echo.
set RESULT=1
goto END


:END
cd !cwd!
if exist !logfile! copy !logfile! !directory!\!target!\!project!_!target!.log
echo.
IF !RESULT!==0 (
  echo ***********
  echo * SUCCESS *
  echo ***********
)
echo.
echo.
exit /b !RESULT!


:VERIFYAPP :: arg1
setlocal enabledelayedexpansion
set r=0
set ADIR=
where %~1 1> temp.txt 2> nul
if not !ERRORLEVEL!==0 (
  for /f "delims=" %%A in ('dir /b /s c:\%~1') do (
    if "!ADIR!"=="" set ADIR=%%A
  )
  if not "!ADIR!"=="" (
    for %%A in ("!ADIR!") do (
      Set Folder=%%~dpA
      Set Name=%%~nxA
    )
  )
) else (
  set /p ADIR= < temp.txt
)
if not "!ADIR!"=="" (
  for /f "tokens=* delims= " %%A in ('echo !ADIR! ') do set ADIR=%%A
  set ADIR=!ADIR:~0,-1!
)
if exist temp.txt del temp.txt
endlocal&set APPDIR=%ADIR%&set RESULT=%r%
goto :EOF


