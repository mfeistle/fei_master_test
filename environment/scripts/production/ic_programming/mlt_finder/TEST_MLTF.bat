@echo off

@rem IMPORTANT: copy this script MUST be started out of the teraterm programm directory

@rem usage: ttpmacro.exe "TEST_MLTF.bat" "serial port"

@rem        serial port = modbus adapter, e.g. COM12 or 12

taskkill /F /FI "IMAGENAME eq ttpmacro*" /T 2>nul 1>nul
taskkill /F /FI "IMAGENAME eq ttermpro*" /T 2>nul 1>nul

ttpmacro.exe /V "TEST_MLTF.ttl" "%~1"
for /L %%e in (0 1 254) do if errorlevel %%e set ret=%%e
echo ret=%ret%& pause

taskkill /F /FI "IMAGENAME eq ttpmacro*" /T 2>nul 1>nul
taskkill /F /FI "IMAGENAME eq ttermpro*" /T 2>nul 1>nul

exit %ret%
