/* output.c: output routines
   Copyright (C) 1999 Bernhard Herzog. */

#include "output.h"
#include "xstd.h"
#include <string.h>

#include "output-eps.h"
#include "output-er.h"
#include "output-p2e.h"
#include "output-sk.h"
#include "output-svg.h"
#include "output-fig.h"
#ifdef HAVE_LIBSWF
#include "output-swf.h"
#endif /* HAVE_LIBSWF */
#include "output-emf.h"
#include "output-mif.h"
#include "output-dxf.h"
#include "output-epd.h"
#include "output-pdf.h"

struct output_format_entry {
    const char * name;
    const char * descr;
    output_write writer;
};


static struct output_format_entry output_formats[] = {
    {"eps",	"Encapsulated PostScript",	output_eps_writer},
    {"ai",	"Adobe Illustrator",		output_eps_writer},
    {"p2e",	"pstoedit frontend format",	output_p2e_writer},
    {"sk",	"Sketch",			output_sk_writer},
    {"svg",	"Scalable Vector Graphics",	output_svg_writer},
    {"fig",     "XFIG 3.2",                     output_fig_writer},
#ifdef HAVE_LIBSWF
    {"swf",	"Shockwave Flash 3",		output_swf_writer},
#endif /* HAVE_LIBSWF */
    {"emf",     "Enhanced Metafile format",     output_emf_writer},
    {"mif",     "FrameMaker MIF format",       output_mif_writer},
    {"er",      "Elastic Reality Shape file",   output_er_writer},
    {"dxf12",   "DXF format (without splines)", output_dxf12_writer},
    {"epd",     "EPD format",   output_epd_writer},
    {"pdf",     "PDF format",                 output_pdf_writer},
    {NULL,	NULL}
};

output_write output_get_handler(string name)
{
    struct output_format_entry *entry;

    for (entry = output_formats; entry->name; entry++)
    {
	if (strcmp(name, entry->name) == 0)
	    break;
    }

    return entry->writer;
}

char **
output_list (void)
{
  char ** list;
  int count = 0;
  int i;

  struct output_format_entry * entry;
  for (entry = output_formats; entry->name; entry++)
    count++;
  
  XMALLOC(list, sizeof(char*)*((2*count)+1));

  entry = output_formats;
  for (i = 0; i < count; i++)
    {
      list[2*i] = (char *)entry[i].name;
      list[2*i+1] = (char *)entry[i].descr;
    }
  list[2*i] = NULL;
  return list;
}

char *
output_shortlist (void)
{
  char * list;
  int count = 0;
  int length = 0;
  int i;

  struct output_format_entry * entry;
  for (entry = output_formats; entry->name; entry++)
    {
      count++;
      length += strlen (entry->name) + 2;
    }
  
  XMALLOC(list, sizeof (char) * (length + 1 + 2));

  entry = output_formats;
  strcpy (list, (char *) entry[0].name);
  for (i = 1; i < count - 1; i++)
    {
      strcat (list, ", ");
      strcat (list, (char *) entry[i].name);
    }
  strcat (list, " or ");
  strcat (list, (char *) entry[i].name);
  return list;
}
