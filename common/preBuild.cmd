@echo off

:: Usage: preBuild.cmd ProjectSrcDir

SETLOCAL ENABLEEXTENSIONS

SET mydir=%~dp0
SET projdir=%~dp1

hg id -i > nul
IF %ERRORLEVEL% EQU 1 GOTO MODS1

hg id -i | find "+" > nul
IF %ERRORLEVEL% EQU 1 (
	GOTO MODS0
) ELSE (
	GOTO MODS1
)

:MODS0
echo #define HG_MODS 0 > %projdir%\hgrev.h
GOTO :END

:MODS1
echo #define HG_MODS 1 > %projdir%\hgrev.h
GOTO :END

:END
echo. >> %projdir%\hgrev.h

ENDLOCAL
:: Always return an errorlevel of 0
exit 0
