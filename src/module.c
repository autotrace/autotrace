/*
 * Copyright (C) 2003 Martin Weber
 * Copyright (C) 2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2004 Steven P. Hirshman
 * SPDX-FileCopyrightText: © 2005 Robin Adams
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */
#include <stddef.h>

#include "private.h"

#include "input.h"
#if !HAVE_MAGICK_READERS
#include "input-bmp.h"
#include "input-pnm.h"
#include "input-tga.h"
#endif /* !HAVE_MAGICK_READERS */
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

#include "output.h"
#include "output-eps.h"
#include "output-er.h"
#include "output-p2e.h"
#include "output-sk.h"
#include "output-svg.h"
#include "output-ugs.h"
#include "output-fig.h"
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
  at_input_add_handler("PNG", "Portable network graphics (native)", input_png_reader);
#endif

#if !HAVE_MAGICK_READERS
  at_input_add_handler("BMP", "Microsoft Windows bitmap image (native)", input_bmp_reader);
  at_input_add_handler("TGA", "Truevision Targa image (native, 8 bit only)", input_tga_reader);
  at_input_add_handler_full("PBM", "Portable bitmap format (native)", input_pnm_reader, 0, "PBM",
                            NULL);
  at_input_add_handler_full("PGM", "Portable graymap format (native)", input_pnm_reader, 0, "PGM",
                            NULL);
  at_input_add_handler_full("PNM", "Portable anymap format (native)", input_pnm_reader, 0, "PNM",
                            NULL);
  at_input_add_handler_full("PPM", "Portable pixmap format (native)", input_pnm_reader, 0, "PPM",
                            NULL);
#endif /* HAVE_MAGICK_READERS */

  at_input_add_handler("GF", "TeX raster font (native)", input_gf_reader);

  return install_input_magick_readers();
}

static int install_output_writers(void)
{
  at_output_add_handler("AI", "Adobe Illustrator", output_eps_writer);
  at_output_add_handler("CGM", "Computer Graphics Metafile", output_cgm_writer);
  at_output_add_handler("DR2D", "IFF DR2D format", output_dr2d_writer);
  at_output_add_handler("DXF", "DXF format (without splines)", output_dxf12_writer);
  at_output_add_handler("EMF", "Enhanced Metafile format", output_emf_writer);
  at_output_add_handler("EPD", "EPD format", output_epd_writer);
  at_output_add_handler("EPS", "Encapsulated PostScript", output_eps_writer);
  at_output_add_handler("ER", "Elastic Reality Shape file", output_er_writer);
  at_output_add_handler("FIG", "XFIG 3.2", output_fig_writer);
  at_output_add_handler("ILD", "ILDA format", output_ild_writer);
  at_output_add_handler("MIF", "FrameMaker MIF format", output_mif_writer);
  at_output_add_handler("P2E", "pstoedit frontend format", output_p2e_writer);
  at_output_add_handler("PDF", "PDF format", output_pdf_writer);
  at_output_add_handler("PLT", "HPGL format", output_plt_writer);
  at_output_add_handler("POV", "Povray format", output_pov_writer);
  at_output_add_handler("SK", "Sketch", output_sk_writer);
  at_output_add_handler("SVG", "Scalable Vector Graphics", output_svg_writer);
  at_output_add_handler("UGS", "Unicode glyph source", output_ugs_writer);

  return install_output_pstoedit_writers();
}
