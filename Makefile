all:
	${MAKE} -C libdcm
	${MAKE} -C dcmr

debug:
	${MAKE} debug -C libdcm
	${MAKE} debug -C dcmr

static:
	${MAKE} -C libdcm
	${MAKE} static -C dcmr

clean:
	${MAKE} clean -C libdcm
	${MAKE} clean -C dcmr

re: clean all
