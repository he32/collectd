/**
 * collectd - src/entropy.c
 * Copyright (C) 2007       Florian octo Forster
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *   Florian octo Forster <octo at collectd.org>
 **/

#include "collectd.h"
#include "common.h"
#include "plugin.h"

static void entropy_submit (double);
static int entropy_read (void);

#if !KERNEL_LINUX && !KERNEL_NETBSD
#  error "No applicable input method."
#endif

#if KERNEL_LINUX

#define ENTROPY_FILE "/proc/sys/kernel/random/entropy_avail"

static int entropy_read (void)
{
	double entropy;
	FILE *fh;
	char buffer[64];

	fh = fopen (ENTROPY_FILE, "r");
	if (fh == NULL)
		return (-1);

	if (fgets (buffer, sizeof (buffer), fh) == NULL)
	{
		fclose (fh);
		return (-1);
	}
	fclose (fh);

	entropy = atof (buffer);
	
	if (entropy > 0.0)
		entropy_submit (entropy);

	return (0);
}
#endif /* KERNEL_LINUX */

#if KERNEL_NETBSD
/* Provide a NetBSD implementation, partial from rndctl.c */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/rnd.h>
#if HAVE_SYS_RNDIO_H
# include <sys/rndio.h>
#endif
#include <paths.h>

static int
entropy_read (void)
{
	rndpoolstat_t rs;
	int fd;

	fd = open(_PATH_URANDOM, O_RDONLY, 0644);
	if (fd < 0)
		return -1;

	if (ioctl(fd, RNDGETPOOLSTAT, &rs) < 0)
		return -1;

	entropy_submit (rs.curentropy);

	close(fd);
	return 0;
}

#endif /* KERNEL_NETBSD */

static void entropy_submit (double entropy)
{
	value_t values[1];
	value_list_t vl = VALUE_LIST_INIT;

	values[0].gauge = entropy;

	vl.values = values;
	vl.values_len = 1;
	sstrncpy (vl.host, hostname_g, sizeof (vl.host));
	sstrncpy (vl.plugin, "entropy", sizeof (vl.plugin));
	sstrncpy (vl.type, "entropy", sizeof (vl.type));

	plugin_dispatch_values (&vl);
}

void module_register (void)
{
	plugin_register_read ("entropy", entropy_read);
} /* void module_register */
