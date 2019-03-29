[![Build Status](https://travis-ci.org/autotrace/autotrace.svg?branch=future)](https://travis-ci.org/autotrace/autotrace)


AutoTrace
=========

AutoTrace is a utility for converting bitmap into vector graphics.

Features
--------
- tracing outline and midline
- color reduction and despeckling
- supports a lot of input and output format

Licenses
--------
The program can be used under the GNU General Public License.

The input and output functions (`input-*.[ch]` and `output-*.[ch]`)
can also be used under the GNU Lesser General Public License(LGPL).

Some of code was partially derived from limn of GNU fontutils.
However, almost all code is rewritten.

Platforms
---------
The program was tested using GNU/Linux, HP UX, Solaris, Windows98,
Windows NT, Windows 2000, MAC and OS/2 4.0. It compiles with GCC,
Borland C++ Builder, Visual C++ and many other compilers.

If you use Visual C++ 6.0 for compilation be sure to have at
least SP 5 otherwise you could get Memory access violations due to
a bug in earlier versions.

Requirements
------------
AutoTrace can be compiled standalone, then it can import pnm, pbm,
pgm, ppm, bmp and tga files. If you have installed
[libpng](http://www.libpng.org/pub/png/libpng.html) you can also read png
files and with ImageMagick a very broad range of input formats is
available.

You will need at least libpng 1.0.6 and ImageMagick 5.2.1.  Most
output formats like dxf, emf, eps, ai, er, fig, svg, epd, dr2d and sk
are directly integrated in AutoTrace, but if you need swf export you
need to install [Ming](http://www.opaque.net/ming/). Also you can
export to the p2e format. This format can be converted by
[pstoedit](www.pstoedit.net) to a large number of other formats. If you have
installed the latest pstoedit (3.32 or newer), autotrace uses pstoedit
directly. However, direct pstoedit support is not stable enough. 
See INSTALL file for more detail.

Installation
------------
See the file `INSTALL`.

Usage
-----
Program comes from two parts: command and library.

Here the options you can use in the command:

    Usage: autotrace.exe [options] <input_name>.
    Options:<input_name> should be a supported image.
      You can use `--' or `-' to start an option.
      You can use any unambiguous abbreviation for an option name.
      You can separate option names and values with `=' or ` '.
    background-color <hexadecimal>: the color of the background that
      should be ignored, for example FFFFFF;
      default is no background color.
    centerline: trace a character's centerline, rather than its outline.
    color-count <unsigned>: number of colors a color bitmap is reduced to,
      it does not work on gray scale, allowed are 1..256;
      default is 0, that means not color reduction is done.
    corner-always-threshold <angle-in-degrees>: if the angle at a pixel is
      less than this, it is considered a corner, even if it is within
      `corner-surround' pixels of another corner; default is 60.
    corner-surround <unsigned>: number of pixels on either side of a
      point to consider when determining if that point is a corner;
      default is 4.
    corner-threshold <angle-in-degrees>: if a pixel, its predecessor(s),
      and its successor(s) meet at an angle smaller than this, it's a
      corner; default is 100.
    despeckle-level <unsigned>: 0..20; default is no despeckling.
    despeckle-tightness <real>: 0.0..8.0; default is 2.0.
    dpi <unsigned>: The dots per inch value in the input image, affects scaling
      of mif output image
    error-threshold <real>: subdivide fitted curves that are off by
      more pixels than this; default is 2.0.
    filter-iterations <unsigned>: smooth the curve this many times
      before fitting; default is 4.
    input-format:  TGA, PBM, PNM, PGM, PPM or BMP.
    help: print this message.
    line-reversion-threshold <real>: if a spline is closer to a straight
      line than this, weighted by the square of the curve length, keep it a
      straight line even if it is a list with curves; default is .01.
    line-threshold <real>: if the spline is not more than this far away
      from the straight line defined by its endpoints,
      then output a straight line; default is 1.
    list-output-formats: print a list of support output formats to stderr.
    list-input-formats:  print a list of support input formats to stderr.
    log: write detailed progress reports to <input_name>.log.
    output-file <filename>: write to <filename>
    output-format <format>: use format <format> for the output file
    output-format <format>: use format <format> for the output file
      eps, ai, p2e, sk, svg, fig, swf, emf, mif, er, dxf, epd, pdf, cgm or dr2d
      can be used.
    preserve-width: whether to preserve line width prior to thinning.\n\
    remove-adjacent-corners: remove corners that are adjacent.
    tangent-surround <unsigned>: number of points on either side of a
      point to consider when computing the tangent at that point; default is 3.
    report-progress: report tracing status in real time.
    debug-arch: print the type of cpu.
    debug-bitmap: dump loaded bitmap to <input_name>.bitmap.
    version: print the version number of this program.
    width-weight-factor: weight factor for fitting the line width.

The library is named libautotrace. About the usage of the library
see `autotrace.h`.
Here is a sample program that uses libautotrace.
To compile, invoke following commands (on posix):

    gcc sample.c `pkg-config --libs autotrace` `pkg-config --cflags autotrace`

    /* sample.c */
    #include <autotrace/autotrace.h>

    int main()
    {
      char * fname = "img/triangle.png";
      at_fitting_opts_type * opts = at_fitting_opts_new();
      at_input_read_func rfunc = at_input_get_handler(fname);
      at_bitmap_type * bitmap ;
      at_splines_type * splines;
      at_output_write_func wfunc = at_output_get_handler_by_suffix("eps");

      bitmap = at_bitmap_read(rfunc, fname, NULL, NULL, NULL);
      splines = at_splines_new(bitmap, opts, NULL, NULL);
      at_splines_write(wfunc, stdout, "", NULL, splines, NULL, NULL);
      return 0;
    }

GUI Frontend
------------
Frontline, a Gtk+/Gnome based GUI frontend, was under development until 2002.
See https://sourceforge.net/projects/autotrace/files/FrontLine/


More Information
----------------
See https://github.com/autotrace/autotrace


Contribution
------------
Programmers wanted!!!

See the [issue tracker](https://github.com/autotrace/autotrace/issues), or the `TODO` file and contact the author.

Author
------
Martin Weber (martweb@gmx.net)
