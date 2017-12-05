/* input-gf.c: import TeX raster font files.

   Copyright (C) 2003 Serge Vakulenko <vak@cronyx.ru>.

   This file is based on sources of Fontutils package
   by Karl Berry and Kathryn Hargreaves.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "input-gf.h"
#include "output-ugs.h"
#include "bitmap.h"

#define WHITE		0

/*
 * GF opcodes.
 */
#define POST		248
#define POST_POST	249

#define GF_SIGNATURE	223
#define GF_ID		131

#define BOC		67
#define BOC1		68
#define EOC		69

#define PAINT_0		0
#define PAINT_63	63
#define PAINT1		64
#define PAINT2		65
#define PAINT3		66

#define SKIP0		70
#define SKIP1		71
#define SKIP2		72
#define SKIP3		73

#define NEW_ROW_0	74
#define NEW_ROW_164	238

#define XXX1		239
#define XXX2		240
#define XXX3		241
#define XXX4		242
#define YYY		243

#define NO_OP		244

#define CHAR_LOC	245
#define CHAR_LOC0	246

typedef struct _gf_char_t {
  unsigned char charcode;
  long h_escapement;
  long tfm_width;
  long char_pointer;
} gf_locator_t;

typedef struct _gf_font_t {
  char *input_filename;
  FILE *input_file;
  double design_size;
  unsigned long checksum;
  double h_pixels_per_point, v_pixels_per_point;
  long bbox_min_col;
  long bbox_max_col;
  long bbox_min_row;
  long bbox_max_row;
  gf_locator_t char_loc[256];
} gf_font_t;

/* The characters are the most important information in the GF file. */

typedef struct {
  gf_font_t *font;
  unsigned char charcode;
  long bbox_min_col;
  long bbox_max_col;
  long bbox_min_row;
  long bbox_max_row;
  long h_escapement;
  long tfm_width;
  unsigned short height, width;
  char *bitmap;
} gf_char_t;

/* This is the pixel at [ROW,COL]. */
#define PIXEL(s,r,c)	(s)->bitmap [(r) * (s)->width + (c)]

static unsigned char get_byte(gf_font_t * font)
{
  unsigned char b;

  if (fread(&b, 1, 1, font->input_file) != 1) {
    fprintf(stderr, "%s: read error\n", font->input_filename);
    exit(-1);
  }
  return b;
}

static unsigned short get_two(gf_font_t * font)
{
  unsigned short b;

  b = (unsigned short)get_byte(font) << 8;
  b |= get_byte(font);
  return b;
}

/*
 * C does not provide an obvious type for a 24-bit quantity, so we
 * return a 32-bit one.
 */
static unsigned long get_three(gf_font_t * font)
{
  unsigned long b;

  b = (unsigned long)get_byte(font) << 16;
  b |= (unsigned long)get_byte(font) << 8;
  b |= get_byte(font);
  return b;
}

static unsigned long get_four(gf_font_t * font)
{
  unsigned long b;

  b = (unsigned long)get_byte(font) << 24;
  b |= (unsigned long)get_byte(font) << 16;
  b |= (unsigned long)get_byte(font) << 8;
  b |= get_byte(font);
  return b;
}

static void move_relative(gf_font_t * font, long count)
{
  if (fseek(font->input_file, count, SEEK_CUR) < 0) {
    fprintf(stderr, "%s: seek error\n", font->input_filename);
    exit(-1);
  }
}

static unsigned char get_previous_byte(gf_font_t * font)
{
  unsigned char b;

  move_relative(font, -1);
  b = get_byte(font);
  move_relative(font, -1);
  return b;
}

static unsigned long get_previous_four(gf_font_t * font)
{
  unsigned long b;

  move_relative(font, -4);
  b = get_four(font);
  move_relative(font, -4);
  return b;
}

/*
 * Skip all specials, leaving the file pointer at the first non-special.
 * We do not save the specials, though.
 */
static void skip_specials(gf_font_t * font)
{
  for (;;) {
    switch (get_byte(font)) {
    case XXX1:
      move_relative(font, get_byte(font));
      continue;
    case XXX2:
      move_relative(font, get_two(font));
      continue;
    case XXX3:
      move_relative(font, get_three(font));
      continue;
    case YYY:
      (void)get_four(font);
      continue;
    default:
      move_relative(font, -1);
      return;
    }
  }
}

/*
 * The ``bitmap'' is a sequence of commands that describe it in terms of
 * run-length encoding.
 *
 * GF's row and column numbers are the lower left corner of a pixel.
 * GF (0,0) is the Cartesian unit square: 0 <= x (col) <= 1,
 * 0 <= y (row) <= 1.  Yes, it's <=, not <.  What does this mean for
 * the maxes and mins?  Let's take the height first:  if a character has
 * min_row = 0 and max_row = 10, we start the ``current'' y at 10,
 * (possibly) paint some pixels on that row, ..., and end up with it at
 * zero, (possibly) painting some pixels on that row.  Thus, there are
 * 11 (10 - 0 + 1) rows in which we might paint pixels.  Now the width:
 * if a character has min_row = 0 and max_row = 4, the current x starts
 * at zero, we paint four pixels (let's say), and now the current x is
 * four (the max possible), so we cannot paint any more.  Thus there are
 * four (4 - 0) columns in which we might paint pixels.
 *
 * Weird, huh?
 */
static void get_character_bitmap(gf_char_t * sym)
{
  unsigned char c;

  /* We expect these to be >= 0, but if the GF file is improper,
   * they might turn to be negative.
   */
  int height, width, painting_black = 0;
  int cur_x, cur_y;             /* This will be the GF position. */

  cur_x = sym->bbox_min_col;
  cur_y = sym->bbox_max_row;

  width = sym->bbox_max_col - sym->bbox_min_col;
  height = sym->bbox_max_row - sym->bbox_min_row + 1;

  /* If the character has zero or negative extent in either dimension,
   * it's not going to have a bitmap.  (If this happens, the GF file is
   * incorrect; but the discrepancy isn't serious, so we may as well not
   * bomb out when it happens, especially since PKtoGF has a bug that
   * produces such a bounding box when the character is all blank.)
   */
  if (width <= 0 || height <= 0) {
    sym->height = 0;
    sym->width = 0;
    sym->bitmap = 0;

    /* The next non-NO_OP byte should be EOC. */
    while ((c = get_byte(sym->font)) == NO_OP)
      continue;                 /* do nothing */
    if (c != EOC) {
      fprintf(stderr, "%s: expected eoc (for a blank character), found %u\n", sym->font->input_filename, c);
      exit(-1);
    }
    return;
  }

  sym->height = height;
  sym->width = width;
  sym->bitmap = calloc(width, height);
  if (!sym->bitmap) {
    fprintf(stderr, "%s: out of memory\n", sym->font->input_filename);
    exit(-1);
  }

  for (;;) {
    c = get_byte(sym->font);
    if (c == EOC)
      break;

    if ( /* PAINT_0 <= c && */ c <= PAINT3) {
      /* No need to test if `PAINT_0 <= c'; it must be,
       * since PAINT_0 is zero and `c' is unsigned.
       */
      /* The paint commands come in two varieties -- either
       * with the length implicitly part of the command,
       * or where it is specified as a separate parameter. */
      unsigned length;

      if ( /* PAINT_0 <= c && */ c <= PAINT_63)
        length = c - PAINT_0;
      else {
        switch (c) {
        case PAINT1:
          length = get_byte(sym->font);
          break;
        case PAINT2:
          length = get_two(sym->font);
          break;
        case PAINT3:
          length = get_three(sym->font);
          break;
        default:
          fprintf(stderr, "%s: invalid painting command %u\n", sym->font->input_filename, c);
          exit(-1);
        }
      }

      /* We have to translate from Cartesian to
       * C coordinates.  That means the x's are the same,
       * but the y's are flipped. */
      if (painting_black) {
        unsigned matrix_x, matrix_y;

        matrix_y = sym->bbox_max_row - cur_y;
        for (; length != 0; length--) {
          matrix_x = cur_x - sym->bbox_min_col;

          PIXEL(sym, matrix_y, matrix_x) = 255;
          cur_x++;
        }
      } else {
        cur_x += length;
      }
      painting_black = !painting_black;

    } else if (SKIP0 <= c && c <= SKIP3) {
      /* Skip commands move down in the GF character,
       * leaving blank rows. */
      unsigned rows_to_skip;

      switch (c) {
      case SKIP0:
        rows_to_skip = 0;
        break;
      case SKIP1:
        rows_to_skip = get_byte(sym->font);
        break;
      case SKIP2:
        rows_to_skip = get_two(sym->font);
        break;
      case SKIP3:
        rows_to_skip = get_three(sym->font);
        break;
      default:
        fprintf(stderr, "%s: invalid skip command %u\n", sym->font->input_filename, c);
        exit(-1);
      }
      cur_y -= rows_to_skip + 1;
      cur_x = sym->bbox_min_col;
      painting_black = 0;

    } else if (NEW_ROW_0 <= c && c <= NEW_ROW_164) {
      /* `new_row' commands move to the next line down
       * and then over. */
      cur_y--;
      cur_x = sym->bbox_min_col + c - NEW_ROW_0;
      painting_black = 1;

    } else if (c == NO_OP) {
      /* do nothing */

    } else if ((XXX1 <= c && c <= XXX4) || c == YYY) {
      skip_specials(sym->font);

    } else {
      fprintf(stderr, "%s: expected paint or skip or new_row, found %u\n", sym->font->input_filename, c);
      exit(-1);
    }
  }
}

/*
 * The GF format does not guarantee that the bounding box is the
 * smallest possible, i.e., that the character bitmap does not have
 * blank rows or columns at an edge.  We want to remove such blanks.
 */
static void deblank(gf_char_t * sym)
{
  unsigned all_white, white_on_left = 0, white_on_right = 0, white_on_top = 0, white_on_bottom = 0;
  int c, r;                     /* int in case GF_CHAR is zero pixels wide. */

  /* Let's start with blank columns at the left-hand side. */
  all_white = 1;
  for (c = 0; c < sym->width && all_white; c++) {
    for (r = 0; r < sym->height && all_white; r++) {
      if (PIXEL(sym, r, c) != WHITE)
        all_white = 0;
    }
    if (all_white)
      white_on_left++;
  }

  /* Now let's check the right-hand side. */
  all_white = 1;
  for (c = sym->width - 1; c >= 0 && all_white; c--) {
    for (r = 0; r < sym->height && all_white; r++) {
      if (PIXEL(sym, r, c) != WHITE)
        all_white = 0;
    }
    if (all_white)
      white_on_right++;
  }

  /* Check for all-white rows on top now. */
  all_white = 1;
  for (r = 0; r < sym->height && all_white; r++) {
    for (c = 0; c < sym->width && all_white; c++) {
      if (PIXEL(sym, r, c) != WHITE)
        all_white = 0;
    }
    if (all_white)
      white_on_top++;
  }

  /* And, last, for all-white rows on the bottom. */
  all_white = 1;
  for (r = sym->height - 1; r >= 0 && all_white; r--) {
    for (c = 0; c < sym->width && all_white; c++) {
      if (PIXEL(sym, r, c) != WHITE)
        all_white = 0;
    }
    if (all_white)
      white_on_bottom++;
  }

  /* If we have to remove columns at either the left or the right, we
   * have to reallocate the memory, since much code depends on the fact
   * that the bitmap is in contiguous memory.  If we have to remove
   * rows, we don't necessarily have to reallocate the memory, but we
   * might as well, to save space.
   */
  if (white_on_left > 0 || white_on_right > 0 || white_on_top > 0 || white_on_bottom > 0) {
    gf_char_t condensed;

    if (white_on_left + white_on_right > sym->width) {
      /* The character was entirely blank. */
      sym->width = 0;
      sym->height = 0;
      sym->bbox_min_col = 0;
      sym->bbox_max_col = 0;
      sym->bbox_min_row = 0;
      sym->bbox_max_row = 0;
      condensed.bitmap = 0;
    } else {
      condensed.width = sym->width - white_on_left - white_on_right;
      condensed.height = sym->height - white_on_top - white_on_bottom;
      condensed.bitmap = calloc(condensed.width, condensed.height);
      if (!condensed.bitmap) {
        fprintf(stderr, "%s: out of memory\n", sym->font->input_filename);
        exit(-1);
      }
      for (r = 0; r < condensed.height; r++)
        for (c = 0; c < condensed.width; c++) {
          PIXEL(&condensed, r, c) = PIXEL(sym, r + white_on_top, c + white_on_left);
        }
      sym->bbox_min_row += white_on_bottom;
      sym->bbox_max_row -= white_on_top;
      sym->bbox_min_col += white_on_left;
      sym->bbox_max_col -= white_on_right;
    }
    free(sym->bitmap);
    sym->bitmap = condensed.bitmap;
  }
}

static int gf_open(gf_font_t * font, char *filename)
{
  unsigned char b, c;
  unsigned long post_ptr;

  font->input_filename = filename;
  font->input_file = fopen(filename, "r");
  if (!font->input_file) {
    perror(filename);
    return 0;
  }

  if (fseek(font->input_file, 0, SEEK_END) < 0) {
    perror(filename);
    return 0;
  }
  /* Check that file is not empty, because we are trying
   * to seek before the beginning. */
  if (ftell(font->input_file) <= 0) {
    fprintf(stderr, "%s: empty file\n", font->input_filename);
    return 0;
  }

  do
    b = get_previous_byte(font);
  while (b == GF_SIGNATURE);
  if (b != GF_ID) {
    fprintf(stderr, "%s: invalid signature (expected %u, found %u)\n", font->input_filename, GF_ID, b);
    return 0;
  }

  post_ptr = get_previous_four(font);
  for (c = 0;; c++) {
    font->char_loc[c].charcode = c;
    font->char_loc[c].tfm_width = 0;
    font->char_loc[c].char_pointer = -1;
    if (c == 255)
      break;
  }

  fseek(font->input_file, post_ptr, SEEK_SET);
  b = get_byte(font);
  if (b != POST) {
    fprintf(stderr, "%s: invalid font structure (expected %u, found %u)\n", font->input_filename, POST, b);
    return 0;
  }
  get_four(font);               /* Ignore the special pointer. */

  font->design_size = get_four(font) / (double)(1L << 20);
  font->checksum = get_four(font);

  /* The resolution values are stored in the file as pixels per point,
     scaled by 2^16. */
  font->h_pixels_per_point = get_four(font) / (double)(1L << 16);
  font->v_pixels_per_point = get_four(font) / (double)(1L << 16);

  font->bbox_min_col = (signed long)get_four(font);
  font->bbox_max_col = (signed long)get_four(font);
  font->bbox_min_row = (signed long)get_four(font);
  font->bbox_max_row = (signed long)get_four(font);

  /* We do not know in advance how many character locators exist,
   * but we do place a maximum on it (contrary to what the GF format
   * definition says), namely, MAX_CHARCODE. */
  for (;;) {
    b = get_byte(font);
    if (b == POST_POST)
      break;

    c = get_byte(font);

    if (b == CHAR_LOC) {
      font->char_loc[c].h_escapement = (double)get_four(font) / (1 << 16) + .5;
      /* Ignore vertical escapement. */
      (void)get_four(font);

    } else if (b == CHAR_LOC0) {
      font->char_loc[c].h_escapement = get_byte(font);

    } else {
      fprintf(stderr, "%s: invalid char_loc command (found %u)\n", font->input_filename, b);
      return 0;
    }

    font->char_loc[c].tfm_width = get_four(font);
    font->char_loc[c].char_pointer = get_four(font);
  }
  return 1;
}

/*
 * Get any character by its code.
 */
static int gf_get_char(gf_font_t * font, gf_char_t * sym, unsigned char charcode)
{
  gf_locator_t *loc;
  unsigned char c, col_delta, row_delta;
  long lcode, back_pointer;

  loc = &font->char_loc[charcode];
  if (loc->char_pointer == -1)
    return 0;

  sym->font = font;
  sym->h_escapement = loc->h_escapement;
  sym->tfm_width = loc->tfm_width;

  if (fseek(font->input_file, loc->char_pointer, SEEK_SET) < 0) {
    fprintf(stderr, "%s: seek error\n", font->input_filename);
    return 0;
  }

  /* This reads the character starting from the current position
   * (but some specials might come first).
   */
  skip_specials(font);

  c = get_byte(font);
  switch (c) {
  case BOC:
    /* If the back pointer actually points somewhere,
     * this character is a ``residue'', and the font
     * is probably too big.
     */
    lcode = (long)get_four(font);
    if (lcode < 0 || lcode > 255) {
      /* Someone is trying to use a font with character codes
       * that are out of our range. */
      fprintf(stderr, "%s: invalid character code %ld (expected %d)\n", font->input_filename, lcode, charcode);
      return 0;
    }
    sym->charcode = lcode;

    back_pointer = (long)get_four(font);
    if (back_pointer != -1)
      fprintf(stderr, "%s: warning: character %u has a non-null back pointer (to %#lx)\n", font->input_filename, sym->charcode, back_pointer);

    sym->bbox_min_col = (long)get_four(font);
    sym->bbox_max_col = (long)get_four(font);
    sym->bbox_min_row = (long)get_four(font);
    sym->bbox_max_row = (long)get_four(font);
    break;

  case BOC1:
    sym->charcode = get_byte(font);

    col_delta = get_byte(font);
    sym->bbox_max_col = get_byte(font);
    sym->bbox_min_col = sym->bbox_max_col - col_delta;

    row_delta = get_byte(font);
    sym->bbox_max_row = get_byte(font);
    sym->bbox_min_row = sym->bbox_max_row - row_delta;
    break;

  case POST:
    return 0;

  default:
    fprintf(stderr, "%s: error reading character (found %u)\n", font->input_filename, c);
    return 0;
  }
  if (sym->charcode != charcode) {
    fprintf(stderr, "%s: warning: character code mismatch, %d != %d\n", font->input_filename, sym->charcode, charcode);
  }

  get_character_bitmap(sym);

  deblank(sym);
  return 1;
}

at_bitmap input_gf_reader(gchar * filename, at_input_opts_type * opts, at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{
  at_exception_type exp = at_exception_new(msg_func, msg_data);
  at_bitmap bitmap = at_bitmap_init(NULL, 0, 0, 0);
  gf_font_t fontdata, *font = &fontdata;
  gf_char_t chardata, *sym = &chardata;
  unsigned int i, j, ptr;

  if (!gf_open(font, filename)) {
    at_exception_fatal(&exp, "Cannot open input GF file");
    return bitmap;
  }
  if (opts->charcode == 0) {
    /* Find a first character in font file. */
    for (i = 0; i < 256; ++i)
      if (font->char_loc[i].char_pointer != -1)
        break;
    if (i >= 256) {
      at_exception_fatal(&exp, "No characters in input GF file");
      return bitmap;
    }
    opts->charcode = i;
  }
  if (!gf_get_char(font, sym, (unsigned char)opts->charcode)) {
    fclose(font->input_file);
    at_exception_fatal(&exp, "Error reading character from GF file");
    return bitmap;
  }

  ugs_design_pixels = font->design_size * font->v_pixels_per_point + 0.5;
  ugs_charcode = opts->charcode;
  ugs_advance_width = sym->h_escapement;
  ugs_left_bearing = sym->bbox_min_col;
  ugs_descend = sym->bbox_min_row;
  ugs_max_col = sym->bbox_max_col;
  ugs_max_row = sym->bbox_max_row;

  bitmap = at_bitmap_init(NULL, sym->width, sym->height, 1);
  for (j = 0, ptr = 0; j < sym->height; j++) {
    for (i = 0; i < sym->width; i++) {
      AT_BITMAP_BITS(&bitmap)[ptr++] = PIXEL(sym, j, i);
    }
  }
  free(sym->bitmap);
  fclose(font->input_file);
  return bitmap;
}
