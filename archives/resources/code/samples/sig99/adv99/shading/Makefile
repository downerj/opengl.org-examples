# overrride with: make "CFLAGS = <whatever>" <target>
SHELL = /bin/sh
CFLAGS = -g -I../util -I/usr/include/GL -fullwarn
LIBS = -lglut -lGLU -lGL -lXmu -lXt -lX11 -lm

SPHERE_MAP_PROGS =  sm_sixviews sm_st2rvec sm_smapmesh sm_cview2smap

PROGS = anisolight alphablend alphablendnosort BRDF bumpmap curvedrefl \
		dark displace envmap glossmap highlight lightmap2d lighttessel \
		materials mirror multispheremap perspective projshadow screendoor \
		shadowvol shadtex spheres $(SPHERE_MAP_PROGS) spheremap \
		spotlight triangles screendoor2


all: $(PROGS)

.c:	../util/texture.h ../util/texture.c
	cc $(CFLAGS) -o $@ $< ../util/texture.c $(LIBS)

sm_cview2smap: sm_cview2smap.o sm_drawmesh.o sm_makemesh.o
	cc $(CFLAGS) -o $@ sm_cview2smap.o sm_drawmesh.o sm_makemesh.o $(LIBS)

clean:
	- rm -f *.o
	@for file in $(PROGS) dummy_file ; do                 \
		if [ -x $$file ]; then rm -f $$file; fi;       \
	done

