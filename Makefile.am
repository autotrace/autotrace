man_MANS = autotrace.1
EXTRA_DIST=$(man_MANS)						\
output-swf.c output-swf.h					\
input-magick.h input-magick.c input-png.c input-png.h		\
output-pstoedit.c output-pstoedit.h				\
autotrace-config.af autotrace-config.in				\
autotrace.dsp autotrace.dsw					\
autotrace.m4 shape_cubic.c.diff README.MING			\
libming.dsw libming.dsp						\
autotrace.sln autotrace.vcproj libming.sln libming.vcproj	\
autotrace.pc.in autotrace.spec.in TODO HACKING FAQ

AUTOMAKE_OPTIONS = foreign no-dependencies

autotraceincludedir=$(includedir)/autotrace
lib_LTLIBRARIES=libautotrace.la
bin_PROGRAMS=autotrace

if HAVE_MAGICK
input_magick_src=input-magick.h input-magick.c
else
input_magick_src=
endif

if HAVE_LIBPNG
input_png_src=input-png.c input-png.h 
else
input_png_src=
endif

input_src=input-pnm.c input-pnm.h \
input-bmp.c input-bmp.h \
input-tga.c input-tga.h \
$(input_png_src) \
$(input_magick_src)

if HAVE_LIBSWF
output_swf_src=output-swf.c output-swf.h
else
output_swf_src=
endif

if HAVE_LIBPSTOEDIT
output_pstoedit_src=output-pstoedit.h output-pstoedit.c
else
output_pstoedit_src=
endif
output_src=output-eps.c output-eps.h \
output-er.c output-er.h \
output-fig.c  output-fig.h \
output-sk.c output-sk.h \
output-svg.c output-svg.h \
output-p2e.c output-p2e.h \
output-emf.c output-emf.h \
output-dxf.c output-dxf.h \
output-epd.c output-epd.h \
output-pdf.c output-pdf.h \
output-mif.c output-mif.h \
output-cgm.c output-cgm.h \
output-dr2d.c output-dr2d.h \
$(output_swf_src) \
$(output_pstoedit_src)

libautotrace_la_SOURCES =\
                $(input_src) $(output_src)	\
		fit.c				\
		bitmap.c 			\
		spline.c 			\
		curve.c 			\
		epsilon-equal.c 		\
		vector.c 			\
		color.c 			\
		autotrace.c			\
		output.c 			\
		input.c 			\
		pxl-outline.c 			\
		median.c 			\
	        thin-image.c			\
		logreport.c 			\
		filename.c                      \
                xstd.c                          \
                epsilon-equal.h 		\
	        thin-image.h                    \
	        quantize.h                      \
		image-header.h    		\
		fit.h  				\
		bitmap.h 			\
		spline.h 			\
		curve.h 			\
		vector.h 			\
		color.h 			\
		pxl-outline.h                   \
                despeckle.c                     \
                despeckle.h			\
	        exception.c                     \
		strgicmp.c			\
		image-proc.c			\
		image-proc.h

autotraceinclude_HEADERS=\
		types.h 			\
		autotrace.h                     \
		input.h				\
		output.h 			\
		exception.h

libautotrace_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

#
# noinst_HEADERS: headers shared between lib and bin.
# File not installed and not shared should be in
# libautotrace_a_SOURCES.
noinst_HEADERS = filename.h                     \
		xstd.h                          \
		logreport.h			\
		message.h                       

autotrace_SOURCES=\
		atou.c 				\
		atou.h 				\
		main.c				\
		cmdline.h 			\
		getopt.c 			\
		getopt.h 			\
		getopt1.c			\
		strgicmp.h

INCLUDES = $(MAGICK_CFLAGS) $(LIBPSTOEDIT_CFLAGS)
autotrace_LDADD =				\
		libautotrace.la			\
		$(MAGICK_LDFLAGS)		\
		$(LIBPNG_LDFLAGS)		\
		$(LIBSWF_LDFLAGS)		\
		$(LIBPSTOEDIT_LIBS)		\
		-lm

m4datadir = $(datadir)/aclocal
m4data_DATA = autotrace.m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA= autotrace.pc

bin_SCRIPTS=autotrace-config
BUILT_SOURCES=autotrace-config
autotrace-config: autotrace-config.in

autotrace-config.in: autotrace-config.af
	autofig $<

dist-hook: autotrace.spec
	cp autotrace.spec $(distdir)

rpm: dist autotrace.spec
	rpm -bb autotrace.spec
