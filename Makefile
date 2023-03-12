default: gtmine.gt1

all: gtmine.gt1 gtmine64.gt1

gtmine.gt1:
	glcc -o $@ gtmine.c -map=32k,./gtmine.ovl
#	glcc -o $@ gtmine.c -map=32k

gtmine64.gt1:
	glcc -o $@ gtmine.c -map=64k

clean: FORCE
	-rm *.gt1

FORCE:
