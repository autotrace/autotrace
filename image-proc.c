/* image-proc.c: image processing routines */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include <assert.h>
#include <math.h>
#include "xstd.h"
#include "image-proc.h"

#define BLACK 0
#define WHITE 0xff
#ifndef M_SQRT2
#define M_SQRT2 1.41421356237
#endif

#if 0
struct etyp { int t00, t11, t01, t01s; };


static at_bool get_edge(bitmap_type, int y, int x, struct etyp *t);
static void check(int v1, int v2, int v3, struct etyp *t);
#endif


/* Allocate storage for a new distance map with the same dimensions
   as BITMAP and initialize it so that pixels in BITMAP with value
   TARGET_VALUE are at distance zero and all other pixels are at
   distance infinity.  Then compute the gray-weighted distance from
   every non-target point to the nearest target point. */

distance_map_type
new_distance_map(bitmap_type bitmap, unsigned char target_value, at_bool padded, at_exception_type * exp)
{
    signed x, y;
    float d, min;
    distance_map_type dist;
    unsigned char *b = BITMAP_BITS(bitmap);
    unsigned w = BITMAP_WIDTH(bitmap);
    unsigned h = BITMAP_HEIGHT(bitmap);
    unsigned spp = BITMAP_PLANES(bitmap);

    dist.height = h; dist.width = w;
    XMALLOC(dist.d, h * sizeof(float*));
    XMALLOC(dist.weight, h * sizeof(float*));
    for (y = 0; y < (signed) h; y++)
    {
      XCALLOC(dist.d[y], w * sizeof(float));
      XMALLOC(dist.weight[y], w * sizeof(float));
    }

    if (spp == 3)
    {
      for (y = 0; y < (signed) h; y++)
      {
        for (x = 0; x < (signed) w; x++, b += spp)
        {
          int gray; float fgray;
          gray = (int)LUMINANCE(b[0], b[1], b[2]);
          dist.d[y][x] = (gray == target_value ? 0.0F : 1.0e10F);
          fgray = gray * 0.0039215686F;  /* = gray / 255.0F */
          dist.weight[y][x] = 1.0F - fgray;
/*        dist.weight[y][x] = 1.0F - (fgray * fgray);*/
/*        dist.weight[y][x] = (fgray < 0.5F ? 1.0F - fgray : -2.0F * fgray * (fgray - 1.0F));*/
	    }
      }
    }
    else
    {
      for (y = 0; y < (signed) h; y++)
      {
        for (x = 0; x < (signed) w; x++, b += spp)
        {
          int gray; float fgray;
          gray = b[0];
          dist.d[y][x] = (gray == target_value ? 0.0F : 1.0e10F);
          fgray = gray * 0.0039215686F;  /* = gray / 255.0F */
          dist.weight[y][x] = 1.0F - fgray;
/*        dist.weight[y][x] = 1.0F - (fgray * fgray);*/
/*        dist.weight[y][x] = (fgray < 0.5F ? 1.0F - fgray : -2.0F * fgray * (fgray - 1.0F)); */
        }
      }
    }

    /* If the image is padded then border points are all at most
       one unit away from the nearest target point. */
    if (padded)
    {
        for (y = 0; y < (signed) h; y++)
        {
            if (dist.d[y][0] > dist.weight[y][0])
                dist.d[y][0] = dist.weight[y][0];
            if (dist.d[y][w - 1] > dist.weight[y][w - 1])
                dist.d[y][w - 1] = dist.weight[y][w - 1];
        }
        for (x = 0; x < (signed) w; x++)
        {
            if (dist.d[0][x] > dist.weight[0][x])
                dist.d[0][x] = dist.weight[0][x];
            if (dist.d[h - 1][x] > dist.weight[h - 1][x])
                dist.d[h - 1][x] = dist.weight[h - 1][x];
        }
    }

    /* Scan the image from left to right, top to bottom.
       Examine the already-visited neighbors of each point (those
       situated above or to the left of it).  Each neighbor knows
       the distance to its nearest target point; add to this distance
       the distance from the central point to the neighbor (either
       sqrt(2) or one) multiplied by the central point's weight
       (derived from its gray level).  Replace the distance already
       stored at the central point if the new distance is smaller. */
    for (y = 1; y < (signed) h; y++)
    {
        for (x = 1; x < (signed) w; x++)
        {
            if (dist.d[y][x] == 0.0F) continue;

            min = dist.d[y][x];

            /* upper-left neighbor */
            d = dist.d[y - 1][x - 1] + (float) M_SQRT2 * dist.weight[y][x];
            if (d < min) min = dist.d[y][x] = d;

            /* upper neighbor */
            d = dist.d[y - 1][x] + dist.weight[y][x];
            if (d < min) min = dist.d[y][x] = d;

            /* left neighbor */
            d = dist.d[y][x - 1] + dist.weight[y][x];
            if (d < min) min = dist.d[y][x] = d;

            /* upper-right neighbor (except at the last column) */
            if (x + 1 < (signed) w)
            {
                d = dist.d[y - 1][x + 1] + (float) M_SQRT2 * dist.weight[y][x];
                if (d < min) min = dist.d[y][x] = d;
            }
        }
    }

    /* Same as above, but now scanning right to left, bottom to top. */
    for (y = h - 2; y >= 0; y--)
    {
        for (x = w - 2; x >= 0; x--)
        {
            min = dist.d[y][x];

            /* lower-right neighbor */
            d = dist.d[y + 1][x + 1] + (float) M_SQRT2 * dist.weight[y][x];
	        if (d < min) min = dist.d[y][x] = d;

            /* lower neighbor */
            d = dist.d[y + 1][x] + dist.weight[y][x];
	        if (d < min) min = dist.d[y][x] = d;

            /* right neighbor */
            d = dist.d[y][x + 1] + dist.weight[y][x];
	        if (d < min) min = dist.d[y][x] = d;

            /* lower-left neighbor (except at the first column) */
            if (x - 1 >= 0)
            {
                d = dist.d[y + 1][x - 1] + (float) M_SQRT2 * dist.weight[y][x];
                if (d < min) min = dist.d[y][x] = d;
            }
        }
    }
    return dist;
}


/* Free the dynamically-allocated storage associated with a distance map. */

void
free_distance_map(distance_map_type *dist)
{
    unsigned y, h;

    if (!dist) return;

    h = BITMAP_HEIGHT(*dist);

    if (dist->d != NULL)
    {
	for (y = 0; y < h; y++)
	    free((at_address*)dist->d[y]);
        free((at_address*)dist->d);
    }
    if (dist->weight != NULL)
    {
	for (y = 0; y < h; y++)
	    free((at_address*)dist->weight[y]);
        free((at_address*)dist->weight);
    }
}


#if 0
void
medial_axis(bitmap_type *bitmap, distance_map_type *dist,
    const color_type *bg_color)
{
    unsigned x, y, test;
    unsigned w, h;
    unsigned char *b;
    float **d, f;
    color_type bg;

    assert(bitmap != NULL);

    assert(BITMAP_PLANES(*bitmap) == 1);

    b = BITMAP_BITS(*bitmap);
    assert(b != NULL);
    assert(dist != NULL);
    d = dist->d;
    assert(d != NULL);

    h = BITMAP_HEIGHT(*dist);
    w = BITMAP_WIDTH(*dist);
    assert(BITMAP_WIDTH(*bitmap) == w && BITMAP_HEIGHT(*bitmap) == h);

    if (bg_color) bg = *bg_color;
    else bg.r = bg.g = bg.b = 255;

    f = d[0][0] + 0.5;
    test = (f < d[1][0]) + (f < d[1][1]) + (f < d[0][1]);
    if (test > 1) b[0] = bg.r;

    f = d[0][w-1] + 0.5;
    test = (f < d[1][w-1]) + (f < d[1][w-2]) + (f < d[0][w-2]);
    if (test > 1) b[w-1] = bg.r;

    for (x = 1; x < w - 1; x++)
    {
	    f = d[0][x] + 0.5;
	    test = (f < d[0][x-1]) + (f < d[0][x+1]) + (f < d[1][x-1])
	        + (f < d[1][x]) + (f < d[1][x+1]);
	    if (test > 1) b[x] = bg.r;
    }
    b += w;

    for (y = 1; y < h - 1; y++)
    {
	    f = d[y][0] + 0.5;
	    test = (f < d[y-1][0]) + (f < d[y-1][1]) + (f < d[y][1])
	        + (f < d[y+1][0]) + (f < d[y+1][1]);
	    if (test > 1) b[0] = bg.r;

	    for (x = 1; x < w - 1; x++)
		{
	        f = d[y][x] + 0.5;
	        test = (f < d[y-1][x-1]) + (f < d[y-1][x]) + (f < d[y-1][x+1])
		    + (f < d[y][x-1]) + (f < d[y][x+1])
		    + (f < d[y+1][x-1]) + (f < d[y+1][x]) + (f < d[y+1][x+1]);
	        if (test > 1) b[x] = bg.r;
		}

	    f = d[y][w-1] + 0.5;
	    test = (f < d[y-1][w-1]) + (f < d[y-1][w-2]) + (f < d[y][w-2])
	        + (f < d[y+1][w-1]) + (f < d[y+1][w-2]);
	    if (test > 1)
	        b[w-1] = bg.r;

        b += w;
    }

    for (x = 1; x < w - 1; x++)
    {
	    f = d[h-1][x] + 0.5;
	    test = (f < d[h-1][x-1]) + (f < d[h-1][x+1])
	        + (f < d[h-2][x-1]) + (f < d[h-2][x]) + (f < d[h-2][x+1]);
	    if (test > 1) b[x] = bg.r;
    }

    f = d[h-1][0] + 0.5;
    test = (f < d[h-2][0]) + (f < d[h-2][1]) + (f < d[h-1][1]);
    if (test > 1) b[0] = bg.r;

    f = d[h-1][w-1] + 0.5;
    test = (f < d[h-2][w-1]) + (f < d[h-2][w-2]) + (f < d[h-1][w-2]);
    if (test > 1) b[w-1] = bg.r;
}
#endif


/* Binarize a grayscale or color image. */

void
binarize(bitmap_type *bitmap)
{
    unsigned i, npixels, spp;
    unsigned char *b;

    assert(bitmap != NULL);
    assert(BITMAP_BITS(*bitmap) != NULL);

    b = BITMAP_BITS(*bitmap);
    spp = BITMAP_PLANES(*bitmap);
    npixels = BITMAP_WIDTH(*bitmap) * BITMAP_HEIGHT(*bitmap);

    if (spp == 1)
    {
	    for (i = 0; i < npixels; i++)
	        b[i] = (b[i] > GRAY_THRESHOLD ? WHITE : BLACK);
    }
    else if (spp == 3)
    {
	    unsigned char *rgb = b;
	    for (i = 0; i < npixels; i++, rgb += 3)
		{
	        b[i] = (LUMINANCE(rgb[0], rgb[1], rgb[2]) > GRAY_THRESHOLD
		        ? WHITE : BLACK);
		}
	    XREALLOC(BITMAP_BITS(*bitmap), npixels);
	    BITMAP_PLANES(*bitmap) = 1;
    }
    else
    {
	    WARNING1("binarize: %u-plane images are not supported", spp);
    }
}


#if 0
/* Thin a binary image, replacing the original image with the thinned one. */

bitmap_type
ip_thin(bitmap_type input_b)
{
    unsigned y, x, i;
    at_bool k, again;
    struct etyp t;
    unsigned w = BITMAP_WIDTH(input_b);
    unsigned h = BITMAP_HEIGHT(input_b);
    size_t num_bytes = w * h;
    bitmap_type b = input_b;

    if (BITMAP_PLANES(input_b) != 1)
    {
	    FATAL1("thin: single-plane image required; "
	        "%u-plane images cannot be thinned", BITMAP_PLANES(input_b));
	    return b;
    }

    /* Process and return a copy of the input image. */
    XMALLOC(b.bitmap, num_bytes);
    memcpy(b.bitmap, input_b.bitmap, num_bytes);

    /* Set background pixels to zero, foreground pixels to one. */
    for (i = 0; i < num_bytes; i++)
	b.bitmap[i] = (b.bitmap[i] == BLACK ? 1 : 0);

    again = true;
    while (again)
    {
	again = false;

	for (y = 1; y < h - 1; y++)
	{
	    for (x = 1; x < w - 1; x++)
	    {
		    /* During processing, pixels are used to store edge
		       type codes, so we can't just test for WHITE or BLACK. */
		    if (*BITMAP_PIXEL(b, y, x) == 0) continue;

		    k = (!get_edge(b, y, x, &t)
		        || (get_edge(b, y, x+1, &t) && *BITMAP_PIXEL(b, y-1, x)
			    && *BITMAP_PIXEL(b, y+1, x))
		        || (get_edge(b, y+1, x, &t) && *BITMAP_PIXEL(b, y, x-1)
			    && *BITMAP_PIXEL(b, y, x+1))
		        || (get_edge(b, y, x+1, &t) && get_edge(b, y+1, x+1, &t)
			    && get_edge(b, y+1, x, &t)));
		    if (k) continue;

		    get_edge(b, y, x, &t);
		    if (t.t01) *BITMAP_PIXEL(b, y, x) |= 4;
		    *BITMAP_PIXEL(b, y, x) |= 2;
		    again = true;
	    }
	}

	for (y = 0; y < h; y++)
	    for (x = 0; x < w; x++)
		    if (*BITMAP_PIXEL(b, y, x) & 02) *BITMAP_PIXEL(b, y, x) = 0;

	for (y = 1; y < h - 1; y++)
	{
	    for (x = 1; x < w - 1; x++)
	    {
		    if (*BITMAP_PIXEL(b, y, x) == 0) continue;

		    k = (!get_edge(b, y, x, &t)
		        || ((*BITMAP_PIXEL(b, y, x) & 04) == 0)
		        || (get_edge(b, y+1, x, &t) && (*BITMAP_PIXEL(b, y, x-1))
			    && *BITMAP_PIXEL(b, y, x+1))
		        || (get_edge(b, y, x+1, &t) && *BITMAP_PIXEL(b, y-1, x)
			    && *BITMAP_PIXEL(b, y+1, x))
		        || (get_edge(b, y+1, x, &t) && get_edge(b, y, x+1, &t)
			    && get_edge(b, y+1, x+1, &t)));
		    if (k) continue;

		    *BITMAP_PIXEL(b, y, x) |= 02;
		    again = true;
	    }
	}

	for (y = 0; y < h; y++)
	{
	    for (x = 0; x < w; x++)
	    {
		    if (*BITMAP_PIXEL(b, y, x) & 02) *BITMAP_PIXEL(b, y, x) = 0;
		    else if (*BITMAP_PIXEL(b, y, x) > 0) *BITMAP_PIXEL(b, y, x) = 1;
	    }
	}
    }

    /* Staircase removal; northward bias. */
    for (y = 1; y < h - 1; y++)
    {
	    for (x = 1; x < w - 1; x++)
		{
	        if (*BITMAP_PIXEL(b, y, x) == 0) continue;

	        k = !(*BITMAP_PIXEL(b, y-1, x)
		        && ((*BITMAP_PIXEL(b, y, x+1) && !*BITMAP_PIXEL(b, y-1, x+1)
		        && !*BITMAP_PIXEL(b, y+1, x-1)
		        && (!*BITMAP_PIXEL(b, y, x-1) || !*BITMAP_PIXEL(b, y+1, x)))
		        || (*BITMAP_PIXEL(b, y, x-1) && !*BITMAP_PIXEL(b, y-1, x-1)
		        && !*BITMAP_PIXEL(b, y+1, x+1) &&
		        (!*BITMAP_PIXEL(b, y, x+1) || !*BITMAP_PIXEL(b, y+1, x)))));
	        if (k) continue;

	        *BITMAP_PIXEL(b, y, x) |= 02;
		}
    }
    for (y = 0; y < h; y++)
    {
	    for (x = 0; x < w; x++)
		{
	        if (*BITMAP_PIXEL(b, y, x) & 02) *BITMAP_PIXEL(b, y, x) = 0;
	        else if (*BITMAP_PIXEL(b, y, x) > 0) *BITMAP_PIXEL(b, y, x) = 1;
		}
    }

    /* Southward bias */
    for (y = 1; y < h - 1; y++)
    {
	    for (x = 1; x < w - 1; x++)
		{
	        if (*BITMAP_PIXEL(b, y, x) == 0) continue;

	        k = !(*BITMAP_PIXEL(b, y+1, x)
		    && ((*BITMAP_PIXEL(b, y, x+1) && !*BITMAP_PIXEL(b, y+1, x+1)
		    && !*BITMAP_PIXEL(b, y-1, x-1) && (!*BITMAP_PIXEL(b, y, x-1)
		    || !*BITMAP_PIXEL(b, y-1, x))) || (*BITMAP_PIXEL(b, y, x-1)
		    && !*BITMAP_PIXEL(b, y+1, x-1) && !*BITMAP_PIXEL(b, y-1, x+1)
		    && (!*BITMAP_PIXEL(b, y, x+1) || !*BITMAP_PIXEL(b, y-1, x)) )));
	        if (k) continue;

	        *BITMAP_PIXEL(b, y, x) |= 02;
		}
    }
    for (y = 0; y < h; y++)
    {
	    for (x = 0; x < w; x++)
		{
	        if (*BITMAP_PIXEL(b, y, x) & 02) *BITMAP_PIXEL(b, y, x) = 0;
	        else if (*BITMAP_PIXEL(b, y, x) > 0) *BITMAP_PIXEL(b, y, x) = 1;
		}
    }

    /* Set background pixels to WHITE, foreground pixels to BLACK. */
    for (i = 0; i < num_bytes; i++)
	b.bitmap[i] = (b.bitmap[i] == 0 ? WHITE : BLACK);
    return b;
}


at_bool get_edge(bitmap_type b, int y, int x, struct etyp *t)
{
    t->t00 = 0; t->t01 = 0; t->t01s = 0; t->t11 = 0;
    check(*BITMAP_PIXEL(b, y - 1, x - 1), *BITMAP_PIXEL(b, y - 1, x),
	*BITMAP_PIXEL(b, y - 1, x + 1), t);
    check(*BITMAP_PIXEL(b, y - 1, x + 1), *BITMAP_PIXEL(b, y, x + 1),
	*BITMAP_PIXEL(b, y + 1, x + 1), t);
    check(*BITMAP_PIXEL(b, y + 1, x + 1), *BITMAP_PIXEL(b, y + 1, x),
	*BITMAP_PIXEL(b, y + 1, x - 1), t);
    check(*BITMAP_PIXEL(b, y + 1, x - 1), *BITMAP_PIXEL(b, y, x - 1),
	*BITMAP_PIXEL(b, y - 1, x - 1), t);
    return *BITMAP_PIXEL(b, y, x) && t->t00 && t->t11 && !t->t01s;
}


void check(int v1, int v2, int v3, struct etyp *t)
{
    if (!v2 && (!v1 || !v3)) t->t00 = 1;
    if (v2 && (v1 || v3)) t->t11 = 1;
    if ((!v1 && v2) || (!v2 && v3)) { t->t01s = t->t01; t->t01 = 1; }
}
#endif
