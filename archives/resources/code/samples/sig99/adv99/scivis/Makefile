# overrride with: make "CFLAGS = <whatever>" <target>
SHELL = /bin/sh
CFLAGS = -g -I../util -I/usr/include/GL -I/usr/X11/include -L/usr/X11/lib
LIBS = -lglut -lGLU -lGL -lXmu -lXt -lX11 -lm


PROGS = illumlines lic vol2dtex vol3dtex terrain plate voronoi

all: $(PROGS)

.c:	../util/texture.h ../util/texture.c
	cc $(CFLAGS) -o $@ $< ../util/texture.c $(LIBS)

terrain: terrain.o curve.o noise.o
	cc $(CFLAGS) -o $@ terrain.o curve.o noise.o $(LIBS)

clean:
	- rm -f *.o
	@ for file in $(PROGS) dummy_file ; do               \
		if [ -x $$file ]; then rm -f $$file; fi;     \
	done			

