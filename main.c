/* autotrace -- convert bitmaps to splines. */

#include "autotrace.h"
#include "input.h"
#include "output.h"
#include "message.h"
#include "cmdline.h"
#include "logreport.h"
#include "getopt.h"
#include "filename.h"
#include "xstd.h"
#include "atou.h"

#include <string.h>
#include <assert.h>

/* Pointers to functions based on input format.  (-input-format)  */
static at_input_read_func input_reader = NULL;

/* Return NAME with any leading path stripped off.  This returns a
   pointer into NAME.  For example, `basename ("/foo/bar.baz")'
   returns "bar.baz".  */
static char * get_basename (char * name);

/* The name of the file we're going to write.  (-output-file) */
static char * output_name = (char *)"";

/* The output function. (-output-format) */
static at_output_write_func output_writer = NULL;

/* Whether to print version information */
static bool printed_version;

/* Whether to trace a character's centerline or its outline */
static bool centerline = false;

/* Whether to write a log file */
static bool logging = false;

/* Should adjacent corners be removed?  */
static bool remove_adj_corners;

/* Thin all the lines in the image prior to fitting. */
static bool thin;

static char * read_command_line (int, char * [], at_fitting_opts_type *);

/* Read the string S as a percentage, i.e., a number between 0 and 100.  */
static real get_percent (char *);

static unsigned int hctoi (char c);

void input_list_formats(FILE * file);
void output_list_formats(FILE* file);

#define DEFAULT_FORMAT "eps"

#define STREQ(s1, s2) (strcmp ((char*)s1, (char*)s2) == 0)
#define STRTOLOWER(str)					\
while (*str != '\0') {char *c = (str); while (*c != '\0') {*c = tolower (*c); c++;} break;}


int
main (int argc, char * argv[])
{
  at_fitting_opts_type * fitting_opts;
  char * input_name, * input_rootname, * logfile_name=NULL;
  at_splines_type * splines;
  at_bitmap_type * bitmap;
  FILE *output_file;

  fitting_opts = at_fitting_opts_new ();

  input_name = read_command_line (argc, argv, fitting_opts);

  at_centerline = centerline;	/* Dirty  */
  fitting_opts->remove_adj_corners = remove_adj_corners;
  fitting_opts->thin = thin;

  if (STREQ (output_name, input_name))
    FATAL("Input and output file may not be the same\n");

  if ((input_rootname = remove_suffix (get_basename (input_name))) == NULL)
	FATAL1 ("Not a valid inputname %s\n", input_name);

  if (logging)
    log_file = xfopen (logfile_name = extend_filename (input_rootname, "log"), "w");

  /* BUG: Sometimes input_rootname points to the heap, sometimes to
     the stack, so it can't safely be freed. */
/*
  if (input_rootname != input_name)
    free (input_rootname);
*/
  if (logging)
    free (logfile_name);

  /* Set input_reader if it is not set in command line args */
  if (!input_reader)
    input_reader = at_input_get_handler (input_name);

  /* Set output_writer if it is not set in command line args */
  if (!output_writer)
    {
      output_writer = at_output_get_handler(DEFAULT_FORMAT);
      if (output_writer == NULL)
	FATAL1("Default format %s not supported\n", DEFAULT_FORMAT);
    }

  /* Open output file */
  if (STREQ (output_name, ""))
    output_file = stdout;
  else
    output_file = xfopen(output_name, "w");

  /* Open the main input file.  */
  if (input_reader != NULL)
    bitmap = at_bitmap_new(input_reader, input_name);
  else
    FATAL ("Unsupported inputformat\n"); 

  splines = at_splines_new(bitmap, fitting_opts);

  at_output_write (output_writer,
		   output_file, 
		   output_name,
		   0, 0, 
		   at_bitmap_get_width(bitmap), 
		   at_bitmap_get_height(bitmap),
		   splines);
  
  if (output_file != stdout)
    fclose (output_file);

  at_splines_free (splines); 
  at_bitmap_free (bitmap);
  at_fitting_opts_free(fitting_opts);

  return 0;
}


/* Reading the options.  */

#define USAGE1 "Options:\
<input_name> should be a filename, " INPUT_SUFFIX_LIST ".\n"\
  GETOPT_USAGE								\
"align-threshold <real>: if either coordinate of the endpoints on a\n\
  spline is closer than this, make them the same; default is .5.\n\
background-color <hexadezimal>: the color of the background that\n\
  should be ignored, for example FFFFFF;\n\
  default is no background color.\n\
centerline: trace a character's centerline, rather than its outline.\n\
color-count <unsigned>: number of colors a color bitmap is reduced to,\n\
  it does not work on grayscale, allowed are 1..256;\n\
  default is 0, that means not color reduction is done.\n\
corner-always-threshold <angle-in-degrees>: if the angle at a pixel is\n\
  less than this, it is considered a corner, even if it is within\n\
  `corner-surround' pixels of another corner; default is 60.\n\
corner-surround <unsigned>: number of pixels on either side of a\n\
  point to consider when determining if that point is a corner;\n\
  default is 4.\n\
corner-threshold <angle-in-degrees>: if a pixel, its predecessor(s),\n\
  and its successor(s) meet at an angle smaller than this, it's a\n\
  corner; default is 100.\n\
error-threshold <real>: subdivide fitted curves that are off by\n\
  more pixels than this; default is 2.0.\n\
filter-alternative-surround <unsigned>: another choice for\n\
  filter-surround; default is 1.\n\
filter-epsilon <real>: if the angles using filter-surround and\n\
  filter-alternative-surround points differ by more than this, use the\n\
  latter; default is 10.0.\n\
filter-iterations <unsigned>: smooth the curve this many times\n\
  before fitting; default is 4.\n\
filter-percent <percent>: when filtering, use the old point plus this\n\
  much of neighbors to determine the new point; default is 33.\n\
filter-surround <unsigned>: number of pixels on either side of a point\n\
  to consider when filtering that point; default is 2.\n\
input-format: " INPUT_SUFFIX_LIST ". \n\
help: print this message.\n"
#define USAGE2 "line-reversion-threshold <real>: if a spline is closer to a straight\n\
  line than this, weighted by the square of the curve length, keep it a\n\
  straight line even if it is a list with curves; default is .01.\n\
line-threshold <real>: if the spline is not more than this far away\n\
  from the straight line defined by its endpoints,\n\
  then output a straight line; default is 1.\n\
list-output-formats: print a list of support output formats to stderr.\n\
list-input-formats:  print a list of support input formats to stderr.\n\
log: write detailed progress reports to <input_name>.log.\n\
output-file <filename>: write to <filename>\n\
output-format <format>: use format <format> for the output file\n"\
"  " OUTPUT_SUFFIX_LIST " can be used.\n\
remove-adjacent-corners: remove corners that are adjacent.\n\
reparameterize-improve <percent>: if reparameterization\n\
  doesn't improve the fit by this much, as a percentage, stop; default\n\
  is 10.\n\
reparameterize-threshold <real>: if an initial fit is off by more\n\
  pixels than this, don't bother to reparameterize; default is 30.\n\
subdivide-search <percent>: percentage of the curve from the initial\n\
  guess for a subdivision point to look for a better one; default is 10.\n\
subdivide-surround <unsigned>: number of points on either side of a\n\
  point to consider when looking for a subdivision point; default is 4.\n\
subdivide-threshold <real>: if a point is this close or closer to a\n\
  straight line, subdivide there; default is .03.\n\
tangent-surround <unsigned>: number of points on either side of a\n\
  point to consider when computing the tangent at that point; default is 3.\n\
thin: thin all the lines in the image prior to fitting.\n\
version: print the version number of this program.\n\
"

/* We return the name of the image to process.  */

static char *
read_command_line (int argc, char * argv[], 
		   at_fitting_opts_type * fitting_opts)
{
  int g;   /* `getopt' return code.  */
  int option_index;
  struct option long_options[]
    = { { "align-threshold",		1, 0, 0 },
	{ "background-color",		1, 0, 0 },
        { "centerline",			0, (int*)&centerline, 1},
        { "color-count",                1, 0, 0 },
        { "corner-always-threshold",    1, 0, 0 },
        { "corner-surround",            1, 0, 0 },
        { "corner-threshold",           1, 0, 0 },
        { "error-threshold",            1, 0, 0 },
        { "filter-alternative-surround",1, 0, 0 },
        { "filter-epsilon",		1, 0, 0 },
        { "filter-iterations",          1, 0, 0 },
        { "filter-percent",		1, 0, 0 },
        { "filter-secondary-surround",  1, 0, 0 },
        { "filter-surround",            1, 0, 0 },
        { "help",                       0, 0, 0 },
        { "input-format",		1, 0, 0 },
        { "line-reversion-threshold",	1, 0, 0 },
        { "line-threshold",             1, 0, 0 },
        { "list-output-formats",               0, 0, 0 },
        { "list-input-formats",               0, 0, 0 },
        { "log",                        0, (int *) &logging, 1 },
        { "output-file",		1, 0, 0 },
        { "output-format",		1, 0, 0 },
        { "range",                      1, 0, 0 },
        { "remove-adjacent-corners",     0, (int *) &remove_adj_corners, 1 },
        { "reparameterize-improve",     1, 0, 0 },
        { "reparameterize-threshold",   1, 0, 0 },
        { "subdivide-search",		1, 0, 0 },
        { "subdivide-surround",		1, 0, 0 },
        { "subdivide-threshold",	1, 0, 0 },
        { "tangent-surround",           1, 0, 0 },
        { "thin",                       0, (int *) &thin, 1},
        { "version",                    0, (int *) &printed_version, 1 },
        { 0, 0, 0, 0 } };

  while (true)
    {

      g = getopt_long_only (argc, argv, "", long_options, &option_index);

      if (g == EOF)
        break;

      if (g == '?')
        exit (1);  /* Unknown option.  */

      assert (g == 0); /* We have no short option names.  */

      if (ARGUMENT_IS ("align-threshold"))
        fitting_opts->align_threshold = (real) atof (optarg);

      else if (ARGUMENT_IS ("background-color"))
        {
           if (strlen (optarg) != 6)
               FATAL ("background-color be six chars long");
	       fitting_opts->bgColor = at_color_new(hctoi (optarg[0]) * 16 + hctoi (optarg[1]),
				       hctoi (optarg[2]) * 16 + hctoi (optarg[3]),
				       hctoi (optarg[4]) * 16 + hctoi (optarg[5]));
 	}
      else if (ARGUMENT_IS ("color-count"))
        fitting_opts->color_count = atou (optarg);

      else if (ARGUMENT_IS ("corner-always-threshold"))
        fitting_opts->corner_always_threshold = (real) atof (optarg);

      else if (ARGUMENT_IS ("corner-surround"))
        fitting_opts->corner_surround = atou (optarg);

      else if (ARGUMENT_IS ("corner-threshold"))
        fitting_opts->corner_threshold = (real) atof (optarg);

      else if (ARGUMENT_IS ("error-threshold"))
        fitting_opts->error_threshold = (real) atof (optarg);

      else if (ARGUMENT_IS ("filter-alternative-surround"))
        fitting_opts->filter_alternative_surround = atou (optarg);

      else if (ARGUMENT_IS ("filter-epsilon"))
        fitting_opts->filter_epsilon = (real) atof (optarg);

      else if (ARGUMENT_IS ("filter-iterations"))
        fitting_opts->filter_iteration_count = atou (optarg);

      else if (ARGUMENT_IS ("filter-percent"))
        fitting_opts->filter_percent = get_percent (optarg);

      else if (ARGUMENT_IS ("filter-surround"))
        fitting_opts->filter_surround = atou (optarg);

      else if (ARGUMENT_IS ("help"))
        {
          fprintf (stderr, "Usage: %s [options] <input_name>.\n", argv[0]);
          fprintf (stderr, USAGE1);
	  fprintf (stderr, USAGE2);
	  fprintf (stderr, 
		   "\nYou can get the source code of autotrace from \n%s\n",
		   at_home_site());
          exit (0);
        }

      else if (ARGUMENT_IS ("input-format"))
        {
	  input_reader = at_input_get_handler_by_suffix (optarg);
	  if (!input_reader)
	      FATAL1 ("Output format %s not supported\n", optarg);
        }

      else if (ARGUMENT_IS ("line-reversion-threshold"))
        fitting_opts->line_reversion_threshold = (real) atof (optarg);

      else if (ARGUMENT_IS ("line-threshold"))
        fitting_opts->line_threshold = (real) atof (optarg);

      else if (ARGUMENT_IS ("list-output-formats"))
        {
	  fprintf (stderr, "Supported output formats:\n");
	  output_list_formats (stderr);
	  exit (0);
        }
      else if (ARGUMENT_IS ("list-input-formats"))
        {
	  fprintf (stderr, "Supported input formats:\n");
	  input_list_formats (stderr);
	  exit (0);
        }

      else if (ARGUMENT_IS ("output-file"))
        output_name = optarg;

      else if (ARGUMENT_IS ("output-format"))
        {
	    output_writer = at_output_get_handler (optarg);
	    if (output_writer == NULL)
	      {
		    FATAL1 ("Output format %s not supported\n", optarg);
	      }
        }

      else if (ARGUMENT_IS ("reparameterize-improve"))
        fitting_opts->reparameterize_improvement = get_percent (optarg);

      else if (ARGUMENT_IS ("reparameterize-threshold"))
        fitting_opts->reparameterize_threshold = (real) atof (optarg);

      else if (ARGUMENT_IS ("subdivide-search"))
        fitting_opts->subdivide_search = get_percent (optarg);

      else if (ARGUMENT_IS ("subdivide-surround"))
        fitting_opts->subdivide_surround = atou (optarg);

      else if (ARGUMENT_IS ("subdivide-threshold"))
        fitting_opts->subdivide_threshold = (real) atof (optarg);

      else if (ARGUMENT_IS ("tangent-surround"))
        fitting_opts->tangent_surround = atou (optarg);

      else if (ARGUMENT_IS ("version"))
        printf ("AutoTrace version %s.\n", at_version());

      /* Else it was just a flag; getopt has already done the assignment.  */
    }
  FINISH_COMMAND_LINE ();

}

/* Return NAME with any leading path stripped off.  This returns a
   pointer into NAME.  For example, `basename ("/foo/bar.baz")'
   returns "bar.baz".  */

static char *
get_basename (char * name)
{
#ifdef WIN32
  char * base = strrchr (name, '\\');
#else
  char * base = strrchr (name, '/');
#endif
  return base ? base + 1 : name;
}


/* Read the string S as a percentage, i.e., a number between 0 and 100.  */

static real get_percent(char * s)
{
  unsigned temp;
  temp = atou (s);
  if (temp > 100)
    FATAL1 ("get_percent: The argument %u should be atmost 100, since \
it's a percentage", temp);
  return (real) ((real) temp / 100.0);
}


/* Convert hex char to integer */

static unsigned int hctoi (char c)
{
  if (c == '0')
    return (0);
  else if (c == '1')
    return (1);
  else if (c == '2')
    return (2);
  else if (c == '3')
    return (3);
  else if (c == '4')
    return (4);
  else if (c == '5')
    return (5);
  else if (c == '6')
    return (6);
  else if (c == '7')
    return (7);
  else if (c == '8')
    return (8);
  else if (c == '9')
    return (9);
  else if (c == 'a')
    return (10);
  else if (c == 'A')
    return (10);
  else if (c == 'b')
    return (11);
  else if (c == 'B')
    return (11);
  else if (c == 'c')
    return (12);
  else if (c == 'C')
    return (12);
  else if (c == 'd')
    return (13);
  else if (c == 'D')
    return (13);
  else if (c == 'e')
    return (14);
  else if (c == 'E')
    return (14);
  else if (c == 'f')
    return (15);
  else if (c == 'F')
    return (15);
  else
    FATAL ("No hex values");
}

void
input_list_formats(FILE * file)
{
  char ** list = at_input_list_new ();
  char ** tmp;

  char * suffix;
  char * descr;
  tmp = list;
  while (*list)
    {
      suffix = *list++;
      descr = *list++;
      fprintf(file, "%5s %s\n", suffix, descr);
    }
  
  at_input_list_free(tmp);
}


void output_list_formats(FILE* file)
{
  char ** list = at_output_list_new ();
  char ** tmp;

  char * suffix;
  char * descr;
  tmp = list;
  while (*list)
    {
      suffix = *list++;
      descr = *list++;
      fprintf(file, "%5s %s\n", suffix, descr);
    }
  
  at_output_list_free(tmp);
}
