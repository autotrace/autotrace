/* output.c: output routines
   Copyright (C) 1999 Bernhard Herzog. */

#include "output.h"
#include "xstd.h"
#include "filename.h"
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
#include "output-cgm.h"

struct output_format_entry {
    const char * name;
    const char * descr;
    at_output_write_func writer;
};

#define END   {NULL, NULL, NULL}
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
    {"mif",     "FrameMaker MIF format",        output_mif_writer},
    {"er",      "Elastic Reality Shape file",   output_er_writer},
    {"dxf",     "DXF format (without splines)", output_dxf12_writer},
    {"epd",     "EPD format",                   output_epd_writer},
    {"pdf",     "PDF format",                   output_pdf_writer},
    {"cgm",     "Computer Graphics Metafile",   output_cgm_writer},
    END
};

at_output_write_func
at_output_get_handler(at_string filename)
{
  char * ext = find_suffix (filename);
  if (ext == NULL)
    ext = "";
  
  return at_output_get_handler_by_suffix (ext);
}

at_output_write_func
at_output_get_handler_by_suffix(at_string name)
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
at_output_list_new (void)
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

void
at_output_list_free(char ** list)
{
  free(list);
}

char *
at_output_shortlist (void)
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

int
at_output_add_handler (at_string suffix, 
		       at_string description, 
		       at_output_write_func func)
{
  return 0;
}
