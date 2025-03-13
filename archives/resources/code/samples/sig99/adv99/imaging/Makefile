# overrride with: make "CFLAGS = <whatever>" <target>
SHELL = /bin/sh
CFLAGS = -g -I../util -I/usr/include/GL -fullwarn
LIBS = -lglut -lGLU -lGL -lXmu -lXt -lX11 -lm

PROGS = convolve interp luminance noise saturation sbias stretch warp

all: $(PROGS)

.c:	../util/texture.h ../util/texture.c
	cc $(CFLAGS) -o $@ $< ../util/texture.c $(LIBS)

clean:
	- rm -f *.o
	@ for file in $(PROGS) dummy_file ; do               \
		if [ -x $$file ]; then rm -f $$file; fi;     \
	done			

