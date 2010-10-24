@echo off

:: Usage: preBuild.cmd ProjectSrcDir

SETLOCAL ENABLEEXTENSIONS

SET mydir=%~dp0
SET projdir=%~dp1

hg id -i > nul
IF %ERRORLEVEL% NEQ 0 (
  copy /Y "%mydir%\hgrev_unknown.h" "%projdir%\hgrev.h" > nul
) ELSE (
  hg id -i | find "+" > nul
  IF !ERRORLEVEL! EQU 0 (
    echo #define HG_MODS 1 > %projdir%\hgrev.h
  ) ELSE (
    echo #define HG_MODS 0 > %projdir%\hgrev.h
  )
  echo. >> %CD%\hgrev.h
)

ENDLOCAL
:: Always return an errorlevel of 0
exit 0
