/* autotrace.h */
#ifndef AUTOTRACE_H
#define AUTOTRACE_H

#include "fit.h"
#include "bitmap.h"
#include "input.h"
#include "spline.h"
#include "output.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Defines
 */
#define AT_INPUT_SUFFIX_LIST INPUT_SUFFIX_LIST
#define AT_OUTPUT_SUFFIX_LIST OUTPUT_SUFFIX_LIST

/*
 * Typedefs
 */
typedef fitting_opts_type at_fitting_opts_type ;
typedef bitmap_type at_bitmap_type;
typedef input_read at_input_read_func;
typedef spline_list_array_type at_splines_type;
typedef output_write at_output_write_func;

/*
 * Option related
 */
at_fitting_opts_type * at_fitting_opts_new(void);
void at_fitting_opts_free(at_fitting_opts_type * opts);
/* TODO internal data access */

/*
 * Bitmap related
 */
at_bitmap_type * at_bitmap_new (at_input_read_func input_reader,
				string filename);
unsigned short at_bitmap_get_width (at_bitmap_type * bitmap);
unsigned short at_bitmap_get_height (at_bitmap_type * bitmap);
void at_bitmap_free (at_bitmap_type * bitmap);
/* TODO internal data access */

/*
 * Input related
 */
at_input_read_func at_input_get_handler (string filename);
at_input_read_func at_input_get_handler_by_suffix (string suffix);
char ** at_input_list_new (void);
void at_input_list_free(char ** list);

/*
 * Spline related
 */
at_splines_type * at_splines_new (at_bitmap_type * bitmap,
				  at_fitting_opts_type * opts);
void at_splines_free (at_splines_type * splines);
/* TODO internal data access */

/*
 * Output related
 */
at_output_write_func at_output_get_handler (string suffix);
void 
at_output_write(at_output_write_func output_writer,
		FILE * writeto,
		char * name,
		int llx, int lly, int urx, int ury,
		at_splines_type * splines);
char ** at_output_list_new (void);
void at_output_list_free(char ** list);

/*
 * Version
 */
char * at_version ();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* AUTOTRACE_H */
