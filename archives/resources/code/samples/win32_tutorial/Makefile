# Makefile for Win32 (nmake)


!include <win32.mak>


# defines
CFLAGS	= $(cflags) $(cdebug) -DWIN32
LIBS	= $(lflags) $(ldebug) glut.lib glu.lib opengl.lib $(guilibs)
SRCS	= animate.c mouse.c image.c simpledib.c minimal.c
EXES	= $(SRCS:.c=.exe)


# default rule
default	: $(EXES)


# cleanup rules
clean	:
	del *.obj *.pdb *.ilk *.ncb *~

clobber	: clean
	del *.exe


# inference rules
$(EXES)	: $*.obj
        $(link) -out:$@ $** $(LIBS)
.c.obj	: 
	$(CC) $(CFLAGS) $<


# dependencies (must come AFTER inference rules)
image.exe	: texture.obj