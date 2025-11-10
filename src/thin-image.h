/*
 * Copyright (C) 2001, 2002 Martin Weber
 * SPDX-FileCopyrightText: © 2000-2001 Martin Weber
 * SPDX-FileCopyrightText: © 2001-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef THIN_IMAGE_H
#define THIN_IMAGE_H

/*
 * C code from the article
 * "Efficient Binary Image Thinning using Neighborhood Maps"
 * by Joseph M. Cychosz, 3ksnn64@ecn.purdue.edu
 * in "Graphics Gems IV", Academic Press, 1994
 */

#include "autotrace.h"
#include "color.h"
#include "input.h"
#include "exception.h"

void thin_image(at_bitmap *image, const at_color *bg_color, at_exception_type *exp);

#endif /* not THIN_IMAGE_H */
