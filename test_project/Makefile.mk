# Простой Makefile для тестирования

CC = g++

program.exe: main.o\
             functions.o
	$(CC) -o program.exe main.o functions.o

main.o: main.cpp functions.h
	$(CC) -c $< -o $@

functions.o: functions.cpp functions.h
	$(CC) -c $< -o $@

.PHONY: clean

clean:
	del *.o