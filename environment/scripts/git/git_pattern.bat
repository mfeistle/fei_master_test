@echo off

REM Get commit hash
FOR /F "tokens=*" %%i in ('git rev-parse --verify HEAD') do set HASH=%%i
echo GIT hash    = %HASH%
set PATTERN=%HASH:~0,8%

REM Verify for modifications
set CHANGES=
FOR /F "tokens=*" %%i in ('git diff-index --name-only HEAD --') do set CHANGES=%%i
IF NOT "%CHANGES%" == "" (set PATTERN=%PATTERN%_m)
echo PATTERN     = %PATTERN%

REM Write into temporary header file
echo #define GIT_PATTERN "%PATTERN%" > version.h

IF NOT "%OFFICIAL_RELEASE%"=="" (
  echo #define OFFICIAL_RELEASE 1 >> version.h
)