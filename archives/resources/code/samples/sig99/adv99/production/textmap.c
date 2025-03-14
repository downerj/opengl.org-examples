#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "GL/gl.h"
#include "textmap.h"
#include "texture.h"

static texfnt *curtfnt;
static unsigned char *fb;
static unsigned char *gptr;

/*
 *	get metric data into image
 *
 */
static void initget(void) {
    gptr = fb;
}

static void getbytes(void *pbuf,int n) {
    char *buf = pbuf;
    static int once;
    static int swapFlag;

    if (!once) {
	union {
	    int testWord;
	    char testByte[4];
	} endianTest;
	endianTest.testWord = 1;
	swapFlag = (endianTest.testByte[0] == 1);
	once = 1;
    }
    if (!swapFlag)  {
	/* big endian */
	while(n--) 
	     *buf++ = *gptr++;
    } else {
	/* little endian */
	buf = (char*)pbuf+n;
	while(n--) 
	     *--buf = *gptr++;
    }
}

static void fixrow(unsigned short *sptr,int n) {
    while(n--) {
	*sptr = *sptr + (0xff<<8);
	/*sptr = (*sptr<<8) + 0xff;*/
	sptr++;
    }
}

texfnt *readtexfont(char *name) {
    texfnt *tfnt;
    unsigned char *image;
    unsigned char *cptr;
    unsigned short *sbuf, *sptr;
    short advancecell, xadvance;
    short llx, lly, urx, ury, ox, oy;
    int i, y, extralines;
    texchardesc *cd;
    int xsize, ysize, components;

    tfnt = (texfnt *)malloc(sizeof(texfnt));
    image = (unsigned char*)read_texture(name, &xsize, &ysize, &components);
    if(!image) {
	 fprintf(stderr,"textmap: can't open font image %s\n",name);
	 return 0;
    }
    extralines = ysize-xsize;
    if(extralines<1) {
	 fprintf(stderr,"textmap: bad input font!!\n");
	 return 0;
    }
    fb = (unsigned char *)malloc(xsize*extralines);
    sbuf = (unsigned short *)malloc(xsize*sizeof(short));
    cptr = fb;
    for(y=xsize; y<ysize; y++) {
	int x;
	for(x = 0; x < xsize; x++)
	    cptr[x] = image[4*(y*xsize+x)];
	cptr += xsize;
    }
    initget();
    tfnt->rasxsize = xsize;
    tfnt->rasysize = xsize;
    getbytes(&tfnt->charmin,sizeof(short));
/*printf("charmin 0x%x\n", tfnt->charmin);*/
    getbytes(&tfnt->charmax,sizeof(short));
    getbytes(&tfnt->pixhigh,sizeof(float));
    getbytes(&advancecell,sizeof(short));
    tfnt->nchars = tfnt->charmax-tfnt->charmin+1;
    tfnt->chars = (texchardesc *)malloc(tfnt->nchars*sizeof(texchardesc));
    tfnt->rasdata = (unsigned short *)malloc(tfnt->rasxsize*tfnt->rasysize*sizeof(long));
    sptr = tfnt->rasdata;
    for(y=0; y<tfnt->rasysize; y++) {
	int x;
	for(x = 0; x < xsize; x++) {
	    unsigned t = image[4*(y*xsize+x)];
	    sptr[x] = t | t << 8;
	}
	fixrow(sptr,tfnt->rasxsize);
	sptr += tfnt->rasxsize;
    }

    cd = tfnt->chars;
    for(i=0; i<tfnt->nchars; i++) {
	/*int c = tfnt->charmin+i;*/
	getbytes(&xadvance,sizeof(short));
	getbytes(&llx,sizeof(short));
	getbytes(&lly,sizeof(short));
	getbytes(&urx,sizeof(short));
	getbytes(&ury,sizeof(short));
	getbytes(&ox,sizeof(short));
	getbytes(&oy,sizeof(short));
	cd->movex = xadvance/(float)advancecell;

	if(llx>=0) {
	    cd->haveimage = 1;
	    cd->llx = (llx-ox)/tfnt->pixhigh;
	    cd->lly = (lly-oy)/tfnt->pixhigh;
	    cd->urx = (urx-ox+1)/tfnt->pixhigh;
	    cd->ury = (ury-oy+1)/tfnt->pixhigh;
	    cd->tllx = llx/(float)tfnt->rasxsize;
	    cd->tlly = lly/(float)tfnt->rasysize;
	    cd->turx = (urx+1)/(float)tfnt->rasxsize;
	    cd->tury = (ury+1)/(float)tfnt->rasysize;
	    cd->data[0] = cd->tllx;
	    cd->data[1] = cd->tlly;

	    cd->data[2] = cd->llx;
	    cd->data[3] = cd->lly;

	    cd->data[4] = cd->turx;
	    cd->data[5] = cd->tlly;

	    cd->data[6] = cd->urx;
	    cd->data[7] = cd->lly;

	    cd->data[8] = cd->turx;
	    cd->data[9] = cd->tury;

	    cd->data[10] = cd->urx;
	    cd->data[11] = cd->ury;

	    cd->data[12] = cd->tllx;
	    cd->data[13] = cd->tury;

	    cd->data[14] = cd->llx;
	    cd->data[15] = cd->ury;

	    cd->data[16] = cd->llx;
	    cd->data[17] = cd->lly;
	    cd->data[18] = cd->urx;
	    cd->data[19] = cd->lly;

	    cd->data[20] = cd->urx;
	    cd->data[21] = cd->ury;
	    cd->data[22] = cd->llx;
	    cd->data[23] = cd->ury;

	} else {
	    cd->haveimage = 0;
	}
	cd++;
    }
    free(fb);
    free(sbuf);
    free(image);
    return tfnt;
}

void
texfont(texfnt *tfnt) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,0,2,tfnt->rasxsize,tfnt->rasysize,0,
		     GL_LUMINANCE_ALPHA,GL_UNSIGNED_BYTE,tfnt->rasdata);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    curtfnt = tfnt;
}

float texstrwidth(char *str) {
    int c;
    int charmin, tnchars;
    texfnt *tfnt;
    texchardesc *cdbase, *cd;
    float xpos;

    tfnt = curtfnt;
    if(!tfnt) {
	fprintf(stderr,"texstrwidth: no texfont set!!\n");
	return 0;
    }
    charmin = tfnt->charmin;
    tnchars = tfnt->nchars;
    cdbase = tfnt->chars;
    xpos = 0.0;
    while(*str) {
	c = *str-charmin;
	if(c<tnchars) {
	    cd = cdbase+c;
	    xpos += cd->movex;
	}
	str++;
    }
    return xpos;
}

void
texcharstr(char *str) {
    int c;
    int charmin, tnchars;
    texfnt *tfnt;
    texchardesc *cdbase, *cd;
    float *fdata, xpos;

    tfnt = curtfnt;
    if(!tfnt) {
	fprintf(stderr,"texcharstr: no texfont set!!\n");
	return;
    }
    charmin = tfnt->charmin;
    tnchars = tfnt->nchars;
    cdbase = tfnt->chars;
    xpos = 0.0;
    while(*str) {
	c = *str-charmin;
	if(c<tnchars) {
	    cd = cdbase+c;
	    if(cd->haveimage) {
		fdata = cd->data;
		fdata[16] = fdata[2]+xpos;
		fdata[18] = fdata[6]+xpos;
		fdata[20] = fdata[10]+xpos;
		fdata[22] = fdata[14]+xpos;
		glBegin(GL_POLYGON);
		    glTexCoord2fv(&fdata[0]);
		    glVertex2fv(&fdata[16]);
		    glTexCoord2fv(&fdata[4]);
		    glVertex2fv(&fdata[18]);
		    glTexCoord2fv(&fdata[8]);
		    glVertex2fv(&fdata[20]);
		    glTexCoord2fv(&fdata[12]);
		    glVertex2fv(&fdata[22]);
		glEnd();
	    }
	    xpos += cd->movex;
	}
	str++;
    }
}

int
texfntinit(char *file) {
    static int once = 0;
    static texfnt *tfnt;
    if (!once) {
	tfnt = readtexfont(file);
	if(!tfnt ) {
	    fprintf(stderr,"texfntinit: can't open input font %s\n", file);
	    return(-1);
	}
	texfont(tfnt);
	once = 1;
    }
    return 0;
}

float texfntwidth(char *str) {
    return texstrwidth(str)*12.5f/6.f;
}


void
texfntstroke(char *s, float xoffset, float yoffset) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPushMatrix();
    glTranslatef(xoffset,yoffset,0.0);
    glScalef(12.5, 12.5, 12.5);
    texcharstr(s);
    glPopMatrix();
    glDisable(GL_BLEND);
}
