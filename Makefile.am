EXTRA_DIST=swf.h output-swf.c output-swf.h  

bin_PROGRAMS=autotrace

autotrace_SOURCES=atou.c fit.c  main.c  remove-suffx.c \
atou.h fit.h  main.h remove-suffx.h \
bitmap.c make-suffix.c \
bitmap.h make-suffix.h \
cmdline.h message.h  spline.c \
concat3.c getopt.c output-eps.c    spline.h \
concat3.h getopt.h output-eps.h    substring.c \
curve.c getopt1.c output-fig.c  substring.h \
curve.h image-header.h   output-fig.h  \
edge.c input-pnm.c output-sk.c  type.h \
edge.h input-pnm.h output-sk.h  types.h \
epsilon-equal.c output-svg.c  usefull.h \
epsilon-equal.h output-svg.h  vector.c \
extend-fname.c vector.h \
extend-fname.h version.c \
output.c  xfile.h input-bmp.c \
output.h  xfopen.c input-bmp.h \
xfseek.c output-p2e.c output-p2e.h\
xmem.h color.c color.h \
find-suffix.c logreport.c pxl-outline.c \
find-suffix.h logreport.h pxl-outline.h \
output-dxf.c output-dxf.h output-dxf12.c \
output-dxf12.h median.c median.h quantize.h

INCLUDES = $(GTK_DPS_CFLAGS)
autotrace_LDADD=	$(GTK_DPS_LDFLAGS) -lm
