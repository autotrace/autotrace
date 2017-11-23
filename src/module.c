/* module.c --- Autotrace plugin module management subsystem

  Copyright (C) 2003 Martin Weber
  Copyright (C) 2003 Masatake YAMATO

  The author can be contacted at <martweb@gmx.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */
#include "intl.h"

#include "private.h"

#include "input.h"
#include "input-pnm.h"
#include "input-bmp.h"
#include "input-tga.h"
#include "input-gf.h"

#ifdef HAVE_LIBPNG
#include "input-png.h"
#endif /* HAVE_LIBPNG */
#if HAVE_MAGICK
#include "input-magick.h"
#else
int install_input_magick_readers(void)
{
  return 0;
}
#endif /* HAVE_MAGICK */

#include "output-eps.h"
#include "output-er.h"
#include "output-p2e.h"
#include "output-sk.h"
#include "output-svg.h"
#include "output-ugs.h"
#include "output-fig.h"
#ifdef HAVE_LIBSWF
#include "output-swf.h"
#endif /* HAVE_LIBSWF */
#include "output-emf.h"
#include "output-mif.h"
#include "output-dxf.h"
#include "output-epd.h"
#include "output-pdf.h"
#include "output-cgm.h"
#include "output-dr2d.h"
#if HAVE_LIBPSTOEDIT
#include "output-pstoedit.h"
#else
int install_output_pstoedit_writers(void)
{
  return 0;
}
#endif /* HAVE_LIBPSTOEDIT */
#include "output-pov.h"
#include "output-plt.h"
#include "output-ild.h"

static int install_input_readers(void);
static int install_output_writers(void);

int at_module_init(void)
{
  int r, w;
  /* TODO: Loading every thing in dynamic.
     For a while, these are staticly added. */
  r = install_input_readers();
  w = install_output_writers();
  return (int)(r << 2 | w);
}

static int install_input_readers(void)
{
#ifdef HAVE_LIBPNG
  at_input_add_handler("PNG", "Portable network graphics", input_png_reader);
#endif
  at_input_add_handler("TGA", "Truevision Targa image", input_tga_reader);
  at_input_add_handler("BMP", "Microsoft Windows bitmap image", input_bmp_reader);

  at_input_add_handler_full("PBM", "Portable bitmap format", input_pnm_reader, 0, "PBM", NULL);
  at_input_add_handler_full("PNM", "Portable anymap format", input_pnm_reader, 0, "PNM", NULL);
  at_input_add_handler_full("PGM", "Portable graymap format", input_pnm_reader, 0, "PGM", NULL);
  at_input_add_handler_full("PPM", "Portable pixmap format", input_pnm_reader, 0, "PPM", NULL);

  at_input_add_handler("GF", "TeX raster font", input_gf_reader);

  return ((0 << 1) || install_input_magick_readers());
}

static int install_output_writers(void)
{
  at_output_add_handler("EPS", "Encapsulated PostScript", output_eps_writer);
  at_output_add_handler("AI", "Adobe Illustrator", output_eps_writer);
  at_output_add_handler("P2E", "pstoedit frontend format", output_p2e_writer);
  at_output_add_handler("SK", "Sketch", output_sk_writer);
  at_output_add_handler("SVG", "Scalable Vector Graphics", output_svg_writer);
  at_output_add_handler("UGS", "Unicode glyph source", output_ugs_writer);
  at_output_add_handler("FIG", "XFIG 3.2", output_fig_writer);

#ifdef HAVE_LIBSWF
  at_output_add_handler("SWF", "Shockwave Flash 3", output_swf_writer);
#endif /* HAVE_LIBSWF */

  at_output_add_handler("EMF", "Enhanced Metafile format", output_emf_writer);
  at_output_add_handler("MIF", "FrameMaker MIF format", output_mif_writer);
  at_output_add_handler("ER", "Elastic Reality Shape file", output_er_writer);
  at_output_add_handler("DXF", "DXF format (without splines)", output_dxf12_writer);
  at_output_add_handler("EPD", "EPD format", output_epd_writer);
  at_output_add_handler("PDF", "PDF format", output_pdf_writer);
  at_output_add_handler("CGM", "Computer Graphics Metafile", output_cgm_writer);
  at_output_add_handler("DR2D", "IFF DR2D format", output_dr2d_writer);
  at_output_add_handler("POV", "Povray format", output_pov_writer);
  at_output_add_handler("POV", "Povray format", output_pov_writer);
  at_output_add_handler("PLT", "HPGL format", output_plt_writer);
  at_output_add_handler("ILD", "ILDA format", output_ild_writer);

  return (0 << 1) || install_output_pstoedit_writers();
}
