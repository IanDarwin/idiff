CFLAGS = -O -g

all:	idiff

install:	idiff
			install idiff /usr/local/bin
			install idiff.1 /usr/local/man/man1
