CC = cc -Wall -Wextra -Wpedantic -Wfatal-errors
LIB = libdcm.a
SRC = data-dictionary.c dcm.c

all:
	${CC} -O3 -c ${SRC}
	ar rc ${LIB} $(SRC:.c=.o)
	ranlib ${LIB}

debug:
	${CC} -ggdb3 -c ${SRC}
	ar rc ${LIB} $(SRC:.c=.o)
	ranlib ${LIB}

clean:
	rm -fr ${LIB} $(SRC:.c=.o)
