@echo off

set SOLUTION_DIR=%Temp%\wp
cmake -G "Ninja" -S . %SOLUTION_DIR% || goto :error
cmake --build %SOLUTION_DIR% || goto :error
copy %SOLUTION_DIR%\compile_commands.json . || goto :error
copy shaders\* ready || goto :error

echo SUCCESS
goto :finish 

:error
echo FAIL

:finish
