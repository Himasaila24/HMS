@echo off
title Hospital Patient Management System - C++ Backend Server
cd /d "%~dp0"

taskkill /F /IM server.exe >nul 2>&1

if not exist "backend\server.exe" (
    echo [INFO] Compiling C++ backend server...
    g++ -std=c++17 -O2 backend/server.cpp -o backend/server.exe -lws2_32 -static -static-libgcc -static-libstdc++
)

echo [INFO] Starting Hospital Patient Management System C++ Server...
backend\server.exe
pause
