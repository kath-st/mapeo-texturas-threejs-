@echo off
echo =======================================================
echo Compilando Laboratorio de Mapeo de Texturas 3D (C++)
echo =======================================================
g++ main.cpp -o laboratorio_texturas.exe -lopengl32 -lglu32 -lfreeglut -lcomdlg32
if %ERRORLEVEL% EQU 0 (
    echo.
    echo [OK] Compilacion exitosa. Ejecutable: laboratorio_texturas.exe
    echo [INFO] Iniciando aplicacion...
    echo =======================================================
    laboratorio_texturas.exe
) else (
    echo.
    echo [ERROR] Error al compilar el codigo fuente.
    echo =======================================================
)
pause
