default: all

all: gtmine32.gt1 gtmine64.gt1

gtmine32.gt1: gtmine.c
	glcc -o $@ gtmine.c -DMEM32=1 -map=32k,./gtmine32.ovl

gtmine64.gt1: gtmine.c
	glcc -o $@ gtmine.c -DMEM32=0 -map=64k

clean: FORCE
	-rm *.gt1

FORCE:
