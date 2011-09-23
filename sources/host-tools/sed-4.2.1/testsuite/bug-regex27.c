/* Test REG_NEWLINE.
   Copyright (C) 2007 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Jakub Jelinek <jakub@redhat.com>, 2007.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <config.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>

struct tests
{
  const char *regex;
  const char *string;
  int cflags;
  int retval;
} tests[] = {
  { "a.b", "a\nb", REG_EXTENDED | REG_NEWLINE, REG_NOMATCH },
  { "a.b", "a\nb", REG_EXTENDED, 0 },
  { "a[^x]b", "a\nb", REG_EXTENDED | REG_NEWLINE, REG_NOMATCH },
  { "a[^x]b", "a\nb", REG_EXTENDED, 0 }
};

int
main (void)
{
  regex_t r;
  size_t i;
  int ret = 0;

  for (i = 0; i < sizeof (tests) / sizeof (tests[i]); ++i)
    {
      memset (&r, 0, sizeof (r));
      if (regcomp (&r, tests[i].regex, tests[i].cflags))
	{
	  printf ("regcomp %lu failed\n", i);
	  ret = 1;
	  continue;
	}
      int rv = regexec (&r, tests[i].string, 0, NULL, 0);
      if (rv != tests[i].retval)
	{
	  printf ("regexec %lu unexpected value %d != %d\n",
		  i, rv, tests[i].retval);
	  ret = 1;
	}
      regfree (&r);
    }
  return ret;
}
