default: all

all: gtmine.gt1

gtmine.gt1:
#	glcc -o $@ gtmine.c -map=32k,./gtmine.ovl
	glcc -o $@ gtmine.c -map=32k

clean: FORCE
	-rm *.gt1

FORCE:
