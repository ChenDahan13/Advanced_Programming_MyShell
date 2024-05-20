all: myshell

myshell: shell2.c
	gcc shell2.c -o myshell

clean:
	rm -f myshell 


