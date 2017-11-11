EXE = dcmr
SRC = data-dictionary.c dcm.c main.c

all:
	cc -Wall -Wextra -Wpedantic -Wfatal-errors ${SRC} -o ${EXE}
debug:
	cc -Wall -Wextra -Wpedantic -Wfatal-errors -ggdb3 ${SRC} -o ${EXE}
static:
	cc -Wall -Wextra -Wpedantic -Wfatal-errors -O3 ${SRC} -o ${EXE} -static
clean:
	rm -fr ${EXE}
