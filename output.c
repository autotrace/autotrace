/* output.c: output routines
   Copyright (C) 1999 Bernhard Herzog. */

#include "output.h"
#include "output-eps.h"
#include "output-p2e.h"
#include "output-sk.h"
#include "output-svg.h"
#include "output-fig.h"
#ifdef LIBSWF
#include "output-swf.h"
#endif /* LIBSWF */
#include "output-dxf.h"
#include "output-dxf12.h"
#include <string.h>

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
#ifdef LIBSWF
    {"swf",	"Shockwave Flash 3",		output_swf_writer},
#endif /* LIBSWF */
    {"dxf",     "DXF R14",                      output_dxf_writer},
    {"dxf12",   "DXF R12",                      output_dxf12_writer},
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


void output_list_formats(FILE* file)
{
    struct output_format_entry * entry;

    for (entry = output_formats; entry->name; entry++)
    {
	fprintf(file, "%5s %s\n", entry->name, entry->descr);
    }
}

/* version 0.24a */
