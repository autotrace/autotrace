EXTRA_DIST=swf.h output-swf.c output-swf.h  \
input-magick.h input-magick.c input-png.c input-png.h \
autotrace-config.af

AUTOMAKE_OPTIONS = foreign no-dependencies

autotraceincludedir=$(includedir)/autotrace
lib_LIBRARIES=libautotrace.a
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


output_src=output-eps.c output-eps.h \
output-fig.c  output-fig.h \
output-sk.c output-sk.h \
output-svg.c output-svg.h \
output-p2e.c output-p2e.h \
output-dxf.c output-dxf.h \
output-dxf12.c output-dxf12.h \
output-emf.c output-emf.h

libautotrace_a_SOURCES = version.c               \
                $(input_src) ${output_src}	\
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
		median.h                        \
		image-header.h    		\
		fit.h  				\
		bitmap.h 			\
		spline.h 			\
		curve.h 			\
		vector.h 			\
		color.h 			\
		pxl-outline.h

autotraceinclude_HEADERS=\
		types.h 			\
		autotrace.h

#
# noinst_HEADERS: headers shared between lib and bin.
# File not installed and not shared should be in
# libautotrace_a_SOURCES.
# TODO: input and output should be in libautotrace_a_SOURCES.
#      -- Masatake
noinst_HEADERS = filename.h                     \
		xstd.h                          \
		logreport.h			\
		usefull.h                       \
		message.h                       \
		input.h 			\
		output.h 			

autotrace_SOURCES=\
		atou.c 				\
		atou.h 				\
		main.c				\
		cmdline.h 			\
		getopt.c 			\
		getopt.h 			\
		getopt1.c

libautotrace_a_INCLUDES = $(MAGICK_CFLAGS)
autotrace_LDADD = libautotrace.a $(MAGICK_LDFLAGS) $(LIBPNG_LDFLAGS) -lm

bin_SCRIPTS=autotrace-config
BUILT_SOURCES=autotrace-config
autotrace-config: autotrace-config.in

%.in: %.af
	autofig $<
