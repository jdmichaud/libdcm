CC = cc -Wall -Wextra -Wpedantic -Wfatal-errors
EXE = pixdump
SRC = pixdump.c

all:
	${CC} -O3 -I../libdcm/ -L../libdcm/ ${SRC} -o ${EXE} -ldcm

debug:
	${CC} -ggdb3 -I../libdcm/ -L../libdcm/ ${SRC} -o ${EXE} -ldcm

static:
	${CC} -O3 -I../libdcm/ -L../libdcm/ ${SRC} -o ${EXE} -static -ldcm

clean:
	rm -fr ${EXE} $(SRC:.c=.o)
