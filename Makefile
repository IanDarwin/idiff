all:	idiff

idiff:	idiff.c
	$(CC) idiff.c -o idiff

clean:
	rm -f *.o a.out
