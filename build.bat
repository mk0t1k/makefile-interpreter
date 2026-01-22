@echo off
set CXXFLAGS=-std=c++23 -O2
set CXX=clang++

echo Compiling object files...
%CXX% %CXXFLAGS% -c main.cpp -o main.o
%CXX% %CXXFLAGS% -c cli.cpp -o cli.o
%CXX% %CXXFLAGS% -c makefile.cpp -o makefile.o
%CXX% %CXXFLAGS% -c rule.cpp -o rule.o
%CXX% %CXXFLAGS% -c argparser\argparser.cpp -o argparser\argparser.o
%CXX% %CXXFLAGS% -c argparser\argument.cpp -o argparser\argument.o

if errorlevel 1 (
    echo Compilation failed!
    exit /b 1
)

echo Linking...
%CXX% main.o cli.o makefile.o rule.o argparser\argparser.o argparser\argument.o -o make.exe

if errorlevel 1 (
    echo Linking failed!
    exit /b 1
)

echo Build completed successfully!
