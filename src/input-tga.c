/* input-tga.c:	reads tga files

   Copyright (C) 1999, 2000, 2001 Martin Weber

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

/* TODO: bitmap functions */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* #include <unistd.h> */

#include "bitmap.h"
#include "logreport.h"
#include "xstd.h"
#include "input-bmp.h"

/* TODO:
   - Handle loading images that aren't 8 bits per channel.
*/

/* Round up a division to the nearest integer. */
#define ROUNDUP_DIVIDE(n,d) (((n) + (d - 1)) / (d))

#define INDEXED 1
#define INDEXEDA 2
#define GRAY 3
#define RGB 5
#define INDEXED_IMAGE 1
#define INDEXEDA_IMAGE 2
#define GRAY_IMAGE 3
#define GRAYA_IMAGE 4
#define RGB_IMAGE 5
#define RGBA_IMAGE 6

struct tga_header {
  unsigned char idLength;
  unsigned char colorMapType;

  /* The image type. */
#define TGA_TYPE_MAPPED      1
#define TGA_TYPE_COLOR       2
#define TGA_TYPE_GRAY        3
#define TGA_TYPE_MAPPED_RLE  9
#define TGA_TYPE_COLOR_RLE  10
#define TGA_TYPE_GRAY_RLE   11
  unsigned char imageType;

  /* Color Map Specification. */
  /* We need to separately specify high and low bytes to avoid endianness
     and alignment problems. */
  unsigned char colorMapIndexLo, colorMapIndexHi;
  unsigned char colorMapLengthLo, colorMapLengthHi;
  unsigned char colorMapSize;

  /* Image Specification. */
  unsigned char xOriginLo, xOriginHi;
  unsigned char yOriginLo, yOriginHi;

  unsigned char widthLo, widthHi;
  unsigned char heightLo, heightHi;

  unsigned char bpp;

  /* Image descriptor.
     3-0: attribute bpp
     4:   left-to-right ordering
     5:   top-to-bottom ordering
     7-6: zero
   */
#define TGA_DESC_ABITS      0x0f
#define TGA_DESC_HORIZONTAL 0x10
#define TGA_DESC_VERTICAL   0x20
  unsigned char descriptor;
};

static struct {
  unsigned int extensionAreaOffset;
  unsigned int developerDirectoryOffset;
#define TGA_SIGNATURE "TRUEVISION-XFILE"
  char signature[16];
  char dot;
  char null;
} tga_footer;

static at_bitmap ReadImage(FILE * fp, struct tga_header *hdr, at_exception_type * exp);
at_bitmap input_tga_reader(gchar * filename, at_input_opts_type * opts, at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{
  FILE *fp;
  struct tga_header hdr;

  at_bitmap image = at_bitmap_init(0, 0, 0, 1);
  at_exception_type exp = at_exception_new(msg_func, msg_data);

  fp = fopen(filename, "rb");
  if (!fp) {
    LOG("TGA: can't open \"%s\"\n", filename);
    at_exception_fatal(&exp, "Cannot open input tga file");
  }

  /* Check the footer. */
  if (fseek(fp, 0L - (sizeof(tga_footer)), SEEK_END)
      || fread(&tga_footer, sizeof(tga_footer), 1, fp) != 1) {
    LOG("TGA: Cannot read footer from \"%s\"\n", filename);
    at_exception_fatal(&exp, "TGA: Cannot read footer");
    goto cleanup;
  }

  /* Check the signature. */

  if (fseek(fp, 0, SEEK_SET) || fread(&hdr, sizeof(hdr), 1, fp) != 1) {
    LOG("TGA: Cannot read header from \"%s\"\n", filename);
    at_exception_fatal(&exp, "TGA: Cannot read header");
    goto cleanup;
  }

  /* Skip the image ID field. */
  if (hdr.idLength && fseek(fp, hdr.idLength, SEEK_CUR)) {
    LOG("TGA: Cannot skip ID field in \"%s\"\n", filename);
    at_exception_fatal(&exp, "TGA: Cannot skip ID field");
    goto cleanup;
  }

  image = ReadImage(fp, &hdr, &exp);
cleanup:
  fclose(fp);
  return image;
}

static int std_fread(unsigned char *buf, int datasize, int nelems, FILE * fp)
{

  return fread(buf, datasize, nelems, fp);
}

#define RLE_PACKETSIZE 0x80

/* Decode a bufferful of file. */
static int rle_fread(unsigned char *buf, int datasize, int nelems, FILE * fp)
{
  static unsigned char *statebuf = 0;
  static int statelen = 0;
  static int laststate = 0;

  int j, k;
  int buflen, count, bytes;
  unsigned char *p;

  /* Scale the buffer length. */
  buflen = nelems * datasize;

  j = 0;
  while (j < buflen) {
    if (laststate < statelen) {
      /* Copy bytes from our previously decoded buffer. */
      bytes = MIN(buflen - j, statelen - laststate);
      memcpy(buf + j, statebuf + laststate, bytes);
      j += bytes;
      laststate += bytes;

      /* If we used up all of our state bytes, then reset them. */
      if (laststate >= statelen) {
        laststate = 0;
        statelen = 0;
      }

      /* If we filled the buffer, then exit the loop. */
      if (j >= buflen)
        break;
    }

    /* Decode the next packet. */
    count = fgetc(fp);
    if (count == EOF) {
      return j / datasize;
    }

    /* Scale the byte length to the size of the data. */
    bytes = ((count & ~RLE_PACKETSIZE) + 1) * datasize;

    if (j + bytes <= buflen) {
      /* We can copy directly into the image buffer. */
      p = buf + j;
    } else {
      /* Allocate the state buffer if we haven't already. */
      if (!statebuf)
        statebuf = (unsigned char *)malloc(RLE_PACKETSIZE * datasize);
      p = statebuf;
    }

    if (count & RLE_PACKETSIZE) {
      /* Fill the buffer with the next value. */
      if (fread(p, datasize, 1, fp) != 1) {
        return j / datasize;
      }

      /* Optimized case for single-byte encoded data. */
      if (datasize == 1)
        memset(p + 1, *p, bytes - 1);
      else
        for (k = datasize; k < bytes; k += datasize)
          memcpy(p + k, p, datasize);
    } else {
      /* Read in the buffer. */
      if (fread(p, bytes, 1, fp) != 1) {
        return j / datasize;
      }
    }

    /* We may need to copy bytes from the state buffer. */
    if (p == statebuf)
      statelen = bytes;
    else
      j += bytes;
  }

  return nelems;
}

static at_bitmap ReadImage(FILE * fp, struct tga_header *hdr, at_exception_type * exp)
{
  at_bitmap image = at_bitmap_init(0, 0, 0, 1);
  unsigned char *buffer = NULL;
  unsigned char *alphas = NULL;

  unsigned short width, height, bpp, abpp, pbpp, nalphas;
  int j, k;
  int pelbytes, wbytes, bsize, npels, pels;
  int rle, badread;
  int itype, dtype;
  unsigned char *cmap = NULL;
  int (*myfread) (unsigned char *, int, int, FILE *);

  /* Find out whether the image is horizontally or vertically reversed. */
  char horzrev = (char)(hdr->descriptor & TGA_DESC_HORIZONTAL);
  char vertrev = (char)(!(hdr->descriptor & TGA_DESC_VERTICAL));

  image.bitmap = NULL;

  /* Reassemble the multi-byte values correctly, regardless of
     host endianness. */
  width = (hdr->widthHi << 8) | hdr->widthLo;
  height = (hdr->heightHi << 8) | hdr->heightLo;

  bpp = hdr->bpp;
  abpp = hdr->descriptor & TGA_DESC_ABITS;

  if (hdr->imageType == TGA_TYPE_COLOR || hdr->imageType == TGA_TYPE_COLOR_RLE)
    pbpp = MIN(bpp / 3, 8) * 3;
  else if (abpp < bpp)
    pbpp = bpp - abpp;
  else
    pbpp = bpp;

  if (abpp + pbpp > bpp) {
    LOG("TGA: %d bit image, %d bit alpha is greater than %d total bits per pixel\n", pbpp, abpp, bpp);
    at_exception_warning(exp, "TGA: alpha bit is too great");

    /* Assume that alpha bits were set incorrectly. */
    abpp = bpp - pbpp;
    LOG("TGA: reducing to %d bit alpha\n", abpp);
    at_exception_warning(exp, "TGA: alpha bit is reduced");
  } else if (abpp + pbpp < bpp) {
    LOG("TGA: %d bit image, %d bit alpha is less than %d total bits per pixel\n", pbpp, abpp, bpp);
    at_exception_warning(exp, "TGA: alpha bit is too little");

    /* Again, assume that alpha bits were set incorrectly. */
    abpp = bpp - pbpp;
    LOG("TGA: increasing to %d bit alpha\n", abpp);
    at_exception_warning(exp, "TGA: alpha bit is increased");
  }

  rle = 0;
  switch (hdr->imageType) {
  case TGA_TYPE_MAPPED_RLE:
    rle = 1;
  case TGA_TYPE_MAPPED:
    itype = INDEXED;

    /* Find the size of palette elements. */
    pbpp = MIN(hdr->colorMapSize / 3, 8) * 3;
    if (pbpp < hdr->colorMapSize)
      abpp = hdr->colorMapSize - pbpp;
    else
      abpp = 0;

    if (bpp != 8) {             /* We can only cope with 8-bit indices. */
      LOG("TGA: index sizes other than 8 bits are unimplemented\n");
      at_exception_fatal(exp, "TGA: index sizes other than 8 bits are unimplemented");
      return image;
    }

    if (abpp)
      dtype = INDEXEDA_IMAGE;
    else
      dtype = INDEXED_IMAGE;
    break;

  case TGA_TYPE_GRAY_RLE:
    rle = 1;
  case TGA_TYPE_GRAY:
    itype = GRAY;

    if (abpp)
      dtype = GRAYA_IMAGE;
    else
      dtype = GRAY_IMAGE;
    break;

  case TGA_TYPE_COLOR_RLE:
    rle = 1;
  case TGA_TYPE_COLOR:
    itype = RGB;

    if (abpp)
      dtype = RGBA_IMAGE;
    else
      dtype = RGB_IMAGE;
    break;

  default:
    {
      LOG("TGA: unrecognized image type %d\n", hdr->imageType);
      at_exception_fatal(exp, "TGA: unrecognized image type");
      return image;
    }
  }

  if ((abpp && abpp != 8) || ((itype == RGB || itype == INDEXED) && pbpp != 24) || (itype == GRAY && pbpp != 8)) {
    /* FIXME: We haven't implemented bit-packed fields yet. */
    LOG("TGA: channel sizes other than 8 bits are unimplemented\n");
    at_exception_fatal(exp, "TGA: channel sizes other than 8 bits are unimplemented");
    return image;
  }

  /* Check that we have a color map only when we need it. */
  if (itype == INDEXED) {
    if (hdr->colorMapType != 1) {
      LOG("TGA: indexed image has invalid color map type %d\n", hdr->colorMapType);
      at_exception_fatal(exp, "TGA: indexed image has invalid color map type");
      return image;
    }
  } else if (hdr->colorMapType != 0) {
    LOG("TGA: non-indexed image has invalid color map type %d\n", hdr->colorMapType);
    at_exception_fatal(exp, "TGA: non-indexed image has invalid color map type");
    return image;
  }

  nalphas = 0;
  if (hdr->colorMapType == 1) {
    /* We need to read in the colormap. */
    int index, colors;
    unsigned int length;

    index = (hdr->colorMapIndexHi << 8) | hdr->colorMapIndexLo;
    length = (hdr->colorMapLengthHi << 8) | hdr->colorMapLengthLo;

    if (length == 0) {
      LOG("TGA: invalid color map length %d\n", length);
      at_exception_fatal(exp, "TGA: invalid color map length");
      return image;
    }

    pelbytes = ROUNDUP_DIVIDE(hdr->colorMapSize, 8);
    colors = length + index;
    cmap = (unsigned char *)malloc(colors * pelbytes);

    /* Zero the entries up to the beginning of the map. */
    memset(cmap, 0, index * pelbytes);

    /* Read in the rest of the colormap. */
    if (fread(cmap + (index * pelbytes), pelbytes, length, fp) != length) {
      LOG("TGA: error reading colormap (ftell == %ld)\n", ftell(fp));
      at_exception_fatal(exp, "TGA: error reading colormap");
      return image;
    }

    /* If we have an alpha channel, then create a mapping to the alpha
       values. */
    if (pelbytes > 3)
      alphas = (unsigned char *)malloc(colors);

    k = 0;
    for (j = 0; j < colors * pelbytes; j += pelbytes) {
      /* Swap from BGR to RGB. */
      unsigned char tmp = cmap[j];
      cmap[k++] = cmap[j + 2];
      cmap[k++] = cmap[j + 1];
      cmap[k++] = tmp;

      /* Take the alpha values out of the colormap. */
      if (alphas)
        alphas[nalphas++] = cmap[j + 3];
    }

    /* If the last color was transparent, then omit it from the
       mapping. */
    if (nalphas && alphas[nalphas - 1] == 0)
      colors--;

    /* Now pretend as if we only have 8 bpp. */
    abpp = 0;
    pbpp = 8;
    pelbytes = 1;
  } else
    pelbytes = 3;

  image = at_bitmap_init(NULL, width, height, 3);

  /* Calculate TGA bytes per pixel. */
  bpp = ROUNDUP_DIVIDE(pbpp + abpp, 8);

  /* Maybe we need to reverse the data. */
  if (horzrev || vertrev)
    buffer = (unsigned char *)malloc(width * height * pelbytes * sizeof(unsigned char));
  if (rle)
    myfread = rle_fread;
  else
    myfread = std_fread;

  wbytes = width * pelbytes;
  badread = 0;

  npels = width * height;
  bsize = wbytes * height;

  /* Suck in the data one height at a time. */
  if (badread)
    pels = 0;
  else
    pels = (*myfread) (image.bitmap, bpp, npels, fp);

  if (pels != npels) {
    if (!badread) {
      /* Probably premature end of file. */
      LOG("TGA: error reading (ftell == %ld)\n", ftell(fp));
      at_exception_warning(exp, "TGA: eroor reading file");
      badread = 1;
    }

    /* Fill the rest of this tile with zeros. */
    memset(image.bitmap + (pels * bpp), 0, ((npels - pels) * bpp));
  }
  /* If we have indexed alphas, then set them. */
  if (nalphas) {
    /* Start at the end of the buffer, and work backwards. */
    k = (npels - 1) * bpp;
    for (j = bsize - pelbytes; j >= 0; j -= pelbytes) {
      /* Find the alpha for this index. */
      image.bitmap[j + 1] = alphas[image.bitmap[k]];
      image.bitmap[j] = image.bitmap[k--];
    }
  }

  if (itype == GRAY)
    for (j = bsize / 3 - 1; j >= 0; j -= 1) {
      /* Find the alpha for this index. */
      image.bitmap[3 * j] = image.bitmap[j];
      image.bitmap[3 * j + 1] = image.bitmap[j];
      image.bitmap[3 * j + 2] = image.bitmap[j];
    }

  if (pelbytes >= 3) {
    /* Rearrange the colors from BGR to RGB. */
    for (j = 0; j < bsize; j += pelbytes) {
      unsigned char tmp = image.bitmap[j];
      image.bitmap[j] = image.bitmap[j + 2];
      image.bitmap[j + 2] = tmp;
    }
  }

  if (horzrev || vertrev) {
    unsigned char *tmp;
    if (vertrev) {
      /* We need to mirror only vertically. */
      for (j = 0; j < bsize; j += wbytes)
        memcpy(buffer + j, image.bitmap + bsize - (j + wbytes), wbytes);
    } else if (horzrev) {
      /* We need to mirror only horizontally. */
      for (j = 0; j < bsize; j += wbytes)
        for (k = 0; k < wbytes; k += pelbytes)
          memcpy(buffer + k + j, image.bitmap + (j + wbytes) - (k + pelbytes), pelbytes);
    } else {
      /* Completely reverse the pixels in the buffer. */
      for (j = 0; j < bsize; j += pelbytes)
        memcpy(buffer + j, image.bitmap + bsize - (j + pelbytes), pelbytes);
    }

    /* Swap the buffers because we modified them. */
    tmp = buffer;
    buffer = image.bitmap;
    image.bitmap = tmp;
  }

  if (fgetc(fp) != EOF) {
    LOG("TGA: too much input data, ignoring extra...\n");
    at_exception_warning(exp, "TGA: too much input data, ignoring extra datum");
  }

  free(buffer);

  if (hdr->colorMapType == 1) {
    unsigned char *temp, *temp2, *temp3;
    unsigned char index;
    int xpos, ypos;

    temp2 = temp = image.bitmap;
    image.bitmap = temp3 = (unsigned char *)malloc(width * height * 3 * sizeof(unsigned char));

    for (ypos = 0; ypos < height; ypos++) {
      for (xpos = 0; xpos < width; xpos++) {
        index = *temp2++;
        *temp3++ = cmap[3 * index + 0];
        *temp3++ = cmap[3 * index + 1];
        *temp3++ = cmap[3 * index + 2];
      }
    }
    free(temp);
    free(cmap);
  }

  free(alphas);

  return image;
}                               /* read_image */
