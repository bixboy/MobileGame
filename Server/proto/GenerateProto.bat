@echo off
setlocal enabledelayedexpansion

:: Dossier o se trouvent les fichiers gnrs
set OUT_DIR=generated

:: Crer le dossier s'il n'existe pas
if not exist %OUT_DIR% mkdir %OUT_DIR%

echo Recherche du compilateur flatc.exe dans les packages xmake...

:: Parcourir le cache de xmake pour trouver flatc.exe
set "FLATC_CMD="
for /R "%LOCALAPPDATA%\.xmake\packages\f\flatbuffers" %%F in (flatc.exe) do (
    if exist "%%F" (
        set "FLATC_CMD=%%F"
        goto :Found
    )
)

:NotFound
echo.
echo [ERREUR] Impossible de trouver flatc.exe.
echo Avez-vous bien compile le serveur une premiere fois avec xmake ?
pause
exit /b 1

:Found
echo Utilisation de: !FLATC_CMD!
echo Compilation des schemas FlatBuffers...

for %%i in (*.fbs) do (
    echo Compilation de %%i...
    "!FLATC_CMD!" --cpp --csharp -o %OUT_DIR% "%%i"
    IF !ERRORLEVEL! NEQ 0 (
        echo.
        echo [ERREUR] La compilation de %%i a echoue.
        pause
        exit /b !ERRORLEVEL!
    )
)

echo.
echo [SUCCES] Fichiers generes dans le dossier : %OUT_DIR%
echo - Pour le C++ (Serveur) : %OUT_DIR%\..._generated.h
echo - Pour le C# (Unity)   : %OUT_DIR%\.../*.cs
echo.
pause
