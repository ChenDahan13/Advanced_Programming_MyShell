all: myshell

myshell: key_attempts.o myshell.o
	gcc -o myshell key_attempts.o myshell.o

key_attempts.o: key_attempts.c key_attempts.h
	gcc -c key_attempts.c

myshell.o: myshell.c myshell.h
	gcc -c myshell.c

clean: 
	rm -f *.o myshell