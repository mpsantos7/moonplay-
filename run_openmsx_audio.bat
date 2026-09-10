@echo off
setlocal
REM FS-A1ST (YM2413 interno) + cartucho MSX-AUDIO (Y8950) no slot.
REM Testar leads AUDIO: MOONPLAY -info GHOSTBUS
REM                         MOONPLAY -info FRAY
REM Samples .MBK ainda nao tocam.

set "DSK=%~dp0emul\dsk\DOS2_moonplay.dsk"
if not exist "%DSK%" (
  echo Disk not found: %DSK%
  echo Run build.bat first.
  exit /b 1
)

set "OMSX="
if exist "%MSXGL_PATH%\tools\openMSX\openmsx.exe" set "OMSX=%MSXGL_PATH%\tools\openMSX\openmsx.exe"
if not defined OMSX if exist "C:\msxgl\tools\openMSX\openmsx.exe" set "OMSX=C:\msxgl\tools\openMSX\openmsx.exe"
if not defined OMSX if exist "%ProgramFiles%\openMSX\openmsx.exe" set "OMSX=%ProgramFiles%\openMSX\openmsx.exe"
if not defined OMSX if exist "%ProgramFiles(x86)%\openMSX\openmsx.exe" set "OMSX=%ProgramFiles(x86)%\openMSX\openmsx.exe"
if not defined OMSX (
  where openmsx.exe >nul 2>&1 && set "OMSX=openmsx.exe"
)

if not defined OMSX (
  echo openMSX not found. Install it or set MSXGL_PATH.
  echo Then run:
  echo   openmsx -machine Panasonic_FS-A1ST -ext msxaudio -diska "%DSK%"
  exit /b 1
)

echo Using: %OMSX%
echo Disk:  %DSK%
echo Machine: Panasonic FS-A1ST + MSX-AUDIO (Y8950)
echo MOONPLAY -info GHOSTBUS
echo ESC stops the song.
"%OMSX%" -machine Panasonic_FS-A1ST -ext msxaudio -diska "%DSK%"
endlocal
