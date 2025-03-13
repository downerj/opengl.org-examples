# overrride with: make "CFLAGS = <whatever>" <target>
CFLAGS = -g

SUBDIRS = vissim \
	  shading \
	  production \
	  imaging \
	  scivis \
	  effects \
 	  cad \
	  natural

default: all

all clean:
	for d in $(SUBDIRS); do         \
		if test -f $$d/Makefile; then \
			echo "\tcd $$d; $(MAKE) $@"; \
			cd $$d; $(MAKE) $@; cd ..; \
		fi; \
	done
