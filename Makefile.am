EXTRA_DIST=swf.h output-swf.c output-swf.h  \
input-magick.h input-magick.c input-png.c input-png.h 

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

libautotrace_a_SOURCES= version.c               \
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
                xstd.c

autotraceinclude_HEADERS=\
		image-header.h    		\
		fit.h  				\
		bitmap.h 			\
		spline.h 			\
		curve.h 			\
		types.h 			\
		vector.h 			\
		color.h 			\
		autotrace.h                     \
		output.h 			\
		input.h 			\
		pxl-outline.h 			\
		median.h 			\
	        quantize.h                      \
	        thin-image.h

noinst_HEADERS = filename.h                     \
		xstd.h                          \
		epsilon-equal.h 		\
		logreport.h			\
		usefull.h                       \
		message.h

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
