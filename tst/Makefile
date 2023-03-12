default: all

all: console-tst.gt1

console-tst.gt1:
	glcc -o $@ console-tst.c -map=32k

clean: FORCE
	-rm *.gt1

FORCE:
