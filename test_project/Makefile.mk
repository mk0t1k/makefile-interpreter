# Простой Makefile для тестирования

program.exe: main.o\
             functions.o
	g++ -o program.exe main.o functions.o

main.o: main.cpp functions.h
	g++ -c $< -o $@

functions.o: functions.cpp functions.h
	g++ -c $< -o $@

.PHONY: clean

clean:
	del *.o