SUBDIRS = po
man_MANS = autotrace.1
EXTRA_DIST=$(man_MANS) \
	   src/output-swf.c \
	   src/output-swf.h \
	   src/input-magick.h \
	   src/input-magick.c \
	   src/input-png.c \
	   src/input-png.h \
	   src/output-pstoedit.c \
	   src/output-pstoedit.h \
	   autotrace-config.af \
	   autotrace-config.in \
	   autotrace.m4 \
	   shape_cubic.c.diff \
	   doc/README.MING \
	   autotrace.pc.in \
	   TODO \
	   doc/HACKING \
	   doc/FAQ

AUTOMAKE_OPTIONS = foreign no-dependencies

autotraceincludedir=$(includedir)/autotrace
lib_LTLIBRARIES=libautotrace.la
bin_PROGRAMS=autotrace

if HAVE_MAGICK
input_magick_src=src/input-magick.h src/input-magick.c
else
input_magick_src=
endif

if HAVE_LIBPNG
input_png_src=src/input-png.c src/input-png.h
else
input_png_src=
endif

input_src=src/input-pnm.c src/input-pnm.h \
	  src/input-bmp.c src/input-bmp.h \
	  src/input-tga.c src/input-tga.h \
	  src/input-gf.c src/input-gf.h \
	  $(input_png_src) \
	  $(input_magick_src)

if HAVE_LIBSWF
output_swf_src=src/output-swf.c src/output-swf.h
else
output_swf_src=
endif

if HAVE_LIBPSTOEDIT
output_pstoedit_src=src/output-pstoedit.h src/output-pstoedit.c
else
output_pstoedit_src=
endif
output_src=src/output-eps.c src/output-eps.h \
	   src/output-er.c src/output-er.h \
	   src/output-fig.c src/output-fig.h \
	   src/output-sk.c src/output-sk.h \
	   src/output-svg.c src/output-svg.h \
	   src/output-ugs.c src/output-ugs.h \
	   src/output-p2e.c src/output-p2e.h \
	   src/output-emf.c src/output-emf.h \
	   src/output-dxf.c src/output-dxf.h \
	   src/output-epd.c src/output-epd.h \
	   src/output-pdf.c src/output-pdf.h \
	   src/output-mif.c src/output-mif.h \
	   src/output-cgm.c src/output-cgm.h \
	   src/output-dr2d.c src/output-dr2d.h \
	   src/output-pov.c src/output-pov.h \
	   src/output-plt.c src/output-plt.h \
	   src/output-ild.c src/output-ild.h \
	   $(output_swf_src) \
	   $(output_pstoedit_src)

libautotrace_la_SOURCES =\
                $(input_src) $(output_src) \
		src/fit.c \
		src/bitmap.c \
		src/spline.c \
		src/curve.c \
		src/epsilon-equal.c \
		src/vector.c \
		src/color.c \
		src/datetime.c \
		src/autotrace.c \
		src/output.c \
		src/input.c \
		src/pxl-outline.c \
		src/median.c \
		src/thin-image.c \
		src/logreport.c \
		src/filename.c \
		src/epsilon-equal.h \
		src/thin-image.h \
		src/quantize.h \
		src/image-header.h \
		src/fit.h \
		src/bitmap.h \
		src/spline.h \
		src/curve.h \
		src/vector.h \
		src/pxl-outline.h \
		src/despeckle.c \
		src/despeckle.h \
		src/exception.c \
		src/image-proc.c \
		src/image-proc.h \
		src/module.c \
		src/private.h \
		src/intl.h

autotraceinclude_HEADERS=\
		src/types.h \
		src/autotrace.h \
		src/input.h \
		src/output.h \
		src/exception.h \
		src/color.h

libautotrace_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

#
# noinst_HEADERS: headers shared between lib and bin.
# File not installed and not shared should be in
# libautotrace_a_SOURCES.
noinst_HEADERS = src/filename.h                     \
		src/xstd.h                          \
		src/logreport.h

autotrace_SOURCES=\
		src/atou.c				\
		src/atou.h				\
		src/main.c				\
		src/cmdline.h

AM_CPPFLAGS = $(MAGICK_CFLAGS) $(LIBPSTOEDIT_CFLAGS) $(LIBSWF_CFLAGS) $(GLIB2_CFLAGS) -DLOCALEDIR=\""$(datadir)/locale"\"

autotrace_LDADD =				\
		libautotrace.la			\
		$(MAGICK_LIBS)		\
		$(LIBPNG_LIBS)		\
		$(LIBSWF_LIBS)		\
		$(LIBPSTOEDIT_LIBS)		\
		$(GLIB2_LIBS)			\
		$(POPT_LIBS)			\
		$(INTLLIBS)			\
		-lm

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA= autotrace.pc

ACLOCAL_AMFLAGS = -I m4
