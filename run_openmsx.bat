@echo off
setlocal
REM Launch the DOS2 disk with YM2413 (FM-PAC). A plain MSX2 without
REM this cartridge is silent: ports 7Ch/7Dh do not exist.

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
  echo   openmsx -exta slotexpander -ext msxdos2 -ext fmpac -ext ram4mb -diska "%DSK%"
  exit /b 1
)

echo Using: %OMSX%
echo Disk:  %DSK%
echo Machine: Panasonic FS-A1ST (internal YM2413, built-in DOS2)
echo MOONPLAY ficheiro.MBM   [-info] [-loop]
echo ESC stops the song.
"%OMSX%" -machine Panasonic_FS-A1ST -diska "%DSK%"
endlocal
