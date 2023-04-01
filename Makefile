ROM=v5a

default: all

all: gtmine32.gt1 gtmine64.gt1

gtmine32.gt1: gtmine.c
	glcc -o $@ gtmine.c -DMEM32=1 -map=32k,./gtmine32.ovl -rom=${ROM}

gtmine64.gt1: gtmine.c
	glcc -o $@ gtmine.c -DMEM32=0 -map=64k -rom=${ROM}

clean: FORCE
	-rm *.gt1

FORCE:
