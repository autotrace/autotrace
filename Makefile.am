EXTRA_DIST=swf.h output-swf.c output-swf.h  \
input-magick.h input-magick.c

AUTOMAKE_OPTIONS = foreign no-dependencies

bin_PROGRAMS=autotrace

if HAVE_MAGICK
input_magick_src=input-magick.h input-magick.c
else
input_magick_src=
endif

input_src=input-pnm.c input-pnm.h \
input-bmp.c input-bmp.h \
input-tga.c input-tga.h \
$(input_magick_src)

output_src=output-eps.c output-eps.h \
output-fig.c  output-fig.h \
output-sk.c output-sk.h \
output-svg.c output-svg.h \
output-p2e.c output-p2e.h \
output-dxf.c output-dxf.h \
output-dxf12.c output-dxf12.h \
output-emf.c output-emf.h

autotrace_SOURCES=$(input_src) ${output_src}	\
		atou.c 				\
		fit.c				\
		main.c				\
		remove-suffx.c			\
		atou.h 				\
		fit.h  				\
		main.h 				\
		remove-suffx.h 			\
		bitmap.c 			\
		make-suffix.c 			\
		bitmap.h 			\
		make-suffix.h 			\
		cmdline.h 			\
		message.h 			\
		spline.c 			\
		concat3.c 			\
		getopt.c 			\
		spline.h 			\
		concat3.h 			\
		getopt.h 			\
		substring.c 			\
		curve.c 			\
		getopt1.c 			\
		substring.h 			\
		curve.h 			\
		image-header.h    		\
		edge.c 				\
		edge.h 				\
		types.h 			\
		epsilon-equal.c 		\
		usefull.h 			\
		epsilon-equal.h 		\
		vector.c 			\
		extend-fname.c 			\
		vector.h 			\
		extend-fname.h 			\
		version.c 			\
		output.c 			\
		xfile.h 			\
		output.h 			\
		xfopen.c 			\
		xfseek.c 			\
		xmem.h 				\
		color.c 			\
		color.h 			\
		find-suffix.c 			\
		logreport.c 			\
		pxl-outline.c 			\
		find-suffix.h 			\
		logreport.h 			\
		pxl-outline.h 			\
		median.c 			\
		median.h 			\
		quantize.h 


INCLUDES = $(MAGICK_CFLAGS)
autotrace_LDADD=	$(MAGICK_LDFLAGS) -lm
