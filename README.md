# Makefile-interpritator

A small Makefile interpreter (in the spirit of `make`) written in C++.

## Quick start

I write it mostly for `Windows` because there are no tool for run makefiles. So there are windows .bat file for building - [build.bat](./build.bat)

If you want to use it on unix-like system, you can use `build.sh` (create it next to `build.bat`) with contents like a following:

```sh
#!/usr/bin/env sh
set -eu

CXXFLAGS="-std=c++23 -O2"
CXX="${CXX:-clang++}"

echo "Cleaning old object files..."
rm -f ./*.o
rm -f ./argparser/*.o

echo "Compiling object files..."
$CXX $CXXFLAGS -c main.cpp -o main.o
$CXX $CXXFLAGS -c cli.cpp -o cli.o
$CXX $CXXFLAGS -c makefile.cpp -o makefile.o
$CXX $CXXFLAGS -c rule.cpp -o rule.o
$CXX $CXXFLAGS -c argparser/argparser.cpp -o argparser/argparser.o
$CXX $CXXFLAGS -c argparser/argument.cpp -o argparser/argument.o

echo "Linking..."
$CXX main.o cli.o makefile.o rule.o argparser/argparser.o argparser/argument.o -o make

echo "Build completed successfully!"

echo "Cleaning object files after build..."
rm -f ./*.o
rm -f ./argparser/*.o
```

## Usage

By default, it searches for a Makefile in the current directory (in this order):
`GNUmakefile`, `makefile`, `Makefile`.

Run format:

```sh
./make [OPTIONS] [target...]
```

### Testing

There a dir [test_project](./test_project) with a simple c++ project for testing this interpretator, to run it you should use command:

```sh
./make.exe -C test_project -f Makefile.mk program.exe clean
```

### Available options
Now this options are available, in the future I'll extend this list

- **`-f, --file <path>`**: path to Makefile.
- **`-C, --directory <dir>`**: change to directory before doing anything.
- **`-n, --dry-run`**: do not run recipes; only print them.
- **`-s, --silent`**: do not echo recipes (except when `--dry-run` is enabled).
- **`-k, --keep-going`**: keep going and build other targets even if some targets cannot be built.
- **`-i, --ignore-errors`**: ignore recipe errors (continue executing the remaining commands in the recipe).
- **`-B, --always-make`**: unconditionally consider targets out-of-date.
- **`-q, --question`**: run no recipes; exit status is 0 if up-to-date, 1 if rebuild is needed.
- **`-h, --help`**: shows you a list of available options and their description.
- **`-v, --version`:** shows you a version of an aplication

<div align="center">
⭐ If you find this tool useful, please consider giving it a star on GitHub!
Happy Building! 🎉
</div>

---

<div align="center">
  
**By Michael Kotkin**

</div>

---