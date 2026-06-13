@echo off
setlocal
title Smart Tourism System

cd /d "%~dp0"

set "BACKEND_EXE=smart-tourism-backend\cmake-build-debug\bin\SmartTourismSystem.exe"
set "FRONTEND_DIR=smart-tourism-frontend"
set "BACKEND_URL=http://localhost:8080/api/health"
set "FRONTEND_URL=http://localhost:3000/#/overview"
set "BACKEND_STARTED=0"

echo.
echo ========================================
echo   Smart Tourism System - One Click
echo   Backend : http://localhost:8080
echo   Frontend: %FRONTEND_URL%
echo   Ctrl+C to stop the frontend
echo ========================================
echo.

if not exist "%BACKEND_EXE%" (
  echo [ERROR] Backend executable not found:
  echo         %BACKEND_EXE%
  echo         Build the backend first.
  pause
  exit /b 1
)

if not exist "%FRONTEND_DIR%\node_modules\.bin\vite.cmd" (
  echo [ERROR] Frontend dependencies are missing.
  echo         Run: cd %FRONTEND_DIR% ^&^& npm install
  pause
  exit /b 1
)

REM Reuse an already-running backend when possible.
curl.exe --silent --fail --max-time 1 "%BACKEND_URL%" >nul 2>&1
if %errorlevel% equ 0 goto backend_running

echo [1/2] Starting C++ backend ...
start "" /B "%BACKEND_EXE%" --port 8080
set "BACKEND_STARTED=1"

REM curl.exe is much faster than starting PowerShell on every retry.
set /a n=0
:wait_backend
curl.exe --silent --fail --max-time 1 "%BACKEND_URL%" >nul 2>&1
if %errorlevel% equ 0 goto backend_ready
set /a n+=1
if %n% geq 10 goto backend_failed
timeout /t 1 /nobreak >nul
goto wait_backend

:backend_running
echo [1/2] Backend is already running.
goto start_frontend

:backend_ready
echo       [OK] Backend ready.

:start_frontend

echo [2/2] Starting Vue frontend ...

pushd "%FRONTEND_DIR%"
call npm.cmd run dev -- --host 0.0.0.0 --port 3000 --strictPort --open /#/overview
popd

echo.
if "%BACKEND_STARTED%"=="1" (
  echo Stopping backend ...
  taskkill /F /IM SmartTourismSystem.exe >nul 2>&1
)
pause
exit /b 0

:backend_failed
echo       [ERROR] Backend did not become ready within 10 seconds.
if "%BACKEND_STARTED%"=="1" taskkill /F /IM SmartTourismSystem.exe >nul 2>&1
pause
exit /b 1
