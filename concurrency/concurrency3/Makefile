all: con3

con3: con3.o
	gcc con3.o -o run_con -pthread

con3.o:
	gcc -c con3.c -g

clean:
	rm -rf *.o run_con
