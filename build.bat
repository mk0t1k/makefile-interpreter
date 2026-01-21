@echo off
set CXXFLAGS=-std=c++23 -O2
set SOURCES=main.cpp cli.cpp makefile.cpp rule.cpp argparser\argparser.cpp argparser\argument.cpp
clang++ %CXXFLAGS% %SOURCES% -o make.exe
