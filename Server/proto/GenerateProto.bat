@echo off
setlocal enabledelayedexpansion

:: ─────────────────────────────────────────────
::  GenerateProto.bat — Compile tous les .fbs
::  et copie automatiquement les fichiers C#
::  vers le projet Unity.
:: ─────────────────────────────────────────────

set SCHEMA_DIR=schemas
set OUT_DIR=generated
set UNITY_CS_DIR=..\..\MMO_MobileGame\Assets\Scripts\Network\Generated

:: Creer les dossiers si necessaire
if not exist %OUT_DIR% mkdir %OUT_DIR%
if not exist "%UNITY_CS_DIR%" mkdir "%UNITY_CS_DIR%"

echo.
echo ========================================
echo   FlatBuffers — Generation des schemas
echo ========================================
echo.

:: ─── Recherche de flatc.exe ───
set "FLATC_CMD="
for /R "%LOCALAPPDATA%\.xmake\packages\f\flatbuffers" %%F in (flatc.exe) do (
    if exist "%%F" (
        set "FLATC_CMD=%%F"
        goto :Found
    )
)

echo [ERREUR] Impossible de trouver flatc.exe.
echo Avez-vous bien compile le serveur avec xmake ?
echo.
pause
exit /b 1

:Found
echo [OK] flatc : !FLATC_CMD!
echo.

:: ─── Compilation de chaque .fbs ───
set COUNT=0
for %%i in (%SCHEMA_DIR%\*.fbs) do (
    echo   Compiling %%~nxi ...
    "!FLATC_CMD!" --cpp --csharp -I %SCHEMA_DIR% -o %OUT_DIR% "%%i"
    IF !ERRORLEVEL! NEQ 0 (
        echo.
        echo [ERREUR] La compilation de %%~nxi a echoue.
        pause
        exit /b !ERRORLEVEL!
    )
    set /a COUNT+=1
)

echo.
echo [OK] !COUNT! schema(s) compile(s) dans %OUT_DIR%\
echo.

:: ─── Copie automatique des C# vers Unity ───
echo ── Copie des fichiers C# vers Unity ──
echo.

set CS_COUNT=0
for /R "%OUT_DIR%" %%F in (*.cs) do (
    copy /Y "%%F" "%UNITY_CS_DIR%\" >nul 2>&1
    echo   Copie: %%~nxF
    set /a CS_COUNT+=1
)

echo.
echo ========================================
echo   Resultat
echo ========================================
echo.
echo   Schemas    : %SCHEMA_DIR%\*.fbs
echo   C++ (.h)   : %OUT_DIR%\*_generated.h
echo   C# (.cs)   : %UNITY_CS_DIR%\*.cs
echo   Fichiers   : !CS_COUNT! fichier(s) C# copies
echo.
pause
