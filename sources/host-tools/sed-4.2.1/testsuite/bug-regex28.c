/* Test RE_HAT_LISTS_NOT_NEWLINE and RE_DOT_NEWLINE.
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
  reg_syntax_t syntax;
  int retval;
} tests[] = {
#define EGREP RE_SYNTAX_EGREP
#define EGREP_NL (RE_SYNTAX_EGREP | RE_DOT_NEWLINE) & ~RE_HAT_LISTS_NOT_NEWLINE
  { "a.b", "a\nb", EGREP, -1 },
  { "a.b", "a\nb", EGREP_NL, 0 },
  { "a[^x]b", "a\nb", EGREP, -1 },
  { "a[^x]b", "a\nb", EGREP_NL, 0 },
  /* While \S and \W are internally handled as [^[:space:]] and [^[:alnum:]_],
     RE_HAT_LISTS_NOT_NEWLINE did not make any difference, so ensure
     it doesn't change.  */
  { "a\\Sb", "a\nb", EGREP, -1 },
  { "a\\Sb", "a\nb", EGREP_NL, -1 },
  { "a\\Wb", "a\nb", EGREP, 0 },
  { "a\\Wb", "a\nb", EGREP_NL, 0 }
};

int
main (void)
{
  struct re_pattern_buffer r;
  size_t i;
  int ret = 0;

  for (i = 0; i < sizeof (tests) / sizeof (tests[i]); ++i)
    {
      re_set_syntax (tests[i].syntax);
      memset (&r, 0, sizeof (r));
      if (re_compile_pattern (tests[i].regex, strlen (tests[i].regex), &r))
	{
	  printf ("re_compile_pattern %lu failed\n", i);
	  ret = 1;
	  continue;
	}
      size_t len = strlen (tests[i].string);
      int rv = re_search (&r, tests[i].string, len, 0, len, NULL);
      if (rv != tests[i].retval)
	{
	  printf ("re_search %lu unexpected value %d != %d\n",
		  i, rv, tests[i].retval);
	  ret = 1;
	}
      regfree (&r);
    }
  return ret;
}
