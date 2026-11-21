@echo off
REM Abre o TamaPoke numa janela, com o mouse fazendo de dedo.
REM Duplo clique aqui e pronto. Nada precisa estar instalado no sistema:
REM o compilador e portatil e mora em Downloads\_toolchain.
setlocal
set PATH=C:\Users\foulu\Downloads\_toolchain\w64devkit\bin;%PATH%
cd /d "%~dp0"

echo Compilando (rapido se nada mudou)...
make win
if errorlevel 1 (
  echo.
  echo Falhou a compilacao. Veja as mensagens acima.
  pause
  exit /b 1
)

echo.
echo   mouse           = dedo (clicar, arrastar para deslizar)
echo   ESC             = sair
echo   ESPACO          = pausa
echo   1 a 9           = velocidade do relogio
echo   G               = salva uma captura
echo   S               = digita um comando de consola (SPEC 25, LVL 30, EGGS...)
echo.
build\tamapoke_win.exe --sd ..\tools\sdcard --zoom 1
endlocal
