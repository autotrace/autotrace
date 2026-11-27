/*
 * Copyright (C) 2000, 2001, 2002 Martin Weber
 * SPDX-FileCopyrightText: © 2000-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2001 Per Grahn
 * SPDX-FileCopyrightText: © 2001-2003 Martin Weber
 * SPDX-FileCopyrightText: © 2002 Ralf Stubner
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */
#include "intl.h"

#include "private.h"

#include "autotrace.h"
#include "exception.h"
#include "logreport.h"

#include "fit.h"
#include "spline.h"

#include "input.h"

#include <glib.h>
#include "image-header.h"
#include "image-proc.h"
#include "quantize.h"
#include "thin-image.h"
#include "despeckle.h"
#include "antialias.h"

#include <errno.h>
#include <locale.h>
#include <string.h>

#define AT_DEFAULT_DPI 72

at_fitting_opts_type *at_fitting_opts_new(void)
{
  at_fitting_opts_type *opts = g_new(at_fitting_opts_type, 1);
  *opts = new_fitting_opts();
  return opts;
}

at_fitting_opts_type *at_fitting_opts_copy(at_fitting_opts_type *original)
{
  at_fitting_opts_type *new_opts;
  if (original == NULL)
    return NULL;

  new_opts = at_fitting_opts_new();
  *new_opts = *original;
  if (original->background_color)
    new_opts->background_color = at_color_copy(original->background_color);
  return new_opts;
}

void at_fitting_opts_free(at_fitting_opts_type *opts)
{
  g_free(opts->background_color);
  g_free(opts);
}

/* Presets (issue #343).  Each sets only the options it names; the rest
   keep their defaults.  The values were chosen with the trace-quality
   corpus (tests/corpus); see the pull request that added them.  */

static void preset_lineart(at_fitting_opts_type *opts)
{
  opts->centerline = TRUE;
  opts->snap_antialiasing = TRUE;
  opts->despeckle_level = 2;
  opts->despeckle_tightness = 0.0;
  opts->noise_removal = 1.0;
}

static void preset_logo(at_fitting_opts_type *opts)
{
  opts->snap_antialiasing = TRUE;
  opts->error_threshold = 1.0;
  opts->keep_knees = TRUE;
}

static void preset_photo(at_fitting_opts_type *opts)
{
  opts->color_count = 32;
  opts->despeckle_level = 2;
}

static void preset_scan(at_fitting_opts_type *opts)
{
  preset_logo(opts);
  opts->despeckle_level = 4;
  opts->despeckle_tightness = 0.0;
  opts->noise_removal = 1.0;
}

static const struct {
  const gchar *name;
  const gchar *description;
  void (*apply)(at_fitting_opts_type *opts);
} presets[] = {
    {"lineart",
     N_("drawn lines: snap anti-aliased edges, remove one-pixel specks and trace the "
        "centerlines"),
     preset_lineart},
    {"logo",
     N_("flat colours with anti-aliased edges, such as logos, icons and text: snap the edges and "
        "fit them closely"),
     preset_logo},
    {"photo",
     N_("photographs and gradients: reduce to 32 colours and merge one-pixel specks into "
        "similar neighbours"),
     preset_photo},
    {"scan", N_("scanned drawings and text: like logo, and remove specks of up to 7 pixels"),
     preset_scan},
};

/* The names of PRESETS, in the same order.  */
static const gchar *const preset_names[] = {"lineart", "logo", "photo", "scan", NULL};
G_STATIC_ASSERT(G_N_ELEMENTS(preset_names) == G_N_ELEMENTS(presets) + 1);

static int find_preset(const gchar *name)
{
  gsize i;
  if (name == NULL)
    return -1;
  for (i = 0; i < G_N_ELEMENTS(presets); i++)
    if (g_ascii_strcasecmp(presets[i].name, name) == 0)
      return (int)i;
  return -1;
}

at_fitting_opts_type *at_fitting_opts_new_preset(const gchar *name)
{
  int i = find_preset(name);
  at_fitting_opts_type *opts;
  if (i < 0)
    return NULL;
  opts = at_fitting_opts_new();
  presets[i].apply(opts);
  return opts;
}

const gchar *const *at_fitting_opts_preset_names(void)
{
  return preset_names;
}

const gchar *at_fitting_opts_preset_description(const gchar *name)
{
  int i = find_preset(name);
  return i < 0 ? NULL : _(presets[i].description);
}

at_input_opts_type *at_input_opts_new(void)
{
  at_input_opts_type *opts = g_new0(at_input_opts_type, 1);
  return opts;
}

at_input_opts_type *at_input_opts_copy(at_input_opts_type *original)
{
  at_input_opts_type *opts;
  opts = at_input_opts_new();
  *opts = *original;
  if (original->background_color)
    opts->background_color = at_color_copy(original->background_color);
  return opts;
}

void at_input_opts_free(at_input_opts_type *opts)
{
  g_free(opts->background_color);
  g_free(opts);
}

at_output_opts_type *at_output_opts_new(void)
{
  at_output_opts_type *opts = g_new(at_output_opts_type, 1);
  opts->dpi = AT_DEFAULT_DPI;
  return opts;
}

at_output_opts_type *at_output_opts_copy(at_output_opts_type *original)
{
  at_output_opts_type *opts = at_output_opts_new();
  *opts = *original;
  return opts;
}

void at_output_opts_free(at_output_opts_type *opts)
{
  g_free(opts);
}

/* The client's message callback, and whether the reader reported a fatal
   error through it.  */
typedef struct {
  at_msg_func msg_func;
  gpointer msg_data;
  gboolean fatal;
} read_messages;

static void note_read_message(const gchar *msg, at_msg_type msg_type, gpointer data)
{
  read_messages *messages = data;

  if (msg_type == AT_MSG_FATAL)
    messages->fatal = TRUE;
  if (messages->msg_func)
    messages->msg_func(msg, msg_type, messages->msg_data);
}

at_bitmap *at_bitmap_read(at_bitmap_reader *reader, gchar *filename, at_input_opts_type *opts,
                          at_msg_func msg_func, gpointer msg_data)
{
  gboolean new_opts = FALSE;
  read_messages messages = {msg_func, msg_data, FALSE};
  at_bitmap *bitmap = g_new(at_bitmap, 1);
  if (opts == NULL) {
    opts = at_input_opts_new();
    new_opts = TRUE;
  }
  /* Readers report errors only through the callback, and return an empty
     bitmap (issue #414): watch the callback, and give the client NULL.
     Decide on the fatal message alone: an empty bitmap without one is a
     real result, such as a blank GF glyph (a font's space), whose metrics
     the UGS writer still needs.  */
  *bitmap = (*reader->func)(filename, opts, note_read_message, &messages, reader->data);
  if (new_opts)
    at_input_opts_free(opts);
  if (messages.fatal) {
    at_bitmap_free(bitmap);
    return NULL;
  }
  return bitmap;
}

at_bitmap *at_bitmap_new(unsigned short width, unsigned short height, unsigned int planes)
{
  at_bitmap *bitmap = g_new(at_bitmap, 1);
  *bitmap = at_bitmap_init(NULL, width, height, planes);
  return bitmap;
}

at_bitmap *at_bitmap_copy(const at_bitmap *src)
{
  at_bitmap *dist;
  unsigned short width, height, planes;

  width = at_bitmap_get_width(src);
  height = at_bitmap_get_height(src);
  planes = at_bitmap_get_planes(src);

  dist = at_bitmap_new(width, height, planes);
  memcpy(dist->bitmap, src->bitmap, (gsize)width * height * planes);
  return dist;
}

at_bitmap at_bitmap_init(unsigned char *area, unsigned short width, unsigned short height,
                         unsigned int planes)
{
  at_bitmap bitmap;
  gsize size = (gsize)width * height * planes;

  if (area)
    bitmap.bitmap = area;
  else {
    if (0 == (width * height))
      bitmap.bitmap = NULL;
    else
      bitmap.bitmap = g_new0(unsigned char, size);
  }

  bitmap.width = width;
  bitmap.height = height;
  bitmap.np = planes;
  return bitmap;
}

void at_bitmap_free(at_bitmap *bitmap)
{
  if (bitmap == NULL)
    return;
  g_free(bitmap->bitmap);
  g_free(bitmap);
}

unsigned short at_bitmap_get_width(const at_bitmap *bitmap)
{
  return bitmap->width;
}

unsigned short at_bitmap_get_height(const at_bitmap *bitmap)
{
  return bitmap->height;
}

unsigned short at_bitmap_get_planes(const at_bitmap *bitmap)
{
  /* Here we use cast rather changing the type definition of
     at_bitmap::np to keep binary compatibility. */
  return (unsigned short)bitmap->np;
}

void at_bitmap_get_color(const at_bitmap *bitmap, unsigned int row, unsigned int col,
                         at_color *color)
{
  unsigned char *p;
  g_return_if_fail(color);
  g_return_if_fail(bitmap);

  p = AT_BITMAP_PIXEL(bitmap, row, col);
  if (at_bitmap_get_planes(bitmap) >= 3)
    at_color_set(color, p[0], p[1], p[2]);
  else
    at_color_set(color, p[0], p[0], p[0]);
}

gboolean at_bitmap_equal_color(const at_bitmap *bitmap, unsigned int row, unsigned int col,
                               at_color *color)
{
  at_color c;

  g_return_val_if_fail(bitmap, FALSE);
  g_return_val_if_fail(color, FALSE);

  at_bitmap_get_color(bitmap, row, col, &c);
  return at_color_equal(&c, color);
}

at_splines_type *at_splines_new(at_bitmap *bitmap, at_fitting_opts_type *opts, at_msg_func msg_func,
                                gpointer msg_data)
{
  return at_splines_new_full(bitmap, opts, msg_func, msg_data, NULL, NULL, NULL, NULL);
}

/* The share of the progress that -snap-antialiasing reports.  The later
   steps report the rest through rest_progress(), so that the client's
   progress still runs once from 0 to 1.  */
#define SNAP_PROGRESS 0.1f

typedef struct {
  at_progress_func notify_progress;
  gpointer progress_data;
} client_progress;

static void snap_progress(gfloat percentage, gpointer data)
{
  client_progress *client = data;
  client->notify_progress(SNAP_PROGRESS * percentage, client->progress_data);
}

static void rest_progress(gfloat percentage, gpointer data)
{
  client_progress *client = data;
  client->notify_progress(SNAP_PROGRESS + (1 - SNAP_PROGRESS) * percentage, client->progress_data);
}

/* trace_bitmap modifies its argument BITMAP when snap_antialiased,
   despeckle, quantize and/or thin_image are invoked. */
static at_splines_type *trace_bitmap(at_bitmap *bitmap, at_fitting_opts_type *opts,
                                     at_msg_func msg_func, gpointer msg_data,
                                     at_progress_func notify_progress, gpointer progress_data,
                                     at_testcancel_func test_cancel, gpointer testcancel_data)
{
  client_progress client = {notify_progress, progress_data};
  image_header_type image_header;
  /* Allocated just before fitting, so that every earlier exit returns NULL
     (issue #372).  */
  at_splines_type *splines = NULL;
  pixel_outline_list_type pixels;
  QuantizeObj *myQuant = NULL; /* curently not used */
  at_exception_type exp = at_exception_new(msg_func, msg_data);
  at_distance_map dist_map, *dist = NULL;
  /* Every pass asks the client through CANCEL, which remembers a TRUE:
     after a pass, the check below must not ask the client again, since it
     may say TRUE only once (issue #407).  The passes that take a callback
     get remembered_cancel() in place of the client's.  */
  cancel_hook cancel = {test_cancel, testcancel_data, FALSE, 0};
  at_testcancel_func ask_cancel = test_cancel ? remembered_cancel : NULL;

#define CANCELP (cancel_requested(&cancel))
#define FATALP (at_exception_got_fatal(&exp))
#define FREE_SPLINE()                                                                              \
  do {                                                                                             \
    if (splines) {                                                                                 \
      at_splines_free(splines);                                                                    \
      splines = NULL;                                                                              \
    }                                                                                              \
  } while (0)

#define CANCEL_THEN_CLEANUP_DIST()                                                                 \
  if (CANCELP)                                                                                     \
    goto cleanup_dist;
#define CANCEL_THEN_CLEANUP_PIXELS()                                                               \
  if (CANCELP) {                                                                                   \
    FREE_SPLINE();                                                                                 \
    goto cleanup_pixels;                                                                           \
  }

#define FATAL_THEN_RETURN()                                                                        \
  if (FATALP)                                                                                      \
    return NULL;
#define FATAL_THEN_CLEANUP_DIST()                                                                  \
  if (FATALP)                                                                                      \
    goto cleanup_dist;
#define FATAL_THEN_CLEANUP_PIXELS()                                                                \
  if (FATALP) {                                                                                    \
    FREE_SPLINE();                                                                                 \
    goto cleanup_pixels;                                                                           \
  }

  /* Before despeckling, so that what remains of an edge is one colour.  */
  if (opts->snap_antialiasing) {
    if (!snap_antialiased(bitmap, notify_progress ? snap_progress : NULL, &client, ask_cancel,
                          &cancel, NULL))
      return NULL;
    if (notify_progress) {
      notify_progress = rest_progress;
      progress_data = &client;
    }
  }

  if (opts->despeckle_level > 0) {
    despeckle(bitmap, opts->despeckle_level, opts->despeckle_tightness, opts->noise_removal,
              &cancel, &exp);
    FATAL_THEN_RETURN();
    if (cancel.cancelled)
      return NULL;
  }

  image_header.width = at_bitmap_get_width(bitmap);
  image_header.height = at_bitmap_get_height(bitmap);

  if (opts->color_count > 0) {
    quantize(bitmap, opts->color_count, opts->background_color, &myQuant, &cancel, &exp);
    if (myQuant)
      quantize_object_free(myQuant); /* curently not used */
    FATAL_THEN_RETURN();
    if (cancel.cancelled)
      return NULL;
  }

  if (opts->centerline) {
    if (opts->preserve_width) {
      /* Preserve line width prior to thinning. */
      dist_map = new_distance_map(bitmap, 255, /*padded= */ TRUE, &cancel, &exp);
      dist = &dist_map;
      FATAL_THEN_CLEANUP_DIST();
    }
    /* Hereafter, dist is allocated. dist must be freed if
       the execution is canceled or exception is raised;
       use FATAL_THEN_CLEANUP_DIST. */
    if (!cancel.cancelled)
      thin_image(bitmap, opts->background_color, &cancel, &exp);
    FATAL_THEN_CLEANUP_DIST()
    if (cancel.cancelled)
      goto cleanup_dist;
  }

  /* Hereafter, pixels is allocated. pixels must be freed if
     the execution is canceled; use CANCEL_THEN_CLEANUP_PIXELS. */
  if (opts->centerline) {
    at_color background_color = {0xff, 0xff, 0xff};
    if (opts->background_color)
      background_color = *opts->background_color;

    pixels = find_centerline_pixels(bitmap, background_color, notify_progress, progress_data,
                                    ask_cancel, &cancel, &exp);
  } else
    pixels = find_outline_pixels(bitmap, opts->background_color, notify_progress, progress_data,
                                 ask_cancel, &cancel, &exp);
  FATAL_THEN_CLEANUP_DIST();
  CANCEL_THEN_CLEANUP_DIST();

  splines = g_new(at_splines_type, 1);
  *splines = fitted_splines(pixels, opts, dist, image_header.width, image_header.height, &exp,
                            notify_progress, progress_data, ask_cancel, &cancel);
  FATAL_THEN_CLEANUP_PIXELS();
  CANCEL_THEN_CLEANUP_PIXELS();

  if (notify_progress)
    notify_progress(1.0, progress_data);

cleanup_pixels:
  free_pixel_outline_list(&pixels);
cleanup_dist:
  if (dist)
    free_distance_map(dist);
  return splines;
#undef CANCELP
#undef FATALP
#undef FREE_SPLINE
#undef CANCEL_THEN_CLEANUP_DIST
#undef CANCEL_THEN_CLEANUP_PIXELS

#undef FATAL_THEN_RETURN
#undef FATAL_THEN_CLEANUP_DIST
#undef FATAL_THEN_CLEANUP_PIXELS
}

/* An RGB copy of the one-plane BITMAP.  */
static at_bitmap *rgb_copy(const at_bitmap *bitmap, cancel_hook *cancel)
{
  unsigned short width = at_bitmap_get_width(bitmap), height = at_bitmap_get_height(bitmap);
  gsize n = (gsize)width * height, i;
  at_bitmap *rgb;

  /* The copy of a big bitmap takes seconds: let the client cancel it.  */
  if (cancel_requested(cancel))
    return NULL;
  rgb = at_bitmap_new(width, height, 3);
  for (i = 0; i < n; i++) {
    if (i % STEPS_PER_CANCEL_CHECK == STEPS_PER_CANCEL_CHECK - 1 && cancel_requested(cancel)) {
      at_bitmap_free(rgb);
      return NULL;
    }
    memset(rgb->bitmap + 3 * i, bitmap->bitmap[i], 3);
  }
  return rgb;
}

/* at_splines_new_full modifies its argument BITMAP when snap_antialiased,
   despeckle, quantize and/or thin_image are invoked, except as below. */
at_splines_type *at_splines_new_full(at_bitmap *bitmap, at_fitting_opts_type *opts,
                                     at_msg_func msg_func, gpointer msg_data,
                                     at_progress_func notify_progress, gpointer progress_data,
                                     at_testcancel_func test_cancel, gpointer testcancel_data)
{
  const at_color *bg = opts->background_color;
  cancel_hook cancel = {test_cancel, testcancel_data, FALSE, 0};
  at_bitmap *rgb;
  at_splines_type *splines;

  /* No pixel of a grey bitmap equals a background colour that isn't grey,
     but each pass reduced such a colour to a grey of its own: the red
     channel, the luminance, or nothing at all (issue #421).  Trace an RGB
     copy instead, so that the result is the same as for an RGB image of
     the same pixels.  The client's bitmap is then left as it was.  */
  if (!(bg && at_bitmap_get_planes(bitmap) == 1 && (bg->r != bg->g || bg->g != bg->b)) ||
      bitmap->bitmap == NULL)
    return trace_bitmap(bitmap, opts, msg_func, msg_data, notify_progress, progress_data,
                        test_cancel, testcancel_data);
  /* A cancel during the copy is final, as in trace_bitmap() (issue #407).  */
  rgb = rgb_copy(bitmap, &cancel);
  if (rgb == NULL)
    return NULL;
  splines = trace_bitmap(rgb, opts, msg_func, msg_data, notify_progress, progress_data, test_cancel,
                         testcancel_data);
  at_bitmap_free(rgb);
  return splines;
}

/* at_splines_write, returning the writer's status: 0 for success.  */
static int write_splines(at_spline_writer *writer, FILE *writeto, gchar *file_name,
                         at_output_opts_type *opts, at_splines_type *splines, at_msg_func msg_func,
                         gpointer msg_data)
{
  int status;
  gboolean new_opts = FALSE;
  int llx, lly, urx, ury;
  llx = 0;
  lly = 0;
  urx = splines->width;
  ury = splines->height;

  if (!file_name)
    file_name = "";

  if (opts == NULL) {
    new_opts = TRUE;
    opts = at_output_opts_new();
  }

  /* The writers format numbers with printf(), which must produce a decimal
     point whatever the caller's locale says.  Switch LC_NUMERIC to "C" for
     the duration of the write only: a library must not leave its host's
     locale changed.  */
  g_autofree gchar *saved_locale = g_strdup(setlocale(LC_NUMERIC, NULL));
  setlocale(LC_NUMERIC, "C");
  status = (*writer->func)(writeto, file_name, llx, lly, urx, ury, opts, *splines, msg_func,
                           msg_data, writer->data);
  if (saved_locale)
    setlocale(LC_NUMERIC, saved_locale);

  if (new_opts)
    at_output_opts_free(opts);
  return status;
}

void at_splines_write(at_spline_writer *writer, FILE *writeto, gchar *file_name,
                      at_output_opts_type *opts, at_splines_type *splines, at_msg_func msg_func,
                      gpointer msg_data)
{
  write_splines(writer, writeto, file_name, opts, splines, msg_func, msg_data);
}

/* The functions with a GError (issue #345) call the ones without, with
   these callbacks in place of the client's.  The old functions are left
   as they were, so that their clients see no change.  */

typedef struct {
  at_msg_func msg_func;
  gpointer msg_data;
  GError *error; /* From the first fatal message.  */
  at_testcancel_func test_cancel;
  gpointer testcancel_data;
  gboolean cancelled; /* Whether test_cancel ever returned TRUE.  */
} error_catcher;

/* Pass warnings on to the client, and keep the first fatal message as the
   error, with the code its reporter gave it.  The client doesn't get the
   fatal messages: it gets the error.  */
static void catch_messages(const gchar *msg, at_msg_type msg_type, gpointer data)
{
  error_catcher *catcher = data;

  if (msg_type == AT_MSG_FATAL) {
    if (catcher->error == NULL)
      catcher->error = g_error_new_literal(AT_ERROR, at_exception_current_code(), msg ? msg : "");
  } else if (catcher->msg_func)
    catcher->msg_func(msg, msg_type, catcher->msg_data);
}

/* Ask the client, and remember whether it ever said TRUE.  */
static gboolean catch_cancel(gpointer data)
{
  error_catcher *catcher = data;

  if (catcher->test_cancel(catcher->testcancel_data)) {
    catcher->cancelled = TRUE;
    return TRUE;
  }
  return FALSE;
}

at_bitmap *at_bitmap_read_with_error(at_bitmap_reader *reader, gchar *filename,
                                     at_input_opts_type *opts, at_msg_func msg_func,
                                     gpointer msg_data, GError **error)
{
  error_catcher catcher = {msg_func, msg_data, NULL, NULL, NULL, FALSE};
  at_bitmap *bitmap;

  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  bitmap = at_bitmap_read(reader, filename, opts, catch_messages, &catcher);
  /* at_bitmap_read returns NULL exactly when there was a fatal message.  */
  if (catcher.error)
    g_propagate_error(error, catcher.error);
  return bitmap;
}

at_splines_type *at_splines_trace(at_bitmap *bitmap, at_fitting_opts_type *opts,
                                  at_msg_func msg_func, gpointer msg_data,
                                  at_progress_func notify_progress, gpointer progress_data,
                                  at_testcancel_func test_cancel, gpointer testcancel_data,
                                  GError **error)
{
  error_catcher catcher = {msg_func, msg_data, NULL, test_cancel, testcancel_data, FALSE};
  at_splines_type *splines;

  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  splines = at_splines_new_full(bitmap, opts, catch_messages, &catcher, notify_progress,
                                progress_data, test_cancel ? catch_cancel : NULL, &catcher);
  if (splines != NULL) {
    /* A fatal message always makes it return NULL, but be safe.  */
    g_clear_error(&catcher.error);
    return splines;
  }
  /* A fatal error wins over a cancel at the same time.  */
  if (catcher.error)
    g_propagate_error(error, catcher.error);
  else if (catcher.cancelled)
    g_set_error_literal(error, AT_ERROR, AT_ERROR_CANCELLED, _("Tracing was cancelled"));
  else
    g_set_error_literal(error, AT_ERROR, AT_ERROR_FAILED, _("Tracing failed"));
  return NULL;
}

/* Set ERROR for a failed write of FILE_NAME, which may be NULL or empty,
   for the REASON, which may be NULL.  Whole sentences, so that they can be
   translated.  */
static void set_write_error(GError **error, const gchar *file_name, const gchar *reason)
{
  gboolean named = file_name != NULL && *file_name != '\0';

  if (named && reason)
    g_set_error(error, AT_ERROR, AT_ERROR_FAILED, _("Writing %s failed: %s"), file_name, reason);
  else if (named)
    g_set_error(error, AT_ERROR, AT_ERROR_FAILED, _("Writing %s failed"), file_name);
  else if (reason)
    g_set_error(error, AT_ERROR, AT_ERROR_FAILED, _("Writing the output failed: %s"), reason);
  else
    g_set_error_literal(error, AT_ERROR, AT_ERROR_FAILED, _("Writing the output failed"));
}

gboolean at_splines_write_with_error(at_spline_writer *writer, FILE *writeto, gchar *file_name,
                                     at_output_opts_type *opts, at_splines_type *splines,
                                     at_msg_func msg_func, gpointer msg_data, GError **error)
{
  error_catcher catcher = {msg_func, msg_data, NULL, NULL, NULL, FALSE};
  gboolean had_error;
  int status;

  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
  /* Only a failure of this write counts, not one the stream already had.  */
  had_error = ferror(writeto) != 0;
  status = write_splines(writer, writeto, file_name, opts, splines, catch_messages, &catcher);
  if (catcher.error) {
    g_propagate_error(error, catcher.error);
    return FALSE;
  }
  if (status != 0) {
    set_write_error(error, file_name, NULL);
    return FALSE;
  }
  if (!had_error) {
    gboolean failed = fflush(writeto) != 0;
    int saved_errno = errno;
    if (failed || ferror(writeto)) {
      set_write_error(error, file_name, failed ? g_strerror(saved_errno) : NULL);
      return FALSE;
    }
  }
  return TRUE;
}

void at_splines_free(at_splines_type *splines)
{
  free_spline_list_array(splines);
  if (splines->background_color)
    at_color_free(splines->background_color);
  g_free(splines);
}

const char *at_version(gboolean long_format)
{
  if (long_format)
    return "AutoTrace version " AUTOTRACE_VERSION;

  return AUTOTRACE_VERSION;
}

const char *at_home_site(void)
{
  return AUTOTRACE_WEB;
}

void autotrace_init(void)
{
  static int initialized = 0;
  if (!initialized) {
#ifdef ENABLE_NLS
    /* Only register our message catalogue.  Selecting the locale is the
       host program's decision, not the library's.  */
    bindtextdomain(PACKAGE, LOCALEDIR);
#endif /* Def: ENABLE_NLS */

    /* Initialize subsystems */
    at_input_init();
    at_output_init();
    at_module_init();

    initialized = 1;
  }
}

const char *at_fitting_opts_doc_func(char *string)
{
  return _(string);
}
