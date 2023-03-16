default: all

all: gtmine32.gt1 gtmine64.gt1

gtmine32.gt1:
	glcc -o $@ gtmine32.c -map=32k,./gtmine32.ovl

gtmine64.gt1:
	glcc -o $@ gtmine64.c -map=64k

clean: FORCE
	-rm *.gt1

FORCE:
