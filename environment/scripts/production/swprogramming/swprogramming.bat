@echo off

for /F "tokens=*" %%I in (config.ini) do set %%I

:START
cls
echo ***********************************
echo * Software Konfiguration einlesen *
echo ***********************************
echo.
set /p cfg=Software-Konfiguration (q zum Beenden): 
echo.
if "%cfg%"=="q" goto END
goto SETUP


:SETUP
set directory="%cfgDirectory%"
for /f "tokens=1* delims=." %%a in ("%cfg%") do (
    set directory=%directory%\%%a\%%b
)
set batch=%directory%\%cfg%.bat
if not exist %batch% echo "batch problem"
if not exist %directory% echo "dir problem"
goto VERIFY


:VERIFY
echo öberprÅfe Software-Konfiguration...
if not exist %directory% goto ERROR
echo öberprÅfe Programmier-Skript...
if not exist %batch% goto ERROR
goto PROGRAM


:PROGRAM
echo Starte Programmierung...
call %batch%
goto END

:ERROR
echo .
echo *********
echo * ERROR *
echo *********
echo.
pause
goto END

:END
echo Beenden...
echo.
echo.

