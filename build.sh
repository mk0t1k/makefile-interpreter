#!/bin/bash

CXXFLAGS="-std=c++23 -O2"
CXX="clang++"

echo Cleaning old object files...
rm -f *.o
rm -f argparser/*.o

echo Compiling object files...
$CXX $CXXFLAGS -c main.cpp -o main.o
$CXX $CXXFLAGS -c cli.cpp -o cli.o
$CXX $CXXFLAGS -c makefile.cpp -o makefile.o
$CXX $CXXFLAGS -c parser.cpp -o parser.o
$CXX $CXXFLAGS -c rule.cpp -o rule.o
$CXX $CXXFLAGS -c argparser/argparser.cpp -o argparser/argparser.o
$CXX $CXXFLAGS -c argparser/argument.cpp -o argparser/argument.o

if [ $? -ne 0 ]; then
    echo Compilation failed!
    exit 1
fi

echo Linking...
$CXX main.o cli.o makefile.o parser.o rule.o argparser/argparser.o argparser/argument.o -o make

if [ $? -ne 0 ]; then
    echo Linking failed!
    exit 1
fi

echo Build completed successfully!

echo Cleaning object files after build...
rm -f *.o
rm -f argparser/*.o
