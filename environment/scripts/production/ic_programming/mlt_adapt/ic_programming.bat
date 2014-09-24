@echo off
rem SWIM/SWD-programming script for STM8/STM32-targets

set VERSION=mlt_adapt-ic_programming-01.01.0001
set PROG_TGT_DEV=STM32F100C8
set PROG_TGT_PROT=SWD


:START
cls
echo.
echo ********************************************************************************
echo Regent Beleuchtungskoerper AG - Switzerland
echo.
echo Firmware Uploader for Mylights Technology finder(TM)
echo (version: %VERSION%)
echo ********************************************************************************
echo.


set start=%time%
set failure=0

if not exist log mkdir log
set logFile=log\%date:~-4,4%%date:~-7,2%%date:~-10,2%_%time:~0,2%%time:~3,2%%time:~6,2%.log
echo Start programming (%date% %start%) >%logFile%
set redirection=1^>^>"%logFile%" 2^>^&1


echo.
echo --------------------------------------------------------------------------------
echo Dateien verifizieren...
call :FINDFILE "mlt_boot*.hex"
if not %RESULT%==0 goto FAILED
echo - %FILENAME%
set FILENAME_BOOT=%FILENAME%
call :FINDFILE "mlt_adapt*.hex"
if not %RESULT%==0 goto FAILED
echo - %FILENAME%
set FILENAME_ADAPT=%FILENAME%
call :VERIFYAPP "Cortex_pgm.exe"
if not %RESULT%==0 goto FAILED
echo - %APPDIR%
set APPDIR_CORTEX=%APPDIR%


set verifyEnd=%time%
call :TIMER %start% %verifyEnd%
echo --------------------------------------------------------------------------------
echo Dauer Verifikation: %DURATION% Sekunden
echo --------------------------------------------------------------------------------


echo.
echo --------------------------------------------------------------------------------
echo Programmierung...
echo - %PROG_TGT_DEV% programmieren...
echo "%APPDIR_CORTEX%" T%PROG_TGT_DEV% WC E P"%FILENAME_BOOT%" P"%FILENAME_ADAPT%" S %redirection%
"%APPDIR_CORTEX%" T%PROG_TGT_DEV% WC E P"%FILENAME_BOOT%" P"%FILENAME_ADAPT%" S %redirection%
if not %errorlevel%==0 set failure=1
set programmingEnd=%time%
call :TIMER %verifyEnd% %programmingEnd%
echo --------------------------------------------------------------------------------
echo Dauer Programmierung: %DURATION% Sekunden
echo --------------------------------------------------------------------------------
if %failure%==1 goto FAILED
goto DONE


:FINDFILE :: arg1
setlocal enabledelayedexpansion
set r=0
set FNAME=
for %%i in (%~1) do (
  if not "%%~i"=="" (
    if not "!FNAME!"=="" (
      goto :MULTIPLE_BINS
      set r=1
    )
    set FNAME=%%~i
  )
)
if "!FNAME!"=="" set r=1
endlocal&set FILENAME=%FNAME%&set RESULT=%r%
goto :EOF 


:VERIFYAPP :: arg1
setlocal enabledelayedexpansion
set r=0
set ADIR=
%~1 1> nul 2> nul
if !errorlevel!==9009 (
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
  set ADIR=%~1
)
if not "!ADIR!"=="" (
  for /f "tokens=* delims= " %%A in ('echo !ADIR! ') do set ADIR=%%A
  set ADIR=!ADIR:~0,-1!
)
endlocal&set APPDIR=%ADIR%&set RESULT=%r%
goto :EOF


:MULTIPLE_BINS
echo MULTIPLE_BINS
goto :EOF


:TIMER :: arg1 arg2
setlocal enabledelayedexpansion
set options="tokens=1-4 delims=:."
for /f %options% %%a in ("%~1") do set start_h=%%a&set /a start_m=100%%b %% 100&set /a start_s=100%%c %% 100&set /a start_ms=100%%d %% 100
for /f %options% %%a in ("%~2") do set end_h=%%a&set /a end_m=100%%b %% 100&set /a end_s=100%%c %% 100&set /a end_ms=100%%d %% 100
set /a hours=!end_h!-!start_h!
set /a mins=!end_m!-!start_m!
set /a secs=!end_s!-!start_s!
if !hours! lss 0 set /a hours = 24!hours!
if !mins! lss 0 set /a hours = !hours! - 1 & set /a mins = 60!mins!
if !secs! lss 0 set /a mins = !mins! - 1 & set /a secs = 60!secs!
:: mission accomplished
set /a totalsecs = !hours!*3600 + !mins!*60 + !secs!
endlocal&set DURATION=%totalsecs%
goto :EOF


:DONE
set RESULT=0
echo.
echo ********************************************************************************
echo ** SUCCESS
echo ********************************************************************************
echo.
@rem set input=
@rem set msg=start upload: ^[ENTER^] - quit: ^[Q + ENTER^]:
@rem set /P input="%msg% "
@rem if /I "%input%" == "" goto :START
goto END


:FAILED
set RESULT=1
echo.
echo ********************************************************************************
echo ** ERROR DURING PROGRAMMING
echo ********************************************************************************
echo.
@rem set input=
@rem set msg=next: ^[ENTER^] - quit: ^[N^]:
@rem set /P input="%msg% "
@rem if /I "%input%" == "" goto :START
goto END


:END
if exist config_before.keyr del config_before.keyr
if exist temp.txt del temp.txt
set end=%time%
call :TIMER %start% %end%
echo.
echo --------------------------------------------------------------------------------
echo Verarbeitungsdauer (total): %DURATION% Sekunden
echo --------------------------------------------------------------------------------
echo -------------------------------------------------------------------------------- >>%logFile%
echo  Error: %RESULT%   Verarbeitungsdauer: %DURATION% Sekunden >>%logFile%
echo -------------------------------------------------------------------------------- >>%logFile%
exit /B %RESULT%


