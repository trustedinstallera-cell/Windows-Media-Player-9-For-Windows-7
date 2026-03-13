@echo off
setlocal enabledelayedexpansion
if exist ".\GetCurrentSID.exe" (
"GetCurrentSID.exe" > nul
set /p SID=<wmp_sid.txt
echo 当前用户 SID: !SID!
)
pause