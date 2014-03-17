OBJS = himenobmtxps.o himenobmtxps_io.o himenobmtxps_err.o

CC = mpicc
RM = /bin/rm
PROG = bmt

CFLAGS = -O3
LDFLAGS = ${CFLAGS}

.SUFFIXES: .c .o

all : ${PROG}

${PROG} : ${OBJS} param.h
	${CC} -o $@ ${OBJS} ${LDFLAGS}

.c.o : param.h
	${CC} -c ${CFLAGS} $<

.PHONY: clean
clean :
	${RM} -f ${PROG} ${OBJS} core






