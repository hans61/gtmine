gtmine.gt1:
	glcc -o $@ gtmine.c -map=32k

clean: FORCE
	-rm *.gt1

FORCE:
