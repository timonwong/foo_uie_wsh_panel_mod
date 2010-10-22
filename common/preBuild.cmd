@echo off

:: Usage: preBuild.cmd ProjectSrcDir

SETLOCAL ENABLEEXTENSIONS

hg id -i > nul
if %ERRORLEVEL% NEQ 0 (
  copy /Y "%mydir%\hgrev_unknown.h" "%CD%\hgrev.h" > nul
) else (
  hg id -i | findstr "+" > nul
  if %ERRORLEVEL% EQU 0 (
    echo #define HG_MODS 1 > %CD%\hgrev.h
  ) else (
    echo #define HG_MODS 1 > %CD%\hgrev.h
  )
  echo. >> %CD%\hgrev.h
)

ENDLOCAL
:: Always return an errorlevel of 0
exit 0
