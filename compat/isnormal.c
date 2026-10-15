/*
 * Copyright (C) 2026 spotUP.
 * This file is part of ixemul-vtcon, a modified ixemul.library. It is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version. See LICENSES.README and COPYING.LIB.
 */
/* isnormal (C99 7.12.3.5) for double, which ixemul's <math.h> lacks (jq 1.7.1
 * needed it): finite, not zero, not subnormal. A function, as isnan and
 * isinf are in this <math.h>; a float argument is promoted, which does not
 * change the class of a normal float. The host test builds this file with
 * -DIXC_ISNORMAL=ixc_isnormal (the host's <math.h> has a macro of the name). */
#include <math.h>
#include <float.h>

#ifndef IXC_ISNORMAL
#undef isnormal
#define IXC_ISNORMAL isnormal
#endif

int
IXC_ISNORMAL(double x)
{
	double a = x < 0 ? -x : x;

	if (x != x)			/* NaN */
		return 0;
	return a >= DBL_MIN && a <= DBL_MAX;	/* excludes 0, subnormals, infinity */
}
