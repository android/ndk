/*  GNU SED, a batch stream editor.
    Copyright (C) 1999, 2002, 2003, 2004, 2005, 2006
    Free Software Foundation, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA. */

#include "sed.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef gettext_noop
# define N_(String) gettext_noop(String)
#else
# define N_(String) (String)
#endif

extern bool use_extended_syntax_p;

static const char errors[] =
  "no previous regular expression\0"
  "cannot specify modifiers on empty regexp";

#define NO_REGEX (errors)
#define BAD_MODIF (NO_REGEX + sizeof(N_("no previous regular expression")))
#define END_ERRORS (BAD_MODIF + sizeof(N_("cannot specify modifiers on empty regexp")))



static void
compile_regex_1 (new_regex, needed_sub)
  struct regex *new_regex;
  int needed_sub;
{
#ifdef REG_PERL
  int errcode;
  errcode = regncomp(&new_regex->pattern, new_regex->re, new_regex->sz,
		     (needed_sub ? 0 : REG_NOSUB)
		     | new_regex->flags
		     | extended_regexp_flags);

  if (errcode)
    {
      char errorbuf[200];
      regerror(errcode, NULL, errorbuf, 200);
      bad_prog(gettext(errorbuf));
    }
#else
  const char *error;
  int syntax = ((extended_regexp_flags & REG_EXTENDED)
		 ? RE_SYNTAX_POSIX_EXTENDED
                 : RE_SYNTAX_POSIX_BASIC);

  syntax &= ~RE_DOT_NOT_NULL;
  syntax |= RE_NO_POSIX_BACKTRACKING;

  switch (posixicity)
    {
    case POSIXLY_EXTENDED:
      syntax &= ~RE_UNMATCHED_RIGHT_PAREN_ORD;
      break;
    case POSIXLY_CORRECT:
      syntax |= RE_UNMATCHED_RIGHT_PAREN_ORD;
      break;
    case POSIXLY_BASIC:
      syntax |= RE_UNMATCHED_RIGHT_PAREN_ORD | RE_LIMITED_OPS | RE_NO_GNU_OPS;
      break;
    }

#ifdef RE_ICASE
  syntax |= (new_regex->flags & REG_ICASE) ? RE_ICASE : 0;
#endif
#ifdef RE_NO_SUB
  syntax |= needed_sub ? 0 : RE_NO_SUB;
#endif

  new_regex->pattern.fastmap = malloc (1 << (sizeof (char) * 8));

  /* If REG_NEWLINE is set, newlines are treated differently.  */
  if (new_regex->flags & REG_NEWLINE)
    {
      /* REG_NEWLINE implies neither . nor [^...] match newline.  */
      syntax &= ~RE_DOT_NEWLINE;
      syntax |= RE_HAT_LISTS_NOT_NEWLINE;
    }

  re_set_syntax (syntax);
  error = re_compile_pattern (new_regex->re, new_regex->sz,
			      &new_regex->pattern);
  new_regex->pattern.newline_anchor = (new_regex->flags & REG_NEWLINE) != 0;

  new_regex->pattern.translate = NULL;
#ifndef RE_ICASE
  if (new_regex->flags & REG_ICASE)
    {
      static char translate[1 << (sizeof(char) * 8)];
      int i;
      for (i = 0; i < sizeof(translate) / sizeof(char); i++)
	translate[i] = tolower (i);

      new_regex->pattern.translate = translate;
    }
#endif

  if (error)
    bad_prog(error);
#endif

  /* Just to be sure, I mark this as not POSIXLY_CORRECT behavior */
  if (needed_sub
      && new_regex->pattern.re_nsub < needed_sub - 1
      && posixicity == POSIXLY_EXTENDED)
    {
      char buf[200];
      sprintf(buf, _("invalid reference \\%d on `s' command's RHS"),
	      needed_sub - 1);
      bad_prog(buf);
    }
}

struct regex *
compile_regex(b, flags, needed_sub)
  struct buffer *b;
  int flags;
  int needed_sub;
{
  struct regex *new_regex;
  size_t re_len;

  /* // matches the last RE */
  if (size_buffer(b) == 0)
    {
      if (flags > 0)
	bad_prog(_(BAD_MODIF));
      return NULL;
    }

  re_len = size_buffer(b);
  new_regex = ck_malloc(sizeof (struct regex) + re_len - 1);
  new_regex->flags = flags;
  memcpy (new_regex->re, get_buffer(b), re_len);

#ifdef REG_PERL
  new_regex->sz = re_len;
#else
  /* GNU regex does not process \t & co. */
  new_regex->sz = normalize_text(new_regex->re, re_len, TEXT_REGEX);
#endif

  compile_regex_1 (new_regex, needed_sub);
  return new_regex;
}

#ifdef REG_PERL
static void
copy_regs (regs, pmatch, nregs)
     struct re_registers *regs;
     regmatch_t *pmatch;
     int nregs;
{
  int i;
  int need_regs = nregs + 1;
  /* We need one extra element beyond `num_regs' for the `-1' marker GNU code
     uses.  */

  /* Have the register data arrays been allocated?  */
  if (!regs->start)
    { /* No.  So allocate them with malloc.  */
      regs->start = MALLOC (need_regs, regoff_t);
      regs->end = MALLOC (need_regs, regoff_t);
      regs->num_regs = need_regs;
    }
  else if (need_regs > regs->num_regs)
    { /* Yes.  We also need more elements than were already
         allocated, so reallocate them.  */
      regs->start = REALLOC (regs->start, need_regs, regoff_t);
      regs->end = REALLOC (regs->end, need_regs, regoff_t);
      regs->num_regs = need_regs;
    }

  /* Copy the regs.  */
  for (i = 0; i < nregs; ++i)
    {
      regs->start[i] = pmatch[i].rm_so;
      regs->end[i] = pmatch[i].rm_eo;
    }
  for ( ; i < regs->num_regs; ++i)
    regs->start[i] = regs->end[i] = -1;
}
#endif

int
match_regex(regex, buf, buflen, buf_start_offset, regarray, regsize)
  struct regex *regex;
  char *buf;
  size_t buflen;
  size_t buf_start_offset;
  struct re_registers *regarray;
  int regsize;
{
  int ret;
  static struct regex *regex_last;
#ifdef REG_PERL
  regmatch_t rm[10], *regmatch = rm;
  if (regsize > 10)
    regmatch = (regmatch_t *) alloca (sizeof (regmatch_t) * regsize);
#endif

  /* printf ("Matching from %d/%d\n", buf_start_offset, buflen); */

  /* Keep track of the last regexp matched. */
  if (!regex)
    {
      regex = regex_last;
      if (!regex_last)
	bad_prog(_(NO_REGEX));
    }
  else
    regex_last = regex;

#ifdef REG_PERL
  regmatch[0].rm_so = CAST(int)buf_start_offset;
  regmatch[0].rm_eo = CAST(int)buflen;
  ret = regexec (&regex->pattern, buf, regsize, regmatch, REG_STARTEND);

  if (regsize)
    copy_regs (regarray, regmatch, regsize);

  return (ret == 0);
#else
  if (regex->pattern.no_sub && regsize)
    compile_regex_1 (regex, regsize);

  regex->pattern.regs_allocated = REGS_REALLOCATE;

  ret = re_search (&regex->pattern, buf, buflen, buf_start_offset,
		   buflen - buf_start_offset,
		   regsize ? regarray : NULL);

  return (ret > -1);
#endif
}


#ifdef DEBUG_LEAKS
void
release_regex(regex)
  struct regex *regex;
{
  regfree(&regex->pattern);
  FREE(regex);
}
#endif /*DEBUG_LEAKS*/
