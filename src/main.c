/* main.c: main driver for autotrace -- convert bitmaps to splines. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "autotrace.h"
#include "logreport.h"
#include "cmdline.h"
#include "getopt.h"
#include "filename.h"
#include "atou.h"
#include "input.h"

#include <string.h>
#include <assert.h>
#include <math.h>
#include <errno.h>

#undef N_
#include "intl.h"
#ifdef ENABLE_NLS
#include <locale.h>
#endif
#include <glib.h>

/* Pointers to functions based on input format.  (-input-format)  */
static at_bitmap_reader *input_reader = NULL;

/* The name of the file we're going to write.  (-output-file) */

static char *output_name = (char *)"";

/* The output function. (-output-format) */
static at_spline_writer *output_writer = NULL;

/* Whether to print version information */
static gboolean printed_version;

/* Whether to dump a bitmap file */
static gboolean dumping_bitmap = FALSE;

/* Report tracing status in real time (--report-progress) */
static gboolean report_progress = FALSE;
#define dot_printer_max_column 50
#define dot_printer_char '|'
static void dot_printer(gfloat percentage, gpointer client_data);

static char *read_command_line(int, char *[], at_fitting_opts_type *, at_input_opts_type *, at_output_opts_type *);

static void dump(at_bitmap * bitmap, FILE * fp);

static void input_list_formats(FILE * file);
static void output_list_formats(FILE * file);

static void exception_handler(const gchar * msg, at_msg_type type, gpointer data);

#define DEFAULT_FORMAT "eps"

int main(int argc, char *argv[])
{
  at_fitting_opts_type *fitting_opts;
  at_input_opts_type *input_opts;
  at_output_opts_type *output_opts;
  char *input_name;
  at_splines_type *splines;
  at_bitmap *bitmap;
  FILE *output_file;
  FILE *dump_file;

  at_progress_func progress_reporter = NULL;
  int progress_stat = 0;

  autotrace_init();

#ifdef ENABLE_NLS
  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);
#endif /* Def: ENABLE_NLS */

  fitting_opts = at_fitting_opts_new();
  input_opts = at_input_opts_new();
  output_opts = at_output_opts_new();

  input_name = read_command_line(argc, argv, fitting_opts, input_opts, output_opts);

  if (output_name != NULL && input_name != NULL && 0 == strcasecmp(output_name, input_name))
    FATAL(_("Input and output file may not be the same\n"));

  /* Set input_reader if it is not set in command line args */
  if (!input_reader)
    input_reader = at_input_get_handler(input_name);

  /* Set output_writer if it is not set in command line args
     Step1. Guess from a file name.
     Step2. Use default. */
  if (!output_writer)
    output_writer = at_output_get_handler(output_name);
  if (!output_writer) {
    output_writer = at_output_get_handler_by_suffix(DEFAULT_FORMAT);
    if (output_writer == NULL)
      FATAL(_("Default format %s is not supported"), DEFAULT_FORMAT);
  }

  /* Open output file */
  if (!strcmp(output_name, ""))
    output_file = stdout;
  else {
    output_file = fopen(output_name, "wb");
    if (output_file == NULL) {
      perror(output_name);
      exit(errno);
    }
  }

  /* Open the main input file.  */
  if (input_reader != NULL) {
    bitmap = at_bitmap_read(input_reader, input_name, input_opts, exception_handler, NULL);

    at_input_opts_free(input_opts);
  } else
    FATAL(_("Unsupported input format"));

  if (report_progress) {
    progress_reporter = dot_printer;
    fprintf(stderr, "%-15s", input_name);
  };

  splines = at_splines_new_full(bitmap, fitting_opts, exception_handler, NULL, progress_reporter, &progress_stat, NULL, NULL);

  /* Dump loaded bitmap if needed */
  if (dumping_bitmap) {
    char *dumpfile_name = NULL;
    char *input_rootname = NULL;
    char *basename = g_path_get_basename(input_name);
    if ((input_rootname = remove_suffix(basename)) == NULL)
      FATAL(_("Not a valid input file name %s"), input_name);

    g_free(basename); // No longer needed. And we don't need to free dumpfile_name - it's just a pointer to bytes in basename.
    if (at_bitmap_get_planes(bitmap) == 1)
      dumpfile_name = g_strconcat(input_rootname, ".dump.pgm", NULL);
    else
      dumpfile_name = g_strconcat(input_rootname, ".dump.ppm", NULL);
    dump_file = fopen(dumpfile_name, "wb");
    if (dump_file == NULL) {
      perror(dumpfile_name);
      g_free(dumpfile_name);
      exit(errno);
    }
    g_free(dumpfile_name); // No longer needed
    if (at_bitmap_get_planes(bitmap) == 1)
      fprintf(dump_file, "%s\n", "P5");
    else
      fprintf(dump_file, "%s\n", "P6");
    fprintf(dump_file, "%s\n", "# Created by AutoTrace");
    fprintf(dump_file, "%d %d\n", at_bitmap_get_width(bitmap), at_bitmap_get_height(bitmap));
    fprintf(dump_file, "%d\n", 255);
    dump(bitmap, dump_file);
    fclose(dump_file);
  }

  at_splines_write(output_writer, output_file, output_name, output_opts, splines, exception_handler, NULL);
  at_output_opts_free(output_opts);

  if (output_file != stdout)
    fclose(output_file);

  at_splines_free(splines);
  at_bitmap_free(bitmap);
  at_fitting_opts_free(fitting_opts);

  if (report_progress)
    fputs("\n", stderr);

  return 0;
}

/* Reading the options.  */

#define USAGE1 "Options:\
<input_name> must be a supported image file.\n"\
  GETOPT_USAGE								\
"\n\
-background-color <hexadecimal>: the color of the background that should\n\
    be ignored, for example FFFFFF; default is no background color.\n\n\
-centerline: trace a character's centerline, rather than its outline.\n\n\
-charcode <unsigned>: code of character to load from GF font file.\n\n\
-color-count <unsigned>: number of colors a color bitmap is reduced to,\n\
    it does not work on grayscale, allowed are 1..256;\n\
    default is 0, that means no color reduction is done.\n\n\
-corner-always-threshold <angle-in-degrees>: if the angle at a pixel is\n\
    less than this, it is considered a corner, even if it is within\n\
    `corner-surround' pixels of another corner; default is 60.\n\n\
-corner-surround <unsigned>: number of pixels on either side of a\n\
    point to consider when determining if that point is a corner;\n\
    default is 4.\n\n\
-corner-threshold <angle-in-degrees>: if a pixel, its predecessor(s),\n\
    and its successor(s) meet at an angle smaller than this, it's a\n\
    corner; default is 100.\n\n\
-despeckle-level <unsigned>: 0..20; default is 0: no despeckling.\n\n\
-despeckle-tightness <real>: 0.0..8.0; default is 2.0.\n\n\
-dpi <unsigned>: The dots per inch value in the input image, affects scaling\n\
    of mif output image\n\n"
#define USAGE2 "\n\
-error-threshold <real>: subdivide fitted curves that are off by\n\
    more pixels than this; default is 2.0.\n\n\
-filter-iterations <unsigned>: smooth the curve this many times\n\
    before fitting; default is 4.\n\n\
-input-format: Available formats: %s.\n\n\
-help: print this message.\n\n\
-line-reversion-threshold <real>: if a spline is closer to a straight\n\
    line than this, weighted by the square of the curve length, keep it a\n\
    straight line even if it is a list with curves; default is .01.\n\n\
-line-threshold <real>: if the spline is not more than this far away\n\
    from the straight line defined by its endpoints,\n\
    then output a straight line; default is 1.\n\n\
-list-output-formats: print a list of supported output formats to stderr.\n\n\
-list-input-formats: print a list of supported input formats to stderr.\n\n\
-log: write detailed progress reports to <input_name>.log.\n\n\
-noise-removal <real>:: 0.0..1.0; default is 0.99.\n\n\
-output-file <filename>: write to <filename>\n\n\
-output-format <format>: use format <format> for the output file. Available formats:\n\
    %s\n\n\
-preserve-width: preserve line width prior to thinning.\n\n\
-remove-adjacent-corners: remove corners that are adjacent.\n\n\
-tangent-surround <unsigned>: number of points on either side of a\n\
    point to consider when computing the tangent at that point; default is 3.\n\n\
-report-progress: report tracing status in real time.\n\n\
-debug-arch: print the type of cpu.\n\n\
-debug-bitmap: dump loaded bitmap to <input_name>.bitmap.ppm or pgm.\n\n\
-version: print the version number of this program.\n\n\
-width-weight-factor <real>: weight factor for fitting the linewidth.\n\n\
"

/* We return the name of the image to process.  */

static char *read_command_line(int argc, char *argv[], at_fitting_opts_type * fitting_opts, at_input_opts_type * input_opts, at_output_opts_type * output_opts)
{
  int g;                        /* `getopt' return code.  */
  int option_index;
  struct option long_options[] = {
	  {"background-color", 1, 0, 0},
	  {"debug-arch", 0, 0, 0},
	  {"debug-bitmap", 0, (int *)&dumping_bitmap, 1},
	  {"centerline", 0, 0, 0},
	  {"charcode", 1, 0, 0},
	  {"color-count", 1, 0, 0},
	  {"corner-always-threshold", 1, 0, 0},
	  {"corner-surround", 1, 0, 0},
	  {"corner-threshold", 1, 0, 0},
	  {"despeckle-level", 1, 0, 0},
	  {"despeckle-tightness", 1, 0, 0},
	  {"dpi", 1, 0, 0},
	  {"error-threshold", 1, 0, 0},
	  {"filter-iterations", 1, 0, 0},
	  {"help", 0, 0, 0},
	  {"input-format", 1, 0, 0},
	  {"line-reversion-threshold", 1, 0, 0},
	  {"line-threshold", 1, 0, 0},
	  {"list-output-formats", 0, 0, 0},
	  {"list-input-formats", 0, 0, 0},
	  {"log", 0, (int *)&logging, 1},
	  {"noise-removal", 1, 0, 0},
	  {"output-file", 1, 0, 0},
	  {"output-format", 1, 0, 0},
	  {"preserve-width", 0, 0, 0},
	  {"remove-adjacent-corners", 0, 0, 0},
	  {"tangent-surround", 1, 0, 0},
	  {"report-progress", 0, (int *)&report_progress, 1},
	  {"version", 0, (int *)&printed_version, 1},
	  {"width-weight-factor", 1, 0, 0},
	  {0, 0, 0, 0}
  };

  while (TRUE) {

    g = getopt_long_only(argc, argv, "", long_options, &option_index);

    if (g == EOF)
      break;

    if (g == '?')
      exit(1);                  /* Unknown option.  */

    assert(g == 0);             /* We have no short option names.  */

    if (ARGUMENT_IS("background-color")) {
      fitting_opts->background_color = at_color_parse(optarg, NULL);
      input_opts->background_color = at_color_copy(fitting_opts->background_color);
    } else if (ARGUMENT_IS("centerline"))
      fitting_opts->centerline = TRUE;

    else if (ARGUMENT_IS("charcode")) {
      fitting_opts->charcode = strtoul(optarg, 0, 0);
      input_opts->charcode = fitting_opts->charcode;
    }

    else if (ARGUMENT_IS("color-count"))
      fitting_opts->color_count = atou(optarg);

    else if (ARGUMENT_IS("corner-always-threshold"))
      fitting_opts->corner_always_threshold = (gfloat) atof(optarg);

    else if (ARGUMENT_IS("corner-surround"))
      fitting_opts->corner_surround = atou(optarg);

    else if (ARGUMENT_IS("corner-threshold"))
      fitting_opts->corner_threshold = (gfloat) atof(optarg);

    else if (ARGUMENT_IS("debug-arch")) {
      int endian = 1;
      char *str;
      if (*(char *)&endian)
        str = "little";
      else
        str = "big";

      printf("%lu bit, %s endian\n", sizeof(void *) * 8, str);
      exit(0);
    }

    else if (ARGUMENT_IS("despeckle-level"))
      fitting_opts->despeckle_level = atou(optarg);

    else if (ARGUMENT_IS("despeckle-tightness"))
      fitting_opts->despeckle_tightness = (gfloat) atof(optarg);

    else if (ARGUMENT_IS("noise-removal"))
      fitting_opts->noise_removal = (gfloat) atof(optarg);

    else if (ARGUMENT_IS("dpi"))
      output_opts->dpi = atou(optarg);

    else if (ARGUMENT_IS("error-threshold"))
      fitting_opts->error_threshold = (gfloat) atof(optarg);

    else if (ARGUMENT_IS("filter-iterations"))
      fitting_opts->filter_iterations = atou(optarg);

    else if (ARGUMENT_IS("help")) {
      char *ishortlist, *oshortlist;
      fprintf(stderr, _("Usage: %s [options] <input_file_name>\n"), argv[0]);
      fprintf(stderr, USAGE1);
      fprintf(stderr, USAGE2, ishortlist = at_input_shortlist(), oshortlist = at_output_shortlist());
      free(ishortlist);
      free(oshortlist);
      fprintf(stderr, _("\nYou can get the source code of autotrace from \n%s\n"), at_home_site());
      exit(0);
    }

    else if (ARGUMENT_IS("input-format")) {
      input_reader = at_input_get_handler_by_suffix(optarg);
      if (!input_reader)
        FATAL(_("Input format %s is not supported\n"), optarg);
    }

    else if (ARGUMENT_IS("line-threshold"))
      fitting_opts->line_threshold = (gfloat) atof(optarg);

    else if (ARGUMENT_IS("line-reversion-threshold"))
      fitting_opts->line_reversion_threshold = (gfloat) atof(optarg);

    else if (ARGUMENT_IS("list-output-formats")) {
      fprintf(stderr, _("Supported output formats:\n"));
      output_list_formats(stderr);
      exit(0);
    } else if (ARGUMENT_IS("list-input-formats")) {
      fprintf(stderr, _("Supported input formats:\n"));
      input_list_formats(stderr);
      exit(0);
    }

    else if (ARGUMENT_IS("output-file"))
      output_name = optarg;

    else if (ARGUMENT_IS("output-format")) {
      output_writer = at_output_get_handler_by_suffix(optarg);
      if (output_writer == NULL)
        FATAL(_("Output format %s is not supported"), optarg);
    } else if (ARGUMENT_IS("preserve-width"))
      fitting_opts->preserve_width = TRUE;

    else if (ARGUMENT_IS("remove-adjacent-corners"))
      fitting_opts->remove_adjacent_corners = TRUE;

    else if (ARGUMENT_IS("tangent-surround"))
      fitting_opts->tangent_surround = atou(optarg);

    else if (ARGUMENT_IS("version"))
      printf(_("AutoTrace version %s.\n"), at_version(FALSE));

    else if (ARGUMENT_IS("width-weight-factor"))
      fitting_opts->width_weight_factor = (gfloat) atof(optarg);

    /* Else it was just a flag; getopt has already done the assignment.  */
  }
  FINISH_COMMAND_LINE();
}

/* Convert hex char to integer */
static void input_list_formats(FILE * file)
{
  const char **list = at_input_list_new();
  const char **tmp;

  const char *suffix;
  const char *descr;
  tmp = list;
  while (*list) {
    suffix = *list++;
    descr = *list++;
    fprintf(file, "%5s %s\n", suffix, descr);
  }

  at_input_list_free(tmp);
}

static void output_list_formats(FILE * file)
{
  const char **list = at_output_list_new();
  const char **tmp;

  const char *suffix;
  const char *descr;
  tmp = list;
  while (*list) {
    suffix = *list++;
    descr = *list++;
    fprintf(file, "%10s %s\n", suffix, descr);
  }

  at_output_list_free(tmp);
}

static void dot_printer(gfloat percentage, gpointer client_data)
{
  int *current = (int *)client_data;
  float unit = (float)1.0 / (float)(dot_printer_max_column);
  int maximum = (int)(percentage / unit);

  while (*current < maximum) {
    fputc(dot_printer_char, stderr);
    (*current)++;
  }
}

static void dump(at_bitmap * bitmap, FILE * fp)
{
  unsigned short width, height;
  unsigned int np;

  width = at_bitmap_get_width(bitmap);
  height = at_bitmap_get_height(bitmap);
  np = at_bitmap_get_planes(bitmap);

  fwrite(AT_BITMAP_BITS(bitmap), sizeof(unsigned char), width * height * np, fp);
}

static void exception_handler(const gchar * msg, at_msg_type type, gpointer data)
{
  if (type == AT_MSG_FATAL) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
  } else if (type == AT_MSG_WARNING)
    fprintf(stderr, "%s\n", msg);
  else
    exception_handler(_("Wrong type of msg"), AT_MSG_FATAL, NULL);
}
