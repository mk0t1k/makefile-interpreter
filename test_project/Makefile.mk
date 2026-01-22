# Простой Makefile для тестирования

program.exe: main.o functions.o
	g++ -o program.exe main.o functions.o

main.o: main.cpp functions.h
	g++ -c main.cpp -o main.o

functions.o: functions.cpp functions.h
	g++ -c functions.cpp -o functions.o

.PHONY: clean

clean:
	del *.o