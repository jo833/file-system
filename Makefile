CC     := gcc
CFLAGS := -Wall -Werror 

SRCS   := server.c

OBJS   := ${SRCS:c=o}
PROGS  := ${SRCS:.c=}

.PHONY: all
all: ${PROGS}

${PROGS} : % : %.o Makefile
	${CC} $< -o $@ udp.c
	${CC} -fPIC -g -c -Wall mkfs.c
	${CC} -fPIC -g -c -Wall mfs.c
	${CC} -fPIC -g -c -Wall udp.c
	${CC} -shared -Wl,-soname,libmfs.so.1 \-o libmfs.so mkfs.o mfs.o udp.o -lc
	${CC} ${CFLAGS} mkfs.c -o mkfs


clean:
	rm -f ${PROGS} ${OBJS}

%.o: %.c Makefile
	${CC} ${CFLAGS} -c $<