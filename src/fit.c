/*
 * Copyright (C) 1992 Free Software Foundation, Inc.
 * SPDX-FileCopyrightText: © 2000-2003 Martin Weber
 * SPDX-FileCopyrightText: © 2000-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2001 Dan Mills
 * SPDX-FileCopyrightText: © 2001 Peter Cucka
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 * SPDX-FileCopyrightText: © 2022 Biswapriyo Nath
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "autotrace.h"
#include "fit.h"
#include "logreport.h"
#include "spline.h"
#include "vector.h"
#include "curve.h"
#include "progress.h"
#include "pxl-outline.h"
#include "epsilon-equal.h"
#include <glib.h>
#include <math.h>
#ifndef FLT_MAX
#include <limits.h>
#include <float.h>
#endif
#ifndef FLT_MIN
#include <limits.h>
#include <float.h>
#endif
#include <string.h>
#include <assert.h>

#define SQUARE(x) ((x) * (x))
#define CUBE(x) ((x) * (x) * (x))

/* We need to manipulate lists of array indices.  */

typedef struct index_list {
  GArray *data; /* of unsigned */
} index_list_type;

/* The usual accessor macros.  */
#define GET_INDEX(i_l, n) g_array_index((i_l).data, unsigned, n)
#define INDEX_LIST_LENGTH(i_l) ((i_l).data->len)
#define GET_LAST_INDEX(i_l) GET_INDEX(i_l, INDEX_LIST_LENGTH(i_l) - 1)

static void append_index(index_list_type *, unsigned);
static void free_index_list(index_list_type *);
static index_list_type new_index_list(void);
static void remove_adjacent_corners(index_list_type *, unsigned, gboolean,
                                    at_exception_type *exception);
static void change_bad_lines(spline_list_type *, fitting_opts_type *);
static void filter(curve_type, fitting_opts_type *);
static void find_vectors(unsigned, pixel_outline_type, vector_type *, vector_type *, unsigned);
static index_list_type find_corners(pixel_outline_type, fitting_opts_type *,
                                    at_exception_type *exception);
static gfloat find_error(curve_type, spline_type, unsigned *, at_exception_type *exception);
static vector_type find_half_tangent(curve_type, gboolean start, unsigned *, unsigned);
static void find_tangent(curve_type, gboolean, gboolean, unsigned);
static spline_type fit_one_spline(curve_type, at_exception_type *exception);
static spline_list_type *fit_curve(curve_type, fitting_opts_type *, at_exception_type *exception);
static spline_list_type *fit_with_least_squares(curve_type, fitting_opts_type *,
                                                at_exception_type *exception);
static spline_list_type *fit_with_line(curve_type);
static void remove_knee_points(curve_type, gboolean);
static void set_initial_parameter_values(curve_type);
static gboolean spline_linear_enough(spline_type *, curve_type, fitting_opts_type *);
static at_coord real_to_int_coord(at_real_coord);
static gfloat distance(at_real_coord, at_real_coord);

/* Borders shared by two outlines (issue #338).

   Two colour regions that touch both trace the pixel border between them,
   in opposite directions.  Fitted separately, the two traces give two
   different curves, and the background shows through between them.  So each
   shared border is fitted once, by the outline that comes first (its owner),
   and the other outline reuses the owner's splines in reverse.

   For that, both outlines must split the border at the same points.  A point
   where the outline on the other side changes (a junction) is always a
   corner, and between junctions a point is a corner if either outline
   found one there (#458).  A curve then runs between two corners on one
   border only, and its fit depends on nothing but its own points.  */

typedef struct {
  unsigned start;       /* the point on this outline where the curve starts */
  int owner;            /* the outline that fits the curve, or -1 for this one */
  unsigned owner_start; /* the point on the owner where its copy starts */
} curve_border_type;

typedef struct {
  /* For the edge from point I to point I+1: the outline on its other side,
     or -1 for none, and the index of the same edge in that outline.  */
  int *partner;
  unsigned *partner_edge;
  /* Whether the outline has a junction.  Without one it shares nothing.  */
  gboolean shared;
  /* One entry per curve, in the order of the outline's curve list.  */
  GArray *curves; /* of curve_border_type */
  /* Once fitted: for the curve starting at point I, the index of its first
     spline in the outline's spline list (-1 for other points) and the
     number of its splines.  */
  int *first_spline;
  unsigned *n_splines;
} shared_border_type;

typedef struct {
  guint64 key;
  unsigned outline, edge;
} edge_key_type;

static int compare_edge_keys(const void *a, const void *b)
{
  guint64 key_a = ((const edge_key_type *)a)->key;
  guint64 key_b = ((const edge_key_type *)b)->key;

  return key_a < key_b ? -1 : key_a > key_b;
}

static void free_shared_borders(shared_border_type *borders, unsigned n_outlines)
{
  unsigned o;

  if (borders == NULL)
    return;
  for (o = 0; o < n_outlines; o++) {
    g_free(borders[o].partner);
    g_free(borders[o].partner_edge);
    if (borders[o].curves)
      g_array_free(borders[o].curves, TRUE);
    g_free(borders[o].first_spline);
    g_free(borders[o].n_splines);
  }
  g_free(borders);
}

/* Pair up the edges that two closed outlines share.  Every edge of an
   outline is one pixel long; sorting them all brings the two copies of a
   shared edge together.  Returns NULL if no edge is shared.  */

static shared_border_type *find_shared_borders(pixel_outline_list_type list)
{
  unsigned n_outlines = O_LIST_LENGTH(list), total = 0, n_keys = 0, o, i;
  edge_key_type *keys;
  shared_border_type *borders;
  gboolean found = FALSE;

  for (o = 0; o < n_outlines; o++)
    total += O_LENGTH(O_LIST_OUTLINE(list, o));
  if (total == 0)
    return NULL;

  keys = g_new(edge_key_type, total);
  borders = g_new0(shared_border_type, n_outlines);

  for (o = 0; o < n_outlines; o++) {
    pixel_outline_type outline = O_LIST_OUTLINE(list, o);
    unsigned n = O_LENGTH(outline);

    borders[o].partner = g_new(int, n);
    borders[o].partner_edge = g_new0(unsigned, n);
    for (i = 0; i < n; i++) {
      at_coord a = O_COORDINATE(outline, i), b = O_COORDINATE(outline, O_NEXT(outline, i));
      guint64 horizontal;

      borders[o].partner[i] = -1;
      if (outline.open)
        continue;
      if (a.y == b.y && (a.x + 1 == b.x || b.x + 1 == a.x))
        horizontal = 1;
      else if (a.x == b.x && (a.y + 1 == b.y || b.y + 1 == a.y))
        horizontal = 0;
      else
        continue;

      keys[n_keys].key =
          ((guint64)MIN(a.x, b.x) << 33) | ((guint64)MIN(a.y, b.y) << 1) | horizontal;
      keys[n_keys].outline = o;
      keys[n_keys].edge = i;
      n_keys++;
    }
  }

  qsort(keys, n_keys, sizeof(edge_key_type), compare_edge_keys);

  /* An edge lies between two pixels, so at most two outlines share it, and
     its two copies are next to each other after sorting.  */
  for (i = 0; i + 1 < n_keys; i++) {
    edge_key_type *k1 = &keys[i], *k2 = &keys[i + 1];
    pixel_outline_type o1, o2;
    at_coord start1, end2;

    if (k1->key != k2->key || k1->outline == k2->outline)
      continue;

    /* The two outlines must run along the edge in opposite directions.  */
    o1 = O_LIST_OUTLINE(list, k1->outline);
    o2 = O_LIST_OUTLINE(list, k2->outline);
    start1 = O_COORDINATE(o1, k1->edge);
    end2 = O_COORDINATE(o2, O_NEXT(o2, k2->edge));
    if (start1.x != end2.x || start1.y != end2.y)
      continue;

    borders[k1->outline].partner[k1->edge] = k2->outline;
    borders[k1->outline].partner_edge[k1->edge] = k2->edge;
    borders[k2->outline].partner[k2->edge] = k1->outline;
    borders[k2->outline].partner_edge[k2->edge] = k1->edge;
    found = TRUE;
  }
  g_free(keys);

  if (!found) {
    free_shared_borders(borders, n_outlines);
    return NULL;
  }
  return borders;
}

/* Whether point V of outline O is a junction: the edges before and after
   it have different outlines on their other side, or the same one but not
   next to each other there.  */

static gboolean is_junction(pixel_outline_list_type list, shared_border_type *borders, unsigned o,
                            unsigned v)
{
  shared_border_type *b = &borders[o];
  unsigned n = O_LENGTH(O_LIST_OUTLINE(list, o));
  unsigned in = v == 0 ? n - 1 : v - 1;
  int q = b->partner[v];

  if (b->partner[in] != q)
    return TRUE;
  if (q < 0)
    return FALSE;
  return b->partner_edge[in] != (b->partner_edge[v] + 1) % O_LENGTH(O_LIST_OUTLINE(list, q));
}

/* Append the splines FROM[FIRST..FIRST+COUNT) to LIST in reverse order,
   each one reversed.  */

static void append_reversed_splines(spline_list_type *list, spline_list_type from, unsigned first,
                                    unsigned count)
{
  unsigned k;

  for (k = count; k > 0; k--) {
    spline_type s = SPLINE_LIST_ELT(from, first + k - 1);
    at_real_coord p = START_POINT(s);

    START_POINT(s) = END_POINT(s);
    END_POINT(s) = p;
    p = CONTROL1(s);
    CONTROL1(s) = CONTROL2(s);
    CONTROL2(s) = p;
    append_spline(list, s);
  }
}

/* Work on all the outlines at once (issue #155).

   Most of the work on an outline depends on nothing but the outline
   itself: finding its corners, splitting it into curves, and fitting
   them.  (A curve on a shared border is fitted only by its owner, and
   the other outline copies the result when the spline lists are put
   together, in order.)  So each of these steps runs on a pool of
   threads, and the calling thread takes the results in the order of the
   outlines.  The pool hands out runs of consecutive outlines: one
   outline at a time costs more in locking than finding its corners does.

   The client's message function is called on the calling thread, and in
   the same order as without threads.  So a task raises its messages on an
   exception of its own, which keeps them, and the calling thread passes
   them on when it takes the task's outline.  */

typedef void (*outline_task_func)(unsigned outline, at_exception_type *exception, gpointer data);

typedef struct {
  at_msg_type type;
  gchar *text;
} kept_message_type;

typedef struct {
  GArray *messages; /* of kept_message_type, or NULL if none */
  gboolean done;
} outline_task_type;

typedef struct {
  outline_task_func func;
  gpointer data;
  unsigned n_outlines;
  unsigned chunk; /* the number of outlines in one item of the pool */
  outline_task_type *tasks;
  GThreadPool *pool; /* NULL: run each task when the calling thread takes it */
  gint cancelled;
  GMutex lock;
  GCond done;
} outline_tasks_type;

/* How many threads to use: the number in the environment variable
   AUTOTRACE_THREADS, or else one per processor.

   One while every debugging message is generated, as for a library client
   that hasn't called at_set_log_level() (see log_level_enabled()):
   g_log() takes one lock for all threads, so that threads mostly wait for
   each other there; with 22 of them, tracing took three times as long as
   with one.  One as well when debugging messages are wanted (-log debug),
   so that the messages of each outline stay together.  */
static unsigned n_threads(unsigned n_outlines)
{
  const gchar *env = g_getenv("AUTOTRACE_THREADS");
  guint64 n = 0;

  if (!log_filters_messages() || log_shows_debug())
    return 1;
  if (env == NULL || !g_ascii_string_to_unsigned(env, 10, 1, G_MAXUINT, &n, NULL))
    n = g_get_num_processors();
  return MAX(1, MIN((unsigned)n, n_outlines));
}

static void keep_message(const gchar *msg, at_msg_type msg_type, gpointer client_data)
{
  outline_task_type *task = client_data;
  kept_message_type m;

  m.type = msg_type;
  m.text = g_strdup(msg);
  if (task->messages == NULL)
    task->messages = g_array_new(FALSE, FALSE, sizeof(kept_message_type));
  g_array_append_val(task->messages, m);
}

static void free_kept_messages(outline_task_type *task)
{
  unsigned i;

  if (task->messages == NULL)
    return;
  for (i = 0; i < task->messages->len; i++)
    g_free(g_array_index(task->messages, kept_message_type, i).text);
  g_array_free(task->messages, TRUE);
  task->messages = NULL;
}

static void run_outline_task(outline_tasks_type *t, unsigned o)
{
  at_exception_type exception = at_exception_new(keep_message, &t->tasks[o]);

  t->func(o, &exception, t->data);
}

static void outline_worker(gpointer data, gpointer user_data)
{
  outline_tasks_type *t = user_data;
  unsigned first = (GPOINTER_TO_UINT(data) - 1) * t->chunk;
  unsigned end = MIN(first + t->chunk, t->n_outlines), o;

  for (o = first; o < end; o++)
    if (!g_atomic_int_get(&t->cancelled))
      run_outline_task(t, o);
  g_mutex_lock(&t->lock);
  for (o = first; o < end; o++)
    t->tasks[o].done = TRUE;
  g_cond_broadcast(&t->done);
  g_mutex_unlock(&t->lock);
}

/* Start FUNC (O, exception, DATA) for each outline O below N_OUTLINES.
   FUNC may run on any thread, so it must change nothing that the task of
   another outline uses.  */
static void start_outline_tasks(outline_tasks_type *t, unsigned n_outlines, outline_task_func func,
                                gpointer data)
{
  unsigned threads = n_threads(n_outlines), o;

  t->func = func;
  t->data = data;
  t->n_outlines = n_outlines;
  /* Enough items to keep all threads busy while the outlines differ in
     size, but not so many that the threads keep waiting for the lock.  */
  t->chunk = MAX(1, n_outlines / (threads * 16));
  t->tasks = g_new0(outline_task_type, n_outlines);
  t->pool = NULL;
  t->cancelled = FALSE;
  g_mutex_init(&t->lock);
  g_cond_init(&t->done);
  if (threads > 1)
    t->pool = g_thread_pool_new(outline_worker, t, threads, FALSE, NULL);
  /* The pool takes the tasks in order, so the first outlines, which the
     calling thread waits for first, are done first.  */
  for (o = 0; t->pool != NULL && o * t->chunk < n_outlines; o++)
    g_thread_pool_push(t->pool, GUINT_TO_POINTER(o + 1), NULL);
}

/* Wait until the task of outline O is done (without threads, run it now),
   and pass its messages on to EXCEPTION.  Nothing is passed on after a
   fatal error: with one exception shared by all outlines, as before
   threads, the outlines after it stopped at once and raised nothing.  */
static void take_outline_task(outline_tasks_type *t, unsigned o, at_exception_type *exception)
{
  outline_task_type *task = &t->tasks[o];
  unsigned i;

  if (t->pool == NULL)
    run_outline_task(t, o);
  else {
    g_mutex_lock(&t->lock);
    while (!task->done)
      g_cond_wait(&t->done, &t->lock);
    g_mutex_unlock(&t->lock);
  }
  for (i = 0; task->messages != NULL && i < task->messages->len; i++) {
    kept_message_type *m = &g_array_index(task->messages, kept_message_type, i);

    if (at_exception_got_fatal(exception))
      break;
    if (m->type == AT_MSG_FATAL)
      at_exception_fatal(exception, m->text);
    else
      at_exception_warning(exception, m->text);
  }
  free_kept_messages(task);
}

/* Drop the tasks not started yet, and wait for the ones running.  */
static void end_outline_tasks(outline_tasks_type *t)
{
  unsigned o;

  if (t->pool != NULL) {
    g_atomic_int_set(&t->cancelled, TRUE);
    g_thread_pool_free(t->pool, TRUE, TRUE);
  }
  for (o = 0; o < t->n_outlines; o++)
    free_kept_messages(&t->tasks[o]);
  g_free(t->tasks);
  g_mutex_clear(&t->lock);
  g_cond_clear(&t->done);
}

/* Run FUNC on every outline, and wait for all of them.  */
static void for_each_outline(unsigned n_outlines, outline_task_func func, gpointer data,
                             at_exception_type *exception)
{
  outline_tasks_type t;
  unsigned o;

  start_outline_tasks(&t, n_outlines, func, data);
  for (o = 0; o < n_outlines; o++)
    take_outline_task(&t, o, exception);
  end_outline_tasks(&t);
}

static curve_list_array_type split_at_corners(pixel_outline_list_type, fitting_opts_type *,
                                              shared_border_type *, at_exception_type *exception);
static void fit_outline(unsigned, at_exception_type *, gpointer);
static spline_list_type assemble_outline(curve_list_type, shared_border_type *, unsigned,
                                         spline_list_array_type *, spline_list_type **,
                                         at_exception_type *exception);

/* Get a new set of fitting options */
fitting_opts_type new_fitting_opts(void)
{
  fitting_opts_type fitting_opts;

  fitting_opts.background_color = NULL;
  fitting_opts.charcode = 0;
  fitting_opts.color_count = 0;
  fitting_opts.corner_always_threshold = (gfloat)60.0;
  fitting_opts.corner_surround = 4;
  fitting_opts.corner_threshold = (gfloat)100.0;
  fitting_opts.error_threshold = (gfloat)2.0;
  fitting_opts.filter_iterations = 4;
  fitting_opts.line_reversion_threshold = (gfloat).01;
  fitting_opts.line_threshold = (gfloat)1.0;
  fitting_opts.remove_adjacent_corners = FALSE;
  fitting_opts.tangent_surround = 3;
  fitting_opts.despeckle_level = 0;
  fitting_opts.despeckle_tightness = 2.0;
  fitting_opts.noise_removal = (gfloat)0.99;
  fitting_opts.centerline = FALSE;
  fitting_opts.preserve_width = FALSE;
  fitting_opts.width_weight_factor = 6.0;
  fitting_opts.snap_antialiasing = FALSE;
  fitting_opts.keep_knees = FALSE;

  return (fitting_opts);
}

/* The top-level call that transforms the list of pixels in the outlines
   of the original character to a list of spline lists fitted to those
   pixels.  */

typedef struct {
  curve_list_array_type *curves;
  fitting_opts_type *fitting_opts;
  at_distance_map *dist;
  shared_border_type *borders;
  /* Per outline, per curve: its splines, or NULL if the curve is
     borrowed or could not be fitted.  */
  spline_list_type ***fits;
} fit_job_type;

static void free_outline_fits(spline_list_type **fits, unsigned n_curves)
{
  unsigned i;

  if (fits == NULL)
    return;
  for (i = 0; i < n_curves; i++)
    if (fits[i] != NULL) {
      free_spline_list(*fits[i]);
      g_free(fits[i]);
    }
  g_free(fits);
}

/* Report the progress before outline THIS_LIST of N_LISTS is put together,
   and return whether the client cancelled.  */
static gboolean progress_then_cancelled(unsigned this_list, unsigned n_lists,
                                        at_progress_func notify_progress, gpointer progress_data,
                                        at_testcancel_func test_cancel, gpointer testcancel_data)
{
  if (notify_progress)
    notify_progress(step_progress(1, this_list, n_lists), progress_data);
  return test_cancel && test_cancel(testcancel_data);
}

spline_list_array_type fitted_splines(pixel_outline_list_type pixel_outline_list,
                                      fitting_opts_type *fitting_opts, at_distance_map *dist,
                                      unsigned short width, unsigned short height,
                                      at_exception_type *exception,
                                      at_progress_func notify_progress, gpointer progress_data,
                                      at_testcancel_func test_cancel, gpointer testcancel_data)
{
  unsigned this_list, n_lists;
  fit_job_type job;
  outline_tasks_type tasks;
  gboolean cancelled;

  spline_list_array_type char_splines = new_spline_list_array();
  /* Centerlines are open and never share a border.  */
  shared_border_type *borders =
      fitting_opts->centerline ? NULL : find_shared_borders(pixel_outline_list);
  curve_list_array_type curve_array =
      split_at_corners(pixel_outline_list, fitting_opts, borders, exception);

  char_splines.centerline = fitting_opts->centerline;
  char_splines.preserve_width = fitting_opts->preserve_width;
  char_splines.width_weight_factor = fitting_opts->width_weight_factor;

  if (fitting_opts->background_color)
    char_splines.background_color = at_color_copy(fitting_opts->background_color);
  else
    char_splines.background_color = NULL;
  /* Set dummy values. Real value is set in upper context. */
  char_splines.width = width;
  char_splines.height = height;

  n_lists = CURVE_LIST_ARRAY_LENGTH(curve_array);
  job.curves = &curve_array;
  job.fitting_opts = fitting_opts;
  job.dist = dist;
  job.borders = borders;
  job.fits = g_new0(spline_list_type **, n_lists);
  /* Ask about the first outline before any is fitted, as without threads.
     After that, a cancel waits only for the outlines being fitted, at most
     one per thread.  */
  cancelled = n_lists > 0 && progress_then_cancelled(0, n_lists, notify_progress, progress_data,
                                                     test_cancel, testcancel_data);
  start_outline_tasks(&tasks, cancelled ? 0 : n_lists, fit_outline, &job);

  for (this_list = 0; this_list < n_lists; this_list++) {
    spline_list_type curve_list_splines;
    curve_list_type curves = CURVE_LIST_ARRAY_ELT(curve_array, this_list);

    if (cancelled ||
        (this_list > 0 && progress_then_cancelled(this_list, n_lists, notify_progress,
                                                  progress_data, test_cancel, testcancel_data)))
      goto cleanup;

    take_outline_task(&tasks, this_list, exception);
    /* The caller frees the result, background colour included, with
       at_splines_free().  Freeing the colour here as well freed it twice.  */
    if (at_exception_got_fatal(exception))
      goto cleanup;
    curve_list_splines =
        assemble_outline(curves, borders, this_list, &char_splines, job.fits[this_list], exception);
    free_outline_fits(job.fits[this_list], CURVE_LIST_LENGTH(curves));
    job.fits[this_list] = NULL;
    curve_list_splines.clockwise = curves.clockwise;

    /* Centerline curves run through pixel indices, but a pixel's centre is
       half a pixel further on both axes (issue #366).  Outline curves run
       along pixel edges and are already in place.  Shifting all four
       control points shifts each Bezier curve exactly.  */
    if (fitting_opts->centerline) {
      unsigned this_spline;

      for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(curve_list_splines); this_spline++) {
        spline_type *spline = &SPLINE_LIST_ELT(curve_list_splines, this_spline);
        at_real_coord *points[] = {&START_POINT(*spline), &CONTROL1(*spline), &CONTROL2(*spline),
                                   &END_POINT(*spline)};
        unsigned k;

        for (k = 0; k < G_N_ELEMENTS(points); k++) {
          points[k]->x += 0.5;
          points[k]->y += 0.5;
        }
      }
    }

    memcpy(&(curve_list_splines.color), &(O_LIST_OUTLINE(pixel_outline_list, this_list).color),
           sizeof(at_color));
    append_spline_list(&char_splines, curve_list_splines);
  }
cleanup:
  /* After a cancel or a fatal error, outlines may still be fitting.  */
  end_outline_tasks(&tasks);
  for (this_list = 0; this_list < n_lists; this_list++)
    free_outline_fits(job.fits[this_list],
                      CURVE_LIST_LENGTH(CURVE_LIST_ARRAY_ELT(curve_array, this_list)));
  g_free(job.fits);
  free_curve_list_array(&curve_array, notify_progress, progress_data);
  free_shared_borders(borders, O_LIST_LENGTH(pixel_outline_list));

  return char_splines;
}

#define CURVE_BORDER(n) g_array_index(border->curves, curve_border_type, n)
#define BORROWED(n) (border != NULL && CURVE_BORDER(n).owner >= 0)

/* Fit the curves of outline THIS_OUTLINE of the job DATA.  Its curve list
   represents a single closed path, e.g., either the inside or outside
   outline of an `o'.  This changes nothing but the curves of that list and
   the outline's entry in the job's fits.

   A curve on a border that an earlier outline owns is not fitted again:
   assemble_outline() takes the owner's splines instead.  */

static void fit_outline(unsigned this_outline, at_exception_type *exception, gpointer data)
{
  fit_job_type *job = data;
  curve_list_type curve_list = CURVE_LIST_ARRAY_ELT(*job->curves, this_outline);
  fitting_opts_type *fitting_opts = job->fitting_opts;
  at_distance_map *dist = job->dist;
  curve_type curve;
  unsigned this_curve, this_spline;
  unsigned curve_list_length = CURVE_LIST_LENGTH(curve_list);
  shared_border_type *border = job->borders != NULL && job->borders[this_outline].shared
                                   ? &job->borders[this_outline]
                                   : NULL;
  spline_list_type **fits = g_new0(spline_list_type *, curve_list_length);

  job->fits[this_outline] = fits;
  DEBUG("\nFitting curve list #%u:\n", this_outline);

  /* Remove the extraneous ``knee'' points before filtering.  Since the
     corners have already been found, we don't need to worry about
     removing a point that should be a corner.  -keep-knees skips this
     (issue #346).  */

  if (!fitting_opts->keep_knees) {
    DEBUG("\nRemoving knees:\n");
    for (this_curve = 0; this_curve < curve_list_length; this_curve++) {
      DEBUG("#%u:", this_curve);
      if (!BORROWED(this_curve))
        remove_knee_points(CURVE_LIST_ELT(curve_list, this_curve),
                           CURVE_LIST_CLOCKWISE(curve_list));
    }
  }

  if (dist != NULL) {
    unsigned this_point;
    unsigned height = dist->height;
    for (this_curve = 0; this_curve < curve_list_length; this_curve++) {
      curve = CURVE_LIST_ELT(curve_list, this_curve);
      for (this_point = 0; this_point < CURVE_LENGTH(curve); this_point++) {
        unsigned x, y;
        float width, w;
        at_real_coord *coord = &CURVE_POINT(curve, this_point);
        x = (unsigned)(coord->x);
        y = height - (unsigned)(coord->y) - 1;

        /* Each (x, y) is a point on the skeleton of the curve, which
           might be offset from the TRUE centerline, where the width
           is maximal.  Therefore, use as the local line width the
           maximum distance over the neighborhood of (x, y).  */
        width = dist->d[y][x];
        if (y >= 1) {
          if ((w = dist->d[y - 1][x]) > width)
            width = w;
          if (x >= 1) {
            if ((w = dist->d[y][x - 1]) > width)
              width = w;
            if ((w = dist->d[y - 1][x - 1]) > width)
              width = w;
          }
          if (x + 1 < dist->width) {
            if ((w = dist->d[y][x + 1]) > width)
              width = w;
            if ((w = dist->d[y - 1][x + 1]) > width)
              width = w;
          }
        }
        if (y + 1 < height) {
          if ((w = dist->d[y + 1][x]) > width)
            width = w;
          if (x >= 1 && (w = dist->d[y + 1][x - 1]) > width)
            width = w;
          if (x + 1 < dist->width && (w = dist->d[y + 1][x + 1]) > width)
            width = w;
        }
        /* WIDTH is the distance from (near) the centreline to the centre
           of the nearest background pixel, half a pixel beyond the edge.
           Doubled, it is a pixel too wide on a line of odd width, whose
           middle pixel lies on the centreline, and right on one of even
           width, whose two middle rows straddle it (#453).  Telling them
           apart pixel by pixel makes the width jump along a jagged line,
           so take off half that error everywhere: a black line along a
           pixel row or column is then at most half a pixel off either
           way.  A pixel only partly inked counts for only part of a step
           of the distance, so weigh the correction the same way.  */
        width -= 0.25F * dist->weight[y][x];
        coord->z = width * (fitting_opts->width_weight_factor);
      }
    }
  }

  /* We filter all the curves in CURVE_LIST at once; otherwise, we would
     look at an unfiltered curve when computing tangents.  */

  DEBUG("\nFiltering curves:\n");
  for (this_curve = 0; this_curve < CURVE_LIST_LENGTH(curve_list); this_curve++) {
    DEBUG("#%u: ", this_curve);
    if (!BORROWED(this_curve))
      filter(CURVE_LIST_ELT(curve_list, this_curve), fitting_opts);
  }

  /* Make the first point in the first curve also be the last point in
     the last curve, so the fit to the whole curve list will begin and
     end at the same point.  This may cause slight errors in computing
     the tangents and t values, but it's worth it for the continuity.
     Of course we don't want to do this if the two points are already
     the same, as they are if the curve is cyclic.  (We don't append it
     earlier, in `split_at_corners', because that confuses the
     filtering.)  Finally, we can't append the point if the curve is
     exactly three points long, because we aren't adding any more data,
     and three points isn't enough to determine a spline.  Therefore,
     the fitting will fail.  */
  curve = CURVE_LIST_ELT(curve_list, 0);
  if (CURVE_CYCLIC(curve) == TRUE)
    append_point(curve, CURVE_POINT(curve, 0));

  /* Finally, fit each curve in the list to a list of splines.  */
  for (this_curve = 0; this_curve < curve_list_length; this_curve++) {
    spline_list_type *curve_splines;

    if (BORROWED(this_curve))
      continue;

    DEBUG("\nFitting curve #%u:\n", this_curve);
    curve_splines = fit_curve(CURVE_LIST_ELT(curve_list, this_curve), fitting_opts, exception);
    if (at_exception_got_fatal(exception)) {
      if (curve_splines != NULL) {
        free_spline_list(*curve_splines);
        g_free(curve_splines);
      }
      return;
    } else if (curve_splines == NULL) {
      DEBUG("Could not fit curve #%u", this_curve);
      at_exception_warning(exception, "Could not fit curve");
    } else {
      DEBUG("Fitted splines for curve #%u:\n", this_curve);
      for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(*curve_splines); this_spline++) {
        DEBUG("  %u: ", this_spline);
        print_spline(SPLINE_LIST_ELT(*curve_splines, this_spline));
      }

      /* After fitting, we may need to change some would-be lines
         back to curves, because they are in a list with other
         curves.  */
      change_bad_lines(curve_splines, fitting_opts);
      fits[this_curve] = curve_splines;
    }
  }
}

/* Join the splines that fit_outline() put into FITS into the spline list
   of CURVE_LIST, outline number THIS_OUTLINE, and return it.  The splines
   of a borrowed curve are the owner's, in reverse, taken from DONE, the
   spline lists of the earlier outlines.  Runs on the calling thread, in
   the order of the outlines.  */

static spline_list_type assemble_outline(curve_list_type curve_list, shared_border_type *borders,
                                         unsigned this_outline, spline_list_array_type *done,
                                         spline_list_type **fits, at_exception_type *exception)
{
  unsigned this_curve, this_spline;
  spline_list_type curve_list_splines = empty_spline_list();
  shared_border_type *border =
      borders != NULL && borders[this_outline].shared ? &borders[this_outline] : NULL;

  curve_list_splines.open = curve_list.open;

  for (this_curve = 0; this_curve < CURVE_LIST_LENGTH(curve_list); this_curve++) {
    unsigned first = SPLINE_LIST_LENGTH(curve_list_splines);

    if (BORROWED(this_curve)) {
      curve_border_type cb = CURVE_BORDER(this_curve);
      shared_border_type *owner = &borders[cb.owner];
      int owner_first = owner->first_spline ? owner->first_spline[cb.owner_start] : -1;

      DEBUG("\nCurve #%u is the reverse of one on outline #%d.\n", this_curve, cb.owner);
      if (owner_first < 0)
        at_exception_warning(exception, "Could not find the fit of a shared border");
      else
        append_reversed_splines(&curve_list_splines, SPLINE_LIST_ARRAY_ELT(*done, cb.owner),
                                owner_first, owner->n_splines[cb.owner_start]);
    } else if (fits[this_curve] != NULL)
      concat_spline_lists(&curve_list_splines, *fits[this_curve]);

    if (border != NULL) {
      border->first_spline[CURVE_BORDER(this_curve).start] = first;
      border->n_splines[CURVE_BORDER(this_curve).start] =
          SPLINE_LIST_LENGTH(curve_list_splines) - first;
    }
  }

  DEBUG("\nFitted splines are:\n");
  for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(curve_list_splines); this_spline++) {
    DEBUG("  %u: ", this_spline);
    print_spline(SPLINE_LIST_ELT(curve_list_splines, this_spline));
  }
  return curve_list_splines;
}

#undef BORROWED
#undef CURVE_BORDER

/* Transform a set of locations to a list of splines (the fewer the
   better).  We are guaranteed that CURVE does not contain any corners.
   We return NULL if we cannot fit the points at all.  */

static spline_list_type *fit_curve(curve_type curve, fitting_opts_type *fitting_opts,
                                   at_exception_type *exception)
{
  spline_list_type *fittedsplines;

  if (CURVE_LENGTH(curve) < 2) {
    DEBUG("Tried to fit curve with less than two points");
    at_exception_warning(exception, "Tried to fit curve with less than two points");
    return NULL;
  }

  /* Do we have enough points to fit with a spline?  */
  fittedsplines = CURVE_LENGTH(curve) < 4 ? fit_with_line(curve)
                                          : fit_with_least_squares(curve, fitting_opts, exception);

  return fittedsplines;
}

/* As mentioned above, the first step is to find the corners in
   PIXEL_LIST, the list of points.  (Presumably we can't fit a single
   spline around a corner.)  The general strategy is to look through all
   the points, remembering which we want to consider corners.  Then go
   through that list, producing the curve_list.  This is dictated by the
   fact that PIXEL_LIST does not necessarily start on a corner---it just
   starts at the character's first outline pixel, going left-to-right,
   top-to-bottom.  But we want all our splines to start and end on real
   corners.

   For example, consider the top of a capital `C' (this is in cmss20):
                     x
                     ***********
                  ******************

   PIXEL_LIST will start at the pixel below the `x'.  If we considered
   this pixel a corner, we would wind up matching a very small segment
   from there to the end of the line, probably as a straight line, which
   is certainly not what we want.

   PIXEL_LIST has one element for each closed outline on the character.
   To preserve this information, we return an array of curve_lists, one
   element (which in turn consists of several curves, one between each
   pair of corners) for each element in PIXEL_LIST.  */

/* The corners of PIXEL_O on its own, before any border is shared.  */

static index_list_type outline_corners(pixel_outline_type pixel_o, fitting_opts_type *fitting_opts,
                                       at_exception_type *exception)
{
  index_list_type corner_list;

  /* find_corners() needs at least `corner_surround' more pixels on
     either side of a point before it is conceivable that we might want
     another corner.  The endpoints of an open outline are automatically
     corners.  */
  if (O_LENGTH(pixel_o) > fitting_opts->corner_surround * 2 + 2)
    corner_list = find_corners(pixel_o, fitting_opts, exception);

  else if (!pixel_o.open) {
    /* A closed outline this short is already the polygon it should
       become: the pixel edges of a small region, or the pixel centres of
       a small closed centerline.  Searched for corners with a smaller
       surround, or fitted as one curve, it lost them: a region of one or
       two pixels collapsed to a line with no area, and neighbours that
       share its border left a hole (#448).  Keep the points where the
       outline turns as corners, so that its sides are fitted as lines.  */
    unsigned p;
    corner_list = new_index_list();
    for (p = 0; p < O_LENGTH(pixel_o); p++) {
      at_coord a = O_COORDINATE(pixel_o, O_PREV(pixel_o, p));
      at_coord b = O_COORDINATE(pixel_o, p);
      at_coord c = O_COORDINATE(pixel_o, O_NEXT(pixel_o, p));
      int cross = (b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x);
      int dot = (b.x - a.x) * (c.x - b.x) + (b.y - a.y) * (c.y - b.y);
      if (cross != 0 || dot < 0)
        append_index(&corner_list, p);
    }
  }

  else {
    int surround;
    if ((surround = (int)(O_LENGTH(pixel_o) - 3) / 2) >= 2) {
      unsigned save_corner_surround = fitting_opts->corner_surround;
      fitting_opts->corner_surround = surround;
      corner_list = find_corners(pixel_o, fitting_opts, exception);
      fitting_opts->corner_surround = save_corner_surround;
    } else {
      corner_list = new_index_list();
    }
  }
  return corner_list;
}

/* Replace the corners of outline O by the ones its shared borders need:
   every junction, and on each border the corners of both outlines that
   share it, so that both split it alike.  IS_CORNER[Q][I] tells whether point I
   is one of outline Q's own corners.  Leaves CORNERS[O] alone if O has no
   junction.  */

static void share_corners(pixel_outline_list_type pixel_list, shared_border_type *borders,
                          unsigned o, gboolean **is_corner, index_list_type *corners)
{
  shared_border_type *b = &borders[o];
  unsigned n = O_LENGTH(O_LIST_OUTLINE(pixel_list, o)), v;
  index_list_type shared = new_index_list();

  for (v = 0; v < n; v++) {
    int q = b->partner[v];
    gboolean corner;

    if (is_junction(pixel_list, borders, o, v))
      corner = b->shared = TRUE;
    else {
      /* A corner of either side is a corner of the border, so that both
         fit it alike.  A corner only one side can see, such as the end
         of a line one pixel wide, which the long outline beside it is
         too coarse to find, would otherwise be smoothed away (#458).
         On outline Q, point V is where edge V ends.  */
      corner = is_corner[o][v];
      if (!corner && q >= 0)
        corner = is_corner[q][(b->partner_edge[v] + 1) % O_LENGTH(O_LIST_OUTLINE(pixel_list, q))];
    }
    if (corner)
      append_index(&shared, v);
  }

  if (b->shared) {
    free_index_list(&corners[o]);
    corners[o] = shared;
    b->curves = g_array_new(FALSE, FALSE, sizeof(curve_border_type));
    b->first_spline = g_new(int, n);
    for (v = 0; v < n; v++)
      b->first_spline[v] = -1;
    b->n_splines = g_new0(unsigned, n);
  } else
    free_index_list(&shared);
}

/* Record that the curve of outline O from point START to point END is the
   next one in its curve list, and who fits it.  */

static void add_curve_border(pixel_outline_list_type pixel_list, shared_border_type *borders,
                             unsigned o, unsigned start, unsigned end)
{
  shared_border_type *b = &borders[o];
  unsigned n = O_LENGTH(O_LIST_OUTLINE(pixel_list, o));
  curve_border_type cb;
  int q = b->partner[start];

  cb.start = start;
  cb.owner = -1;
  cb.owner_start = 0;
  if (q >= 0 && (unsigned)q < o) {
    /* The copy on Q starts where this curve ends, at the end of the
       curve's last edge.  */
    cb.owner = q;
    cb.owner_start = b->partner_edge[(end + n - 1) % n];
  }
  g_array_append_val(b->curves, cb);
}

typedef struct {
  pixel_outline_list_type pixel_list;
  fitting_opts_type *fitting_opts;
  shared_border_type *borders;
  index_list_type *corners;
  gboolean **is_corner; /* NULL if no border is shared */
  curve_list_type *curve_lists;
} split_job_type;

/* Find the corners of outline O alone.  */
static void find_own_corners(unsigned o, at_exception_type *exception, gpointer data)
{
  split_job_type *job = data;
  pixel_outline_type pixel_o = O_LIST_OUTLINE(job->pixel_list, o);
  /* outline_corners() changes corner_surround for short outlines.  */
  fitting_opts_type fitting_opts = *job->fitting_opts;
  unsigned i;

  DEBUG("#%u:", o);
  job->corners[o] = outline_corners(pixel_o, &fitting_opts, exception);
  DEBUG(" [%u].\n", INDEX_LIST_LENGTH(job->corners[o]));
  if (job->is_corner != NULL) {
    job->is_corner[o] = g_new0(gboolean, O_LENGTH(pixel_o));
    for (i = 0; i < INDEX_LIST_LENGTH(job->corners[o]); i++)
      job->is_corner[o][GET_INDEX(job->corners[o], i)] = TRUE;
  }
}

/* Settle the corners of outline O with its neighbours, and split it into
   curves at them.  Uses the IS_CORNER of other outlines, so it runs only
   after find_own_corners() is done for all of them.  */
static void split_outline(unsigned o, at_exception_type *exception, gpointer data)
{
  split_job_type *job = data;
  pixel_outline_list_type pixel_list = job->pixel_list;
  shared_border_type *borders = job->borders;
  curve_type curve, first_curve;
  index_list_type corner_list;
  unsigned p, this_corner;
  curve_list_type curve_list = new_curve_list();
  pixel_outline_type pixel_o = O_LIST_OUTLINE(pixel_list, o);
  gboolean shared;

  (void)exception;
  if (borders != NULL)
    share_corners(pixel_list, borders, o, job->is_corner, job->corners);
  corner_list = job->corners[o];
  shared = borders != NULL && borders[o].shared;

  CURVE_LIST_CLOCKWISE(curve_list) = O_CLOCKWISE(pixel_o);
  curve_list.open = pixel_o.open;

  /* Remember the first curve so we can make it be the `next' of the
     last one.  (And vice versa.)  */
  first_curve = new_curve();

  curve = first_curve;

  if (INDEX_LIST_LENGTH(corner_list) == 0) {
    /* No corners.  Use all of the pixel outline as the curve.  */
    for (p = 0; p < O_LENGTH(pixel_o); p++)
      append_pixel(curve, O_COORDINATE(pixel_o, p));

    if (curve_list.open == TRUE)
      CURVE_CYCLIC(curve) = FALSE;
    else
      CURVE_CYCLIC(curve) = TRUE;
  } else { /* Each curve consists of the points between (inclusive) each pair
              of corners.  */
    for (this_corner = 0; this_corner < INDEX_LIST_LENGTH(corner_list) - 1; this_corner++) {
      curve_type previous_curve = curve;
      unsigned corner = GET_INDEX(corner_list, this_corner);
      unsigned next_corner = GET_INDEX(corner_list, this_corner + 1);

      for (p = corner; p <= next_corner; p++)
        append_pixel(curve, O_COORDINATE(pixel_o, p));

      append_curve(&curve_list, curve);
      if (shared)
        add_curve_border(pixel_list, borders, o, corner, next_corner);
      curve = new_curve();
      NEXT_CURVE(previous_curve) = curve;
      PREVIOUS_CURVE(curve) = previous_curve;
    }

    /* The last curve is different.  It consists of the points
       (inclusive) between the last corner and the end of the list,
       and the beginning of the list and the first corner.  */
    for (p = GET_LAST_INDEX(corner_list); p < O_LENGTH(pixel_o); p++)
      append_pixel(curve, O_COORDINATE(pixel_o, p));

    if (!pixel_o.open) {
      for (p = 0; p <= GET_INDEX(corner_list, 0); p++)
        append_pixel(curve, O_COORDINATE(pixel_o, p));
      if (shared)
        add_curve_border(pixel_list, borders, o, GET_LAST_INDEX(corner_list),
                         GET_INDEX(corner_list, 0));
    } else {
      curve_type last_curve = PREVIOUS_CURVE(curve);
      PREVIOUS_CURVE(first_curve) = NULL;
      if (last_curve)
        NEXT_CURVE(last_curve) = NULL;
    }
  }

  free_index_list(&job->corners[o]);

  /* Add `curve' to the end of the list, updating the pointers in
     the chain.  */
  append_curve(&curve_list, curve);
  NEXT_CURVE(curve) = first_curve;
  PREVIOUS_CURVE(first_curve) = curve;

  job->curve_lists[o] = curve_list;
}

static curve_list_array_type split_at_corners(pixel_outline_list_type pixel_list,
                                              fitting_opts_type *fitting_opts,
                                              shared_border_type *borders,
                                              at_exception_type *exception)
{
  unsigned o, n_outlines = O_LIST_LENGTH(pixel_list);
  curve_list_array_type curve_array = new_curve_list_array();
  split_job_type job;

  job.pixel_list = pixel_list;
  job.fitting_opts = fitting_opts;
  job.borders = borders;
  job.corners = g_new(index_list_type, n_outlines);
  job.is_corner = borders != NULL ? g_new(gboolean *, n_outlines) : NULL;
  job.curve_lists = g_new(curve_list_type, n_outlines);

  DEBUG("\nFinding corners:\n");
  for_each_outline(n_outlines, find_own_corners, &job, exception);
  for_each_outline(n_outlines, split_outline, &job, exception);

  for (o = 0; o < n_outlines; o++)
    append_curve_list(&curve_array, job.curve_lists[o]);

  if (job.is_corner != NULL) {
    for (o = 0; o < n_outlines; o++)
      g_free(job.is_corner[o]);
    g_free(job.is_corner);
  }
  g_free(job.curve_lists);
  g_free(job.corners);
  return curve_array;
}

/* We consider a point to be a corner if (1) the angle defined by the
   `corner_surround' points coming into it and going out from it is less
   than `corner_threshold' degrees, and no point within
   `corner_surround' points has a smaller angle; or (2) the angle is less
   than `corner_always_threshold' degrees.

   Because of the different cases, it is convenient to have the
   following macro to append a corner on to the list we return.  The
   character argument C is simply so that the different cases can be
   distinguished in the log file.  */

#define APPEND_CORNER(index, angle, c)                                                             \
  do {                                                                                             \
    append_index(&corner_list, index);                                                             \
    DEBUG(" (%d,%d)%c%.3f", O_COORDINATE(pixel_outline, index).x,                                  \
          O_COORDINATE(pixel_outline, index).y, c, angle);                                         \
  } while (0)

static index_list_type find_corners(pixel_outline_type pixel_outline,
                                    fitting_opts_type *fitting_opts, at_exception_type *exception)
{
  unsigned p, next_p, start_p, end_p;
  index_list_type corner_list = new_index_list();

  start_p = 0;
  end_p = O_LENGTH(pixel_outline) - 1;
  if (pixel_outline.open) {
    if (end_p <= fitting_opts->corner_surround * 2)
      return corner_list;
    APPEND_CORNER(0, 0.0, '@');
    start_p += fitting_opts->corner_surround;
    end_p -= fitting_opts->corner_surround;
  }

  /* Consider each pixel on the outline in turn.  */
  for (p = start_p; p <= end_p; p = next_p) {
    gfloat corner_angle;
    vector_type in_vector, out_vector;

    next_p = p + 1;

    /* Check if the angle is small enough.  */
    find_vectors(p, pixel_outline, &in_vector, &out_vector, fitting_opts->corner_surround);
    corner_angle = Vangle(in_vector, out_vector, exception);
    if (at_exception_got_fatal(exception))
      goto cleanup;

    if (fabs(corner_angle) <= fitting_opts->corner_threshold) {
      /* We want to keep looking, instead of just appending the
         first pixel we find with a small enough angle, since there
         might be another corner within `corner_surround' pixels, with
         a smaller angle.  If that is the case, we want that one.  */
      gfloat best_corner_angle = corner_angle;
      unsigned best_corner_index = p;
      index_list_type equally_good_list = new_index_list();
      /* As we come into the loop, `p' is the index of the point
         that has an angle less than `corner_angle'.  We use `i' to
         move through the pixels next to that, and `q' for moving
         through the adjacent pixels to each `p'.  */
      unsigned q = p;
      unsigned i = p + 1;

      while (TRUE) {
        /* Perhaps the angle is sufficiently small that we want to
           consider this a corner, even if it's not the best
           (unless we've already wrapped around in the search,
           i.e., `q<i', in which case we have already added the
           corner, and we don't want to add it again).  We want to
           do this check on the first candidate we find, as well
           as the others in the loop, hence this comes before the
           stopping condition.  */
        if (corner_angle <= fitting_opts->corner_always_threshold && q >= p)
          APPEND_CORNER(q, corner_angle, '\\');

        /* Exit the loop if we've looked at `corner_surround'
           pixels past the best one we found, or if we've looked
           at all the pixels.  */
        if (i >= best_corner_index + fitting_opts->corner_surround || i >= O_LENGTH(pixel_outline))
          break;

        /* Check the angle.  */
        q = i % O_LENGTH(pixel_outline);
        find_vectors(q, pixel_outline, &in_vector, &out_vector, fitting_opts->corner_surround);
        corner_angle = Vangle(in_vector, out_vector, exception);
        if (at_exception_got_fatal(exception))
          goto cleanup;

        /* If we come across a corner that is just as good as the
           best one, we should make it a corner, too.  This
           happens, for example, at the points on the `W' in some
           typefaces, where the ``points'' are flat.  */
        if (epsilon_equal(corner_angle, best_corner_angle))
          append_index(&equally_good_list, q);

        else if (corner_angle < best_corner_angle) {
          best_corner_angle = corner_angle;
          /* We want to check `corner_surround' pixels beyond the
             new best corner.  */
          i = best_corner_index = q;
          free_index_list(&equally_good_list);
          equally_good_list = new_index_list();
        }

        i++;
      }

      /* After we exit the loop, `q' is the index of the last point
         we checked.  We have already added the corner if
         `best_corner_angle' is less than `corner_always_threshold'.
         Again, if we've already wrapped around, we don't want to
         add the corner again.  */
      if (best_corner_angle > fitting_opts->corner_always_threshold && best_corner_index >= p) {
        unsigned j;

        APPEND_CORNER(best_corner_index, best_corner_angle, '/');

        for (j = 0; j < INDEX_LIST_LENGTH(equally_good_list); j++)
          APPEND_CORNER(GET_INDEX(equally_good_list, j), best_corner_angle, '@');
      }
      free_index_list(&equally_good_list);

      /* If we wrapped around in our search, we're done; otherwise,
         we don't want the outer loop to look at the pixels that we
         already looked at in searching for the best corner.  */
      next_p = ((q < p) ? O_LENGTH(pixel_outline) : q) + 1;
    } /* End of searching for the best corner.  */
  } /* End of considering each pixel.  */

  if (INDEX_LIST_LENGTH(corner_list) > 0)
    /* We never want two corners next to each other, since the
       only way to fit such a ``curve'' would be with a straight
       line, which usually interrupts the continuity dreadfully.  */
    remove_adjacent_corners(&corner_list, O_LENGTH(pixel_outline) - (pixel_outline.open ? 2 : 1),
                            fitting_opts->remove_adjacent_corners, exception);
cleanup:
  return corner_list;
}

/* Return the difference vectors coming in and going out of the outline
   OUTLINE at the point whose index is TEST_INDEX.  In Phoenix,
   Schneider looks at a single point on either side of the point we're
   considering.  That works for him because his points are not touching.
   But our points *are* touching, and so we have to look at
   `corner_surround' points on either side, to get a better picture of
   the outline's shape.  */

static void find_vectors(unsigned test_index, pixel_outline_type outline, vector_type *in,
                         vector_type *out, unsigned corner_surround)
{
  int i;
  unsigned n_done;
  at_coord candidate = O_COORDINATE(outline, test_index);

  in->dx = in->dy = in->dz = 0.0;
  out->dx = out->dy = out->dz = 0.0;

  /* Add up the differences from p of the `corner_surround' points
     before p.  */
  for (i = O_PREV(outline, test_index), n_done = 0; n_done < corner_surround;
       i = O_PREV(outline, i), n_done++)
    *in = Vadd(*in, IPsubtract(O_COORDINATE(outline, i), candidate));

  /* And the points after p.  */
  for (i = O_NEXT(outline, test_index), n_done = 0; n_done < corner_surround;
       i = O_NEXT(outline, i), n_done++)
    *out = Vadd(*out, IPsubtract(O_COORDINATE(outline, i), candidate));
}

/* Remove adjacent points from the index list LIST.  We do this by first
   sorting the list and then running through it.  Since these lists are
   quite short, a straight selection sort (e.g., p.139 of the Art of
   Computer Programming, vol.3) is good enough.  LAST_INDEX is the index
   of the last pixel on the outline, i.e., the next one is the first
   pixel. We need this for checking the adjacency of the last corner.

   We need to do this because the adjacent corners turn into
   two-pixel-long curves, which can only be fit by straight lines.  */

static void remove_adjacent_corners(index_list_type *list, unsigned last_index,
                                    gboolean remove_adj_corners, at_exception_type *exception)
{
  unsigned j, step = 1;
  unsigned last;
  index_list_type new_list = new_index_list();

  for (j = INDEX_LIST_LENGTH(*list) - 1; j > 0; j--) {
    unsigned search;
    unsigned temp;
    /* Find maximal element below `j'.  */
    unsigned max_index = j;

    for (search = 0; search < j; search++)
      if (GET_INDEX(*list, search) > GET_INDEX(*list, max_index))
        max_index = search;

    if (max_index != j) {
      temp = GET_INDEX(*list, j);
      GET_INDEX(*list, j) = GET_INDEX(*list, max_index);
      GET_INDEX(*list, max_index) = temp;

      /* xx -- really have to sort?  */
      DEBUG("needed exchange");
      at_exception_warning(exception, "needed exchange");
    }
  }

  /* The list is sorted.  Now look for adjacent entries.  Each time
     through the loop we insert the current entry and, if appropriate,
     the next entry.  */
  for (j = 0; j < INDEX_LIST_LENGTH(*list) - 1; j += step) {
    unsigned current = GET_INDEX(*list, j);
    unsigned next = GET_INDEX(*list, j + 1);

    /* We should never have inserted the same element twice.  */

    /* Skip the next entry when it is adjacent to this one.  */
    step = ((remove_adj_corners) && ((next == current + 1) || (next == current))) ? 2 : 1;

    append_index(&new_list, current);
  }

  /* Don't append the last element if it is 1) adjacent to the previous
     one; or 2) adjacent to the very first one.  */
  last = GET_LAST_INDEX(*list);
  if (INDEX_LIST_LENGTH(new_list) == 0 ||
      !(last == GET_LAST_INDEX(new_list) + 1 || (last == last_index && GET_INDEX(*list, 0) == 0)))
    append_index(&new_list, last);

  free_index_list(list);
  *list = new_list;
}

/* A ``knee'' is a point which forms a ``right angle'' with its
   predecessor and successor.  See the documentation (the `Removing
   knees' section) for an example and more details.

   The argument CLOCKWISE tells us which direction we're moving.  (We
   can't figure that information out from just the single segment with
   which we are given to work.)

   We should never find two consecutive knees.

   Since the first and last points are corners (unless the curve is
   cyclic), it doesn't make sense to remove those.  */

/* This evaluates to TRUE if the vector V is zero in one direction and
   nonzero in the other.  */
#define ONLY_ONE_ZERO(v) (((v).dx == 0.0 && (v).dy != 0.0) || ((v).dy == 0.0 && (v).dx != 0.0))

/* There are four possible cases for knees, one for each of the four
   corners of a rectangle; and then the cases differ depending on which
   direction we are going around the curve.  The tests are listed here
   in the order of upper left, upper right, lower right, lower left.
   Perhaps there is some simple pattern to the
   clockwise/counterclockwise differences, but I don't see one.  */
#define CLOCKWISE_KNEE(prev_delta, next_delta)                                                     \
  (((prev_delta).dx == -1.0 && (next_delta).dy == 1.0) ||                                          \
   ((prev_delta).dy == 1.0 && (next_delta).dx == 1.0) ||                                           \
   ((prev_delta).dx == 1.0 && (next_delta).dy == -1.0) ||                                          \
   ((prev_delta).dy == -1.0 && (next_delta).dx == -1.0))

#define COUNTERCLOCKWISE_KNEE(prev_delta, next_delta)                                              \
  (((prev_delta).dy == 1.0 && (next_delta).dx == -1.0) ||                                          \
   ((prev_delta).dx == 1.0 && (next_delta).dy == 1.0) ||                                           \
   ((prev_delta).dy == -1.0 && (next_delta).dx == 1.0) ||                                          \
   ((prev_delta).dx == -1.0 && (next_delta).dy == -1.0))

static void remove_knee_points(curve_type curve, gboolean clockwise)
{
  unsigned i;
  unsigned offset = (CURVE_CYCLIC(curve) == TRUE) ? 0 : 1;
  at_coord previous = real_to_int_coord(CURVE_POINT(curve, CURVE_PREV(curve, offset)));
  curve_type trimmed_curve = copy_most_of_curve(curve);

  if (CURVE_CYCLIC(curve) == FALSE)
    append_pixel(trimmed_curve, real_to_int_coord(CURVE_POINT(curve, 0)));

  for (i = offset; i < CURVE_LENGTH(curve) - offset; i++) {
    at_coord current = real_to_int_coord(CURVE_POINT(curve, i));
    at_coord next = real_to_int_coord(CURVE_POINT(curve, CURVE_NEXT(curve, i)));
    vector_type prev_delta = IPsubtract(previous, current);
    vector_type next_delta = IPsubtract(next, current);

    if (ONLY_ONE_ZERO(prev_delta) && ONLY_ONE_ZERO(next_delta) &&
        ((clockwise && CLOCKWISE_KNEE(prev_delta, next_delta)) ||
         (!clockwise && COUNTERCLOCKWISE_KNEE(prev_delta, next_delta))))
      DEBUG(" (%d,%d)", current.x, current.y);
    else {
      previous = current;
      append_pixel(trimmed_curve, current);
    }
  }

  if (CURVE_CYCLIC(curve) == FALSE)
    append_pixel(trimmed_curve, real_to_int_coord(LAST_CURVE_POINT(curve)));

  if (CURVE_LENGTH(trimmed_curve) == CURVE_LENGTH(curve))
    DEBUG(" (none)");

  DEBUG(".\n");

  free_curve(curve);
  *curve = *trimmed_curve;
  g_free(trimmed_curve); /* free_curve? --- Masatake */
}

/* Smooth the curve by adding in neighboring points.  Do this
   `filter_iterations' times.  But don't change the corners.  */

static void filter(curve_type curve, fitting_opts_type *fitting_opts)
{
  unsigned iteration, this_point;
  unsigned offset = (CURVE_CYCLIC(curve) == TRUE) ? 0 : 1;
  at_real_coord prev_new_point;

  /* We must have at least three points---the previous one, the current
     one, and the next one.  But if we don't have at least five, we will
     probably collapse the curve down onto a single point, which means
     we won't be able to fit it with a spline.  */
  if (CURVE_LENGTH(curve) < 5) {
    DEBUG("Length is %u, not enough to filter.\n", CURVE_LENGTH(curve));
    return;
  }

  prev_new_point.x = FLT_MAX;
  prev_new_point.y = FLT_MAX;
  prev_new_point.z = FLT_MAX;

  for (iteration = 0; iteration < fitting_opts->filter_iterations; iteration++) {
    curve_type newcurve = copy_most_of_curve(curve);
    gboolean collapsed = FALSE;

    /* Keep the first point on the curve.  */
    if (offset)
      append_point(newcurve, CURVE_POINT(curve, 0));

    for (this_point = offset; this_point < CURVE_LENGTH(curve) - offset; this_point++) {
      vector_type in, out, sum;
      at_real_coord new_point;

      /* Calculate the vectors in and out, computed by looking at n points
         on either side of this_point. Experimental it was found that 2 is
         optimal. */

      signed int prev, prevprev; /* have to be signed */
      unsigned int next, nextnext;
      at_real_coord candidate = CURVE_POINT(curve, this_point);

      prev = CURVE_PREV(curve, this_point);
      prevprev = CURVE_PREV(curve, prev);
      next = CURVE_NEXT(curve, this_point);
      nextnext = CURVE_NEXT(curve, next);

      /* Add up the differences from p of the `surround' points
         before p.  */
      in.dx = in.dy = in.dz = 0.0;

      in = Vadd(in, Psubtract(CURVE_POINT(curve, prev), candidate));
      if (prevprev >= 0)
        in = Vadd(in, Psubtract(CURVE_POINT(curve, prevprev), candidate));

      /* And the points after p.  Don't use more points after p than we
         ended up with before it.  */
      out.dx = out.dy = out.dz = 0.0;

      out = Vadd(out, Psubtract(CURVE_POINT(curve, next), candidate));
      if (nextnext < CURVE_LENGTH(curve))
        out = Vadd(out, Psubtract(CURVE_POINT(curve, nextnext), candidate));

      /* Start with the old point.  */
      new_point = candidate;
      sum = Vadd(in, out);
      /* We added 2*n+2 points, so we have to divide the sum by 2*n+2 */
      new_point.x += sum.dx / 6;
      new_point.y += sum.dy / 6;
      new_point.z += sum.dz / 6;
      if (fabs(prev_new_point.x - new_point.x) < 0.3 &&
          fabs(prev_new_point.y - new_point.y) < 0.3 &&
          fabs(prev_new_point.z - new_point.z) < 0.3) {
        collapsed = TRUE;
        break;
      }

      /* Put the newly computed point into a separate curve, so it
         doesn't affect future computation (on this iteration).  */
      append_point(newcurve, prev_new_point = new_point);
    }

    if (collapsed)
      free_curve(newcurve);
    else {
      /* Just as with the first point, we have to keep the last point.  */
      if (offset)
        append_point(newcurve, LAST_CURVE_POINT(curve));

      /* Set the original curve to the newly filtered one, and go again.  */
      free_curve(curve);
      *curve = *newcurve;
    }
    g_free(newcurve);
  }

  log_curve(curve, FALSE);
}

/* This routine returns the curve fitted to a straight line in a very
   simple way: make the first and last points on the curve be the
   endpoints of the line.  This simplicity is justified because we are
   called only on very short curves.  */

static spline_list_type *fit_with_line(curve_type curve)
{
  spline_type line;

  DEBUG("Fitting with straight line:\n");

  SPLINE_DEGREE(line) = LINEARTYPE;
  START_POINT(line) = CONTROL1(line) = CURVE_POINT(curve, 0);
  END_POINT(line) = CONTROL2(line) = LAST_CURVE_POINT(curve);

  /* Make sure that this line is never changed to a cubic.  */
  SPLINE_LINEARITY(line) = 0;

  DEBUG("  ");
  print_spline(line);

  return new_spline_list_with_spline(line);
}

/* The least squares method is well described in Schneider's thesis.
   Briefly, we try to fit the entire curve with one spline. If that
   fails, we subdivide the curve.  */

static spline_list_type *fit_with_least_squares(curve_type curve, fitting_opts_type *fitting_opts,
                                                at_exception_type *exception)
{
  gfloat error = 0, best_error = FLT_MAX;
  spline_type spline, best_spline;
  spline_list_type *spline_list = NULL;
  unsigned worst_point = 0;
  gfloat previous_error = FLT_MAX;
  gboolean perfect_fit;

  DEBUG("\nFitting with least squares:\n");

  /* Phoenix reduces the number of points with a ``linear spline
     technique''.  But for fitting letterforms, that is
     inappropriate.  We want all the points we can get.  */

  /* It makes no difference whether we first set the `t' values or
     find the tangents.  This order makes the documentation a little
     more coherent.  */

  DEBUG("Finding tangents:\n");
  find_tangent(curve, /* to_start */ TRUE, /* cross_curve */ FALSE, fitting_opts->tangent_surround);
  find_tangent(curve, /* to_start */ FALSE, /* cross_curve */ FALSE,
               fitting_opts->tangent_surround);

  set_initial_parameter_values(curve);

  /* Now we loop, subdividing, until CURVE has
     been fit.  */
  while (TRUE) {
    spline = best_spline = fit_one_spline(curve, exception);
    if (at_exception_got_fatal(exception))
      goto cleanup;

    if (SPLINE_DEGREE(spline) == LINEARTYPE)
      DEBUG("  fitted to line:\n");
    else
      DEBUG("  fitted to spline:\n");

    DEBUG("    ");
    print_spline(spline);

    if (SPLINE_DEGREE(spline) == LINEARTYPE)
      break;

    error = find_error(curve, spline, &worst_point, exception);
    if (error <= previous_error) {
      best_error = error;
      best_spline = spline;
    }
    break;
  }

  if (SPLINE_DEGREE(spline) == LINEARTYPE) {
    spline_list = new_spline_list_with_spline(spline);
    DEBUG("Accepted error of %.3f.\n", error);
    return (spline_list);
  }

  /* Go back to the best fit.  */
  spline = best_spline;
  error = best_error;

  /* The spline interpolates the curve's endpoints, so the error there is
     only floating-point noise.  If find_error() still reports an endpoint
     as the worst point (or found no worst point at all), the spline runs
     through every point and no subdivision can improve on it.  Splitting
     at an endpoint would hand the whole curve back to fit_curve() and
     recurse until the stack overflows, which is what an error threshold
     of 0 used to do (issue #247).  */
  perfect_fit = worst_point == 0 || worst_point >= CURVE_LENGTH(curve) - 1;

  if ((error < fitting_opts->error_threshold || perfect_fit) && CURVE_CYCLIC(curve) == FALSE) {
    /* The points were fitted with a
       spline.  We end up here whenever a fit is accepted.  We have
       one more job: see if the ``curve'' that was fit should really
       be a straight line. */
    if (spline_linear_enough(&spline, curve, fitting_opts)) {
      SPLINE_DEGREE(spline) = LINEARTYPE;
      DEBUG("Changed to line.\n");
    }
    spline_list = new_spline_list_with_spline(spline);
    DEBUG("Accepted error of %.3f.\n", error);
  } else {
    /* We couldn't fit the curve acceptably, so subdivide.  */
    unsigned subdivision_index;
    spline_list_type *left_spline_list;
    spline_list_type *right_spline_list;
    curve_type left_curve = new_curve();
    curve_type right_curve = new_curve();

    /* Keep the linked list of curves intact.  */
    NEXT_CURVE(right_curve) = NEXT_CURVE(curve);
    PREVIOUS_CURVE(right_curve) = left_curve;
    NEXT_CURVE(left_curve) = right_curve;
    PREVIOUS_CURVE(left_curve) = curve;
    NEXT_CURVE(curve) = left_curve;

    DEBUG("\nSubdividing (error %.3f):\n", error);
    DEBUG("  Original point: (%.3f,%.3f), #%u.\n", CURVE_POINT(curve, worst_point).x,
          CURVE_POINT(curve, worst_point).y, worst_point);
    /* A cyclic curve is always split at least once.  If it fits
       perfectly there is no worst point to split at, so split it in the
       middle; either way both halves are shorter than CURVE, which is
       what bounds the recursion.  */
    subdivision_index = perfect_fit ? CURVE_LENGTH(curve) / 2 : worst_point;
    DEBUG("  Final point: (%.3f,%.3f), #%u.\n", CURVE_POINT(curve, subdivision_index).x,
          CURVE_POINT(curve, subdivision_index).y, subdivision_index);

    /* The last point of the left-hand curve will also be the first
       point of the right-hand curve.  */
    CURVE_LENGTH(left_curve) = subdivision_index + 1;
    CURVE_LENGTH(right_curve) = CURVE_LENGTH(curve) - subdivision_index;
    left_curve->point_list = curve->point_list;
    right_curve->point_list = curve->point_list + subdivision_index;

    /* We want to use the tangents of the curve which we are
       subdividing for the start tangent for left_curve and the
       end tangent for right_curve.  */
    CURVE_START_TANGENT(left_curve) = CURVE_START_TANGENT(curve);
    CURVE_END_TANGENT(right_curve) = CURVE_END_TANGENT(curve);

    /* We have to set up the two curves before finding the tangent at
       the subdivision point.  The tangent at that point must be the
       same for both curves, or noticeable bumps will occur in the
       character.  But we want to use information on both sides of the
       point to compute the tangent, hence cross_curve = true.  */
    find_tangent(left_curve, /* to_start_point: */ FALSE,
                 /* cross_curve: */ TRUE, fitting_opts->tangent_surround);
    CURVE_START_TANGENT(right_curve) = CURVE_END_TANGENT(left_curve);

    /* Now that we've set up the curves, we can fit them.  */
    left_spline_list = fit_curve(left_curve, fitting_opts, exception);
    if (at_exception_got_fatal(exception))
      /* TODO: Memory allocated for left_curve and right_curve
         will leak. */
      goto cleanup;

    right_spline_list = fit_curve(right_curve, fitting_opts, exception);
    /* TODO: Memory allocated for left_curve and right_curve
       will leak. */
    if (at_exception_got_fatal(exception))
      goto cleanup;

    /* Neither of the subdivided curves could be fit, so fail.  */
    if (left_spline_list == NULL && right_spline_list == NULL)
      return NULL;

    /* Put the two together (or whichever of them exist).  */
    spline_list = new_spline_list();

    if (left_spline_list == NULL) {
      DEBUG("Could not fit spline to left curve (%lx).\n", (unsigned long)(uintptr_t)left_curve);
      at_exception_warning(exception, "Could not fit left spline list");
    } else {
      concat_spline_lists(spline_list, *left_spline_list);
      free_spline_list(*left_spline_list);
      g_free(left_spline_list);
    }

    if (right_spline_list == NULL) {
      DEBUG("Could not fit spline to right curve (%lx).\n", (unsigned long)(uintptr_t)right_curve);
      at_exception_warning(exception, "Could not fit right spline list");
    } else {
      concat_spline_lists(spline_list, *right_spline_list);
      free_spline_list(*right_spline_list);
      g_free(right_spline_list);
    }
    if (CURVE_END_TANGENT(left_curve))
      g_free(CURVE_END_TANGENT(left_curve));
    g_free(left_curve);
    g_free(right_curve);
  }
cleanup:
  return spline_list;
}

/* Our job here is to find alpha1 (and alpha2), where t1_hat (t2_hat) is
   the tangent to CURVE at the starting (ending) point, such that:

   control1 = alpha1*t1_hat + starting point
   control2 = alpha2*t2_hat + ending_point

   and the resulting spline (starting_point .. control1 and control2 ..
   ending_point) minimizes the least-square error from CURVE.

   See pp.57--59 of the Phoenix thesis.

   The B?(t) here corresponds to B_i^3(U_i) there.
   The Bernshte\u in polynomials of degree n are defined by
   B_i^n(t) = { n \choose i } t^i (1-t)^{n-i}, i = 0..n  */

#define B0(t) CUBE((gfloat)1.0 - (t))
#define B1(t) ((gfloat)3.0 * (t) * SQUARE((gfloat)1.0 - (t)))
#define B2(t) ((gfloat)3.0 * SQUARE(t) * ((gfloat)1.0 - (t)))
#define B3(t) CUBE(t)

static spline_type fit_one_spline(curve_type curve, at_exception_type *exception)
{
  /* Since our arrays are zero-based, the `C0' and `C1' here correspond
     to `C1' and `C2' in the paper.  */
  gfloat X_C1_det, C0_X_det, C0_C1_det;
  gfloat alpha1, alpha2;
  spline_type spline = {0};
  vector_type start_vector, end_vector;
  unsigned i;
  g_autofree vector_type *A = NULL;
  vector_type t1_hat = *CURVE_START_TANGENT(curve);
  vector_type t2_hat = *CURVE_END_TANGENT(curve);
  gfloat C[2][2] = {{0.0, 0.0}, {0.0, 0.0}};
  gfloat X[2] = {0.0, 0.0};

  A = g_new(vector_type, CURVE_LENGTH(curve) * 2); /* A dynamically allocated array. */

  START_POINT(spline) = CURVE_POINT(curve, 0);
  END_POINT(spline) = LAST_CURVE_POINT(curve);
  start_vector = make_vector(START_POINT(spline));
  end_vector = make_vector(END_POINT(spline));

  for (i = 0; i < CURVE_LENGTH(curve); i++) {
    A[(i << 1) + 0] = Vmult_scalar(t1_hat, B1(CURVE_T(curve, i)));
    A[(i << 1) + 1] = Vmult_scalar(t2_hat, B2(CURVE_T(curve, i)));
  }

  for (i = 0; i < CURVE_LENGTH(curve); i++) {
    vector_type temp, temp0, temp1, temp2, temp3;
    vector_type *Ai = A + (i << 1);

    C[0][0] += Vdot(Ai[0], Ai[0]);
    C[0][1] += Vdot(Ai[0], Ai[1]);
    /* C[1][0] = C[0][1] (this is assigned outside the loop)  */
    C[1][1] += Vdot(Ai[1], Ai[1]);

    /* Now the right-hand side of the equation in the paper.  */
    temp0 = Vmult_scalar(start_vector, B0(CURVE_T(curve, i)));
    temp1 = Vmult_scalar(start_vector, B1(CURVE_T(curve, i)));
    temp2 = Vmult_scalar(end_vector, B2(CURVE_T(curve, i)));
    temp3 = Vmult_scalar(end_vector, B3(CURVE_T(curve, i)));

    temp = make_vector(
        Vsubtract_point(CURVE_POINT(curve, i), Vadd(temp0, Vadd(temp1, Vadd(temp2, temp3)))));

    X[0] += Vdot(temp, Ai[0]);
    X[1] += Vdot(temp, Ai[1]);
  }

  C[1][0] = C[0][1];

  X_C1_det = X[0] * C[1][1] - X[1] * C[0][1];
  C0_X_det = C[0][0] * X[1] - C[0][1] * X[0];
  C0_C1_det = C[0][0] * C[1][1] - C[1][0] * C[0][1];
  if (C0_C1_det == 0.0) {
    /* Zero determinant */
    alpha1 = 0;
    alpha2 = 0;
  } else {
    alpha1 = X_C1_det / C0_C1_det;
    alpha2 = C0_X_det / C0_C1_det;
  }
  CONTROL1(spline) = Vadd_point(START_POINT(spline), Vmult_scalar(t1_hat, alpha1));
  CONTROL2(spline) = Vadd_point(END_POINT(spline), Vmult_scalar(t2_hat, alpha2));
  SPLINE_DEGREE(spline) = CUBICTYPE;

  return spline;
}

/* Find reasonable values for t for each point on CURVE.  The method is
   called chord-length parameterization, which is described in Plass &
   Stone.  The basic idea is just to use the distance from one point to
   the next as the t value, normalized to produce values that increase
   from zero for the first point to one for the last point.  */

static void set_initial_parameter_values(curve_type curve)
{
  unsigned p;

  DEBUG("\nAssigning initial t values:\n  ");

  CURVE_T(curve, 0) = 0.0;

  for (p = 1; p < CURVE_LENGTH(curve); p++) {
    at_real_coord point = CURVE_POINT(curve, p), previous_p = CURVE_POINT(curve, p - 1);
    gfloat d = distance(point, previous_p);
    CURVE_T(curve, p) = CURVE_T(curve, p - 1) + d;
  }

  if (LAST_CURVE_T(curve) == 0.0)
    LAST_CURVE_T(curve) = 1.0;

  for (p = 1; p < CURVE_LENGTH(curve); p++)
    CURVE_T(curve, p) = CURVE_T(curve, p) / LAST_CURVE_T(curve);

  log_entire_curve(curve);
}

/* Find an approximation to the tangent to an endpoint of CURVE (to the
   first point if TO_START_POINT is TRUE, else the last).  If
   CROSS_CURVE is TRUE, consider points on the adjacent curve to CURVE.

   It is important to compute an accurate approximation, because the
   control points that we eventually decide upon to fit the curve will
   be placed on the half-lines defined by the tangents and
   endpoints...and we never recompute the tangent after this.  */

static void find_tangent(curve_type curve, gboolean to_start_point, gboolean cross_curve,
                         unsigned tangent_surround)
{
  vector_type tangent;
  vector_type **curve_tangent =
      (to_start_point == TRUE) ? &(CURVE_START_TANGENT(curve)) : &(CURVE_END_TANGENT(curve));
  unsigned n_points = 0;

  DEBUG("  tangent to %s: ", (to_start_point == TRUE) ? "start" : "end");

  if (*curve_tangent == NULL) {
    *curve_tangent = g_new(vector_type, 1);
    do {
      tangent = find_half_tangent(curve, to_start_point, &n_points, tangent_surround);

      if ((cross_curve == TRUE) || (CURVE_CYCLIC(curve) == TRUE)) {
        curve_type adjacent_curve =
            (to_start_point == TRUE) ? PREVIOUS_CURVE(curve) : NEXT_CURVE(curve);
        vector_type tangent2 = find_half_tangent(adjacent_curve, TRUE, &n_points, tangent_surround);

        DEBUG("(adjacent curve half tangent (%.3f,%.3f,%.3f)) ", tangent2.dx, tangent2.dy,
              tangent2.dz);
        tangent = Vadd(tangent, tangent2);
      }
      tangent_surround--;

    } while (tangent.dx == 0.0 && tangent.dy == 0.0);

    assert(n_points > 0);
    **curve_tangent = Vmult_scalar(tangent, (gfloat)(1.0 / n_points));
    if ((CURVE_CYCLIC(curve) == TRUE) && CURVE_START_TANGENT(curve))
      *CURVE_START_TANGENT(curve) = **curve_tangent;
    if ((CURVE_CYCLIC(curve) == TRUE) && CURVE_END_TANGENT(curve))
      *CURVE_END_TANGENT(curve) = **curve_tangent;
  } else
    DEBUG("(already computed) ");

  DEBUG("(%.3f,%.3f,%.3f).\n", (*curve_tangent)->dx, (*curve_tangent)->dy, (*curve_tangent)->dz);
}

/* Find the change in y and change in x for `tangent_surround' (a global)
   points along CURVE.  Increment N_POINTS by the number of points we
   actually look at.  */

static vector_type find_half_tangent(curve_type c, gboolean to_start_point, unsigned *n_points,
                                     unsigned tangent_surround)
{
  unsigned p;
  int factor = to_start_point ? 1 : -1;
  unsigned tangent_index = to_start_point ? 0 : c->length - 1;
  at_real_coord tangent_point = CURVE_POINT(c, tangent_index);
  vector_type tangent = {0.0, 0.0, 0.0};
  unsigned int surround;

  if ((surround = CURVE_LENGTH(c) / 2) > tangent_surround)
    surround = tangent_surround;

  for (p = 1; p <= surround; p++) {
    int this_index = p * factor + tangent_index;
    at_real_coord this_point;

    if (this_index < 0 || this_index >= (int)c->length)
      break;

    this_point = CURVE_POINT(c, p * factor + tangent_index);

    /* Perhaps we should weight the tangent from `this_point' by some
       factor dependent on the distance from the tangent point.  */
    tangent = Vadd(tangent, Vmult_scalar(Psubtract(this_point, tangent_point), (gfloat)factor));
    (*n_points)++;
  }

  return tangent;
}

/* When this routine is called, we have computed a spline representation
   for the digitized curve.  The question is, how good is it?  If the
   fit is very good indeed, we might have an error of zero on each
   point, and then WORST_POINT becomes irrelevant.  But normally, we
   return the error at the worst point, and the index of that point in
   WORST_POINT.  The error computation itself is the Euclidean distance
   from the original curve CURVE to the fitted spline SPLINE.  */

static gfloat find_error(curve_type curve, spline_type spline, unsigned *worst_point,
                         at_exception_type *exception)
{
  unsigned this_point;
  gfloat total_error = 0.0;
  gfloat worst_error = FLT_MIN;

  *worst_point = CURVE_LENGTH(curve) + 1; /* A sentinel value.  */

  for (this_point = 0; this_point < CURVE_LENGTH(curve); this_point++) {
    at_real_coord curve_point = CURVE_POINT(curve, this_point);
    gfloat t = CURVE_T(curve, this_point);
    at_real_coord spline_point = evaluate_spline(spline, t);
    gfloat this_error = distance(curve_point, spline_point);
    if (this_error >= worst_error) {
      *worst_point = this_point;
      worst_error = this_error;
    }
    total_error += this_error;
  }

  if (*worst_point ==
      CURVE_LENGTH(curve) + 1) { /* Didn't have any ``worst point''; the error should be zero.  */
    if (epsilon_equal(total_error, 0.0))
      DEBUG("  Every point fit perfectly.\n");
    else {
      DEBUG("No worst point found; something is wrong");
      at_exception_warning(exception, "No worst point found; something is wrong");
    }
  } else {
    if (epsilon_equal(total_error, 0.0))
      DEBUG("  Every point fit perfectly.\n");
    else {
      DEBUG("  Worst error (at (%.3f,%.3f,%.3f), point #%u) was %.3f.\n",
            CURVE_POINT(curve, *worst_point).x, CURVE_POINT(curve, *worst_point).y,
            CURVE_POINT(curve, *worst_point).z, *worst_point, worst_error);
      DEBUG("  Total error was %.3f.\n", total_error);
      DEBUG("  Average error (over %u points) was %.3f.\n", CURVE_LENGTH(curve),
            total_error / CURVE_LENGTH(curve));
    }
  }

  return worst_error;
}

/* Supposing that we have accepted the error, another question arises:
   would we be better off just using a straight line?  */

static gboolean spline_linear_enough(spline_type *spline, curve_type curve,
                                     fitting_opts_type *fitting_opts)
{
  gfloat A, B, C;
  unsigned this_point;
  gfloat dist = 0.0, start_end_dist, threshold;

  DEBUG("Checking linearity:\n");

  A = END_POINT(*spline).x - START_POINT(*spline).x;
  B = END_POINT(*spline).y - START_POINT(*spline).y;
  C = END_POINT(*spline).z - START_POINT(*spline).z;

  start_end_dist = SQUARE(A) + SQUARE(B) + SQUARE(C);
  DEBUG("start_end_distance is %.3f.\n", sqrt(start_end_dist));

  DEBUG("  Line endpoints are (%.3f, %.3f, %.3f) and ", START_POINT(*spline).x,
        START_POINT(*spline).y, START_POINT(*spline).z);
  DEBUG("(%.3f, %.3f, %.3f)\n", END_POINT(*spline).x, END_POINT(*spline).y, END_POINT(*spline).z);

  for (this_point = 0; this_point < CURVE_LENGTH(curve); this_point++) {
    gfloat a, b, c, w;
    gfloat t = CURVE_T(curve, this_point);
    at_real_coord spline_point = evaluate_spline(*spline, t);

    a = spline_point.x - START_POINT(*spline).x;
    b = spline_point.y - START_POINT(*spline).y;
    c = spline_point.z - START_POINT(*spline).z;
    w = (A * a + B * b + C * c) / start_end_dist;

    dist += (gfloat)sqrt(SQUARE(a - A * w) + SQUARE(b - B * w) + SQUARE(c - C * w));
  }
  DEBUG("  Total distance is %.3f, ", dist);

  dist /= (CURVE_LENGTH(curve) - 1);
  DEBUG("which is %.3f normalized.\n", dist);

  /* We want reversion of short curves to splines to be more likely than
     reversion of long curves, hence the second division by the curve
     length, for use in `change_bad_lines'.  */
  SPLINE_LINEARITY(*spline) = dist;
  DEBUG("  Final linearity: %.3f.\n", SPLINE_LINEARITY(*spline));
  if (start_end_dist * (gfloat)0.5 > fitting_opts->line_threshold)
    threshold = fitting_opts->line_threshold;
  else
    threshold = start_end_dist * (gfloat)0.5;
  DEBUG("threshold is %.3f .\n", threshold);
  if (dist < threshold)
    return TRUE;
  else
    return FALSE;
}

/* Unfortunately, we cannot tell in isolation whether a given spline
   should be changed to a line or not.  That can only be known after the
   entire curve has been fit to a list of splines.  (The curve is the
   pixel outline between two corners.)  After subdividing the curve, a
   line may very well fit a portion of the curve just as well as the
   spline---but unless a spline is truly close to being a line, it
   should not be combined with other lines.  */

static void change_bad_lines(spline_list_type *spline_list, fitting_opts_type *fitting_opts)
{
  unsigned this_spline;
  gboolean found_cubic = FALSE;
  unsigned length = SPLINE_LIST_LENGTH(*spline_list);

  DEBUG("\nChecking for bad lines (length %u):\n", length);

  /* First see if there are any splines in the fitted shape.  */
  for (this_spline = 0; this_spline < length; this_spline++) {
    if (SPLINE_DEGREE(SPLINE_LIST_ELT(*spline_list, this_spline)) == CUBICTYPE) {
      found_cubic = TRUE;
      break;
    }
  }

  /* If so, change lines back to splines (we haven't done anything to
     their control points, so we only have to change the degree) unless
     the spline is close enough to being a line.  */
  if (found_cubic)
    for (this_spline = 0; this_spline < length; this_spline++) {
      spline_type s = SPLINE_LIST_ELT(*spline_list, this_spline);

      if (SPLINE_DEGREE(s) == LINEARTYPE) {
        DEBUG("  #%u: ", this_spline);
        if (SPLINE_LINEARITY(s) > fitting_opts->line_reversion_threshold) {
          DEBUG("reverted, ");
          SPLINE_DEGREE(SPLINE_LIST_ELT(*spline_list, this_spline)) = CUBICTYPE;
        }
        DEBUG("linearity %.3f.\n", SPLINE_LINEARITY(s));
      }
    }
  else
    DEBUG("  No lines.\n");
}

/* Lists of array indices (well, that is what we use it for).  */

static index_list_type new_index_list(void)
{
  index_list_type index_list;

  index_list.data = g_array_new(FALSE, FALSE, sizeof(unsigned));

  return index_list;
}

static void free_index_list(index_list_type *index_list)
{
  if (index_list->data)
    g_array_free(index_list->data, TRUE);
  index_list->data = NULL;
}

static void append_index(index_list_type *list, unsigned new_index)
{
  g_array_append_val(list->data, new_index);
}

/* Turn an real point into a integer one.  */

static at_coord real_to_int_coord(at_real_coord real_coord)
{
  at_coord int_coord;

  int_coord.x = lround(real_coord.x);
  int_coord.y = lround(real_coord.y);

  return int_coord;
}

/* Return the Euclidean distance between P1 and P2.  */

static gfloat distance(at_real_coord p1, at_real_coord p2)
{
  gfloat x = p1.x - p2.x, y = p1.y - p2.y, z = p1.z - p2.z;
  return (gfloat)sqrt(SQUARE(x) + SQUARE(y) + SQUARE(z));
}
