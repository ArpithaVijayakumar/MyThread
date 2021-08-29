all:myProgram

myProgram: main.o libmythread.a
	gcc -o myProgram main.o -L. -lmythread

libmythread.a:mythread.o
	ar rcs libmythread.a mythread.o
	
mythread.o: mythread.h mythread.c
	gcc -w -c -o mythread.o mythread.c

main: main.c 
	gcc -o try main.o main.c
	
clean:
	rm -rf mythread.a mythread.o