ROM=v5a
GLCC=glcc
D=./

all: ${D}gtmine32.gt1 ${D}gtmine64.gt1

${D}gtmine32.gt1: gtmine.c
	${GLCC} -o $@ gtmine.c -DMEM32=1 -map=32k,./gtmine32.ovl -rom=${ROM}

${D}gtmine64.gt1: gtmine.c
	${GLCC} -o $@ gtmine.c -DMEM32=0 -map=64k -rom=${ROM}

clean: FORCE
	-rm ${D}gtmine32.gt1 ${D}gtmine64.gt1

FORCE:
