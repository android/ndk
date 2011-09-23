/*  GNU SED, a batch stream editor.
    Copyright (C) 1989,90,91,92,93,94,95,98,99,2002,2003,2004,2005,2006,2008
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

/* compile.c: translate sed source into internal form */

#include "sed.h"
#include <stdio.h>
#include <ctype.h>

#ifdef HAVE_STRINGS_H
# include <strings.h>
# ifdef HAVE_MEMORY_H
#  include <memory.h>
# endif
#else
# include <string.h>
#endif /* HAVE_STRINGS_H */

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifndef EXIT_FAILURE
# define EXIT_FAILURE 1
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <obstack.h>


#define YMAP_LENGTH		256 /*XXX shouldn't this be (UCHAR_MAX+1)?*/
#define VECTOR_ALLOC_INCREMENT	40

/* let's not confuse text editors that have only dumb bracket-matching... */
#define OPEN_BRACKET	'['
#define CLOSE_BRACKET	']'
#define OPEN_BRACE	'{'
#define CLOSE_BRACE	'}'

struct prog_info {
  /* When we're reading a script command from a string, `prog.base'
     points to the first character in the string, 'prog.cur' points
     to the current character in the string, and 'prog.end' points
     to the end of the string.  This allows us to compile script
     strings that contain nulls. */
  const unsigned char *base;
  const unsigned char *cur;
  const unsigned char *end;

  /* This is the current script file.  If it is NULL, we are reading
     from a string stored at `prog.cur' instead.  If both `prog.file'
     and `prog.cur' are NULL, we're in trouble! */
  FILE *file;
};

/* Information used to give out useful and informative error messages. */
struct error_info {
  /* This is the name of the current script file. */
  const char *name;

  /* This is the number of the current script line that we're compiling. */
  countT line;

  /* This is the index of the "-e" expressions on the command line. */
  countT string_expr_count;
};


/* Label structure used to resolve GOTO's, labels, and block beginnings. */
struct sed_label {
  countT v_index;		/* index of vector element being referenced */
  char *name;			/* NUL-terminated name of the label */
  struct error_info err_info;	/* track where `{}' blocks start */
  struct sed_label *next;	/* linked list (stack) */
};

struct special_files {
  struct output outf;
  FILE **pfp;
};

FILE *my_stdin, *my_stdout, *my_stderr;
struct special_files special_files[] = {
  { { "/dev/stdin", false, NULL, NULL }, &my_stdin },
  { { "/dev/stdout", false, NULL, NULL }, &my_stdout },
  { { "/dev/stderr", false, NULL, NULL }, &my_stderr },
  { { NULL, false, NULL, NULL }, NULL }
};


/* Where we are in the processing of the input. */
static struct prog_info prog;
static struct error_info cur_input;

/* Information about labels and jumps-to-labels.  This is used to do
   the required backpatching after we have compiled all the scripts. */
static struct sed_label *jumps = NULL;
static struct sed_label *labels = NULL;

/* We wish to detect #n magic only in the first input argument;
   this flag tracks when we have consumed the first file of input. */
static bool first_script = true;

/* Allow for scripts like "sed -e 'i\' -e foo": */
static struct buffer *pending_text = NULL;
static struct text_buf *old_text_buf = NULL;

/* Information about block start positions.  This is used to backpatch
   block end positions. */
static struct sed_label *blocks = NULL;

/* Use an obstack for compilation. */
static struct obstack obs;

/* Various error messages we may want to print */
static const char errors[] =
  "multiple `!'s\0"
  "unexpected `,'\0"
  "invalid usage of +N or ~N as first address\0"
  "unmatched `{'\0"
  "unexpected `}'\0"
  "extra characters after command\0"
  "expected \\ after `a', `c' or `i'\0"
  "`}' doesn't want any addresses\0"
  ": doesn't want any addresses\0"
  "comments don't accept any addresses\0"
  "missing command\0"
  "command only uses one address\0"
  "unterminated address regex\0"
  "unterminated `s' command\0"
  "unterminated `y' command\0"
  "unknown option to `s'\0"
  "multiple `p' options to `s' command\0"
  "multiple `g' options to `s' command\0"
  "multiple number options to `s' command\0"
  "number option to `s' command may not be zero\0"
  "strings for `y' command are different lengths\0"
  "delimiter character is not a single-byte character\0"
  "expected newer version of sed\0"
  "invalid usage of line address 0\0"
  "unknown command: `%c'";

#define BAD_BANG (errors)
#define BAD_COMMA (BAD_BANG + sizeof(N_("multiple `!'s")))
#define BAD_STEP (BAD_COMMA + sizeof(N_("unexpected `,'")))
#define EXCESS_OPEN_BRACE (BAD_STEP + sizeof(N_("invalid usage of +N or ~N as first address")))
#define EXCESS_CLOSE_BRACE (EXCESS_OPEN_BRACE + sizeof(N_("unmatched `{'")))
#define EXCESS_JUNK (EXCESS_CLOSE_BRACE + sizeof(N_("unexpected `}'")))
#define EXPECTED_SLASH (EXCESS_JUNK + sizeof(N_("extra characters after command")))
#define NO_CLOSE_BRACE_ADDR (EXPECTED_SLASH + sizeof(N_("expected \\ after `a', `c' or `i'")))
#define NO_COLON_ADDR (NO_CLOSE_BRACE_ADDR + sizeof(N_("`}' doesn't want any addresses")))
#define NO_SHARP_ADDR (NO_COLON_ADDR + sizeof(N_(": doesn't want any addresses")))
#define NO_COMMAND (NO_SHARP_ADDR + sizeof(N_("comments don't accept any addresses")))
#define ONE_ADDR (NO_COMMAND + sizeof(N_("missing command")))
#define UNTERM_ADDR_RE (ONE_ADDR + sizeof(N_("command only uses one address")))
#define UNTERM_S_CMD (UNTERM_ADDR_RE + sizeof(N_("unterminated address regex")))
#define UNTERM_Y_CMD (UNTERM_S_CMD + sizeof(N_("unterminated `s' command")))
#define UNKNOWN_S_OPT (UNTERM_Y_CMD + sizeof(N_("unterminated `y' command")))
#define EXCESS_P_OPT (UNKNOWN_S_OPT + sizeof(N_("unknown option to `s'")))
#define EXCESS_G_OPT (EXCESS_P_OPT + sizeof(N_("multiple `p' options to `s' command")))
#define EXCESS_N_OPT (EXCESS_G_OPT + sizeof(N_("multiple `g' options to `s' command")))
#define ZERO_N_OPT (EXCESS_N_OPT + sizeof(N_("multiple number options to `s' command")))
#define Y_CMD_LEN (ZERO_N_OPT + sizeof(N_("number option to `s' command may not be zero")))
#define BAD_DELIM (Y_CMD_LEN + sizeof(N_("strings for `y' command are different lengths")))
#define ANCIENT_VERSION (BAD_DELIM + sizeof(N_("delimiter character is not a single-byte character")))
#define INVALID_LINE_0 (ANCIENT_VERSION + sizeof(N_("expected newer version of sed")))
#define UNKNOWN_CMD (INVALID_LINE_0 + sizeof(N_("invalid usage of line address 0")))
#define END_ERRORS (UNKNOWN_CMD + sizeof(N_("unknown command: `%c'")))

static struct output *file_read = NULL;
static struct output *file_write = NULL;


/* Complain about an unknown command and exit. */
void
bad_command(ch)
  char ch;
{
  const char *msg = _(UNKNOWN_CMD);
  char *unknown_cmd = xmalloc(strlen(msg));
  sprintf(unknown_cmd, msg, ch);
  bad_prog(unknown_cmd);
}

/* Complain about a programming error and exit. */
void
bad_prog(why)
  const char *why;
{
  if (cur_input.name)
    fprintf(stderr, _("%s: file %s line %lu: %s\n"),
	    myname, cur_input.name, CAST(unsigned long)cur_input.line, why);
  else
    fprintf(stderr, _("%s: -e expression #%lu, char %lu: %s\n"),
	    myname,
	    CAST(unsigned long)cur_input.string_expr_count,
	    CAST(unsigned long)(prog.cur-prog.base),
	    why);
  exit(EXIT_FAILURE);
}


/* Read the next character from the program.  Return EOF if there isn't
   anything to read.  Keep cur_input.line up to date, so error messages
   can be meaningful. */
static int inchar P_((void));
static int
inchar()
{
  int ch = EOF;

  if (prog.cur)
    {
      if (prog.cur < prog.end)
	ch = *prog.cur++;
    }
  else if (prog.file)
    {
      if (!feof(prog.file))
	ch = getc(prog.file);
    }
  if (ch == '\n')
    ++cur_input.line;
  return ch;
}

/* unget `ch' so the next call to inchar will return it.   */
static void savchar P_((int ch));
static void
savchar(ch)
  int ch;
{
  if (ch == EOF)
    return;
  if (ch == '\n' && cur_input.line > 0)
    --cur_input.line;
  if (prog.cur)
    {
      if (prog.cur <= prog.base || *--prog.cur != ch)
	panic("Called savchar() with unexpected pushback (%x)",
	      CAST(unsigned char)ch);
    }
  else
    ungetc(ch, prog.file);
}

/* Read the next non-blank character from the program.  */
static int in_nonblank P_((void));
static int
in_nonblank()
{
  int ch;
  do
    ch = inchar();
    while (ISBLANK(ch));
  return ch;
}

/* Read an integer value from the program.  */
static countT in_integer P_((int ch));
static countT
in_integer(ch)
  int ch;
{
  countT num = 0;

  while (ISDIGIT(ch))
    {
      num = num * 10 + ch - '0';
      ch = inchar();
    }
  savchar(ch);
  return num;
}

static int add_then_next P_((struct buffer *b, int ch));
static int
add_then_next(b, ch)
  struct buffer *b;
  int ch;
{
  add1_buffer(b, ch);
  return inchar();
}

static char * convert_number P_((char *, char *, const char *, int, int, int));
static char *
convert_number(result, buf, bufend, base, maxdigits, default_char)
  char *result;
  char *buf;
  const char *bufend;
  int base;
  int maxdigits;
  int default_char;
{
  int n = 0;
  char *p;

  for (p=buf; p < bufend && maxdigits-- > 0; ++p)
    {
      int d = -1;
      switch (*p)
	{
	case '0': d = 0x0; break;
	case '1': d = 0x1; break;
	case '2': d = 0x2; break;
	case '3': d = 0x3; break;
	case '4': d = 0x4; break;
	case '5': d = 0x5; break;
	case '6': d = 0x6; break;
	case '7': d = 0x7; break;
	case '8': d = 0x8; break;
	case '9': d = 0x9; break;
	case 'A': case 'a': d = 0xa; break;
	case 'B': case 'b': d = 0xb; break;
	case 'C': case 'c': d = 0xc; break;
	case 'D': case 'd': d = 0xd; break;
	case 'E': case 'e': d = 0xe; break;
	case 'F': case 'f': d = 0xf; break;
	}
      if (d < 0 || base <= d)
	break;
      n = n * base + d;
    }
  if (p == buf)
    *result = default_char;
  else
    *result = n;
  return p;
}


/* Read in a filename for a `r', `w', or `s///w' command. */
static struct buffer *read_filename P_((void));
static struct buffer *
read_filename()
{
  struct buffer *b;
  int ch;

  b = init_buffer();
  ch = in_nonblank();
  while (ch != EOF && ch != '\n')
    {
#if 0 /*XXX ZZZ 1998-09-12 kpp: added, then had second thoughts*/
      if (posixicity == POSIXLY_EXTENDED)
	if (ch == ';' || ch == '#')
	  {
	    savchar(ch);
	    break;
	  }
#endif
      ch = add_then_next(b, ch);
    }
  add1_buffer(b, '\0');
  return b;
}

static struct output *get_openfile P_((struct output **file_ptrs, const char *mode, int fail));
static struct output *
get_openfile(file_ptrs, mode, fail)
     struct output **file_ptrs;
     const char *mode;
     int fail;
{
  struct buffer *b;
  char *file_name;
  struct output *p;

  b = read_filename();
  file_name = get_buffer(b);
  for (p=*file_ptrs; p; p=p->link)
    if (strcmp(p->name, file_name) == 0)
      break;

  if (posixicity == POSIXLY_EXTENDED)
    {
      /* Check whether it is a special file (stdin, stdout or stderr) */
      struct special_files *special = special_files;
		  
      /* std* sometimes are not constants, so they
         cannot be used in the initializer for special_files */
      my_stdin = stdin; my_stdout = stdout; my_stderr = stderr;
      for (special = special_files; special->outf.name; special++)
        if (strcmp(special->outf.name, file_name) == 0)
          {
	    special->outf.fp = *special->pfp;
	    free_buffer (b);
	    return &special->outf;
          }
    }

  if (!p)
    {
      p = OB_MALLOC(&obs, 1, struct output);
      p->name = ck_strdup(file_name);
      p->fp = ck_fopen(p->name, mode, fail);
      p->missing_newline = false;
      p->link = *file_ptrs;
      *file_ptrs = p;
    }
  free_buffer(b);
  return p;
}


static struct sed_cmd *next_cmd_entry P_((struct vector **vectorp));
static struct sed_cmd *
next_cmd_entry(vectorp)
  struct vector **vectorp;
{
  struct sed_cmd *cmd;
  struct vector *v;

  v = *vectorp;
  if (v->v_length == v->v_allocated)
    {
      v->v_allocated += VECTOR_ALLOC_INCREMENT;
      v->v = REALLOC(v->v, v->v_allocated, struct sed_cmd);
    }

  cmd = v->v + v->v_length;
  cmd->a1 = NULL;
  cmd->a2 = NULL;
  cmd->range_state = RANGE_INACTIVE;
  cmd->addr_bang = false;
  cmd->cmd = '\0';	/* something invalid, to catch bugs early */

  *vectorp  = v;
  return cmd;
}

static int snarf_char_class P_((struct buffer *b, mbstate_t *cur_stat));
static int
snarf_char_class(b, cur_stat)
  struct buffer *b;
  mbstate_t *cur_stat;
{
  int ch;
  int state = 0;
  int delim;
  bool pending_mb = 0;
  
  ch = inchar();
  if (ch == '^')
    ch = add_then_next(b, ch);
  if (ch == CLOSE_BRACKET)
    ch = add_then_next(b, ch);
  
  /* States are:
	0 outside a collation element, character class or collation class
	1 after the bracket
	2 after the opening ./:/=
	3 after the closing ./:/= */

  for (;; ch = add_then_next (b, ch))
    {
      pending_mb = BRLEN (ch, cur_stat) != 1;

      switch (ch)
	{
	case EOF:
	case '\n':
	  return ch;

	case '.':
	case ':':
	case '=':
	  if (pending_mb)
	    continue;

	  if (state == 1)
	    {
	      delim = ch;
	      state = 2;
	    }
	  else if (state == 2 && ch == delim)
	    state = 3;
	  else
	    break;

	  continue;

	case OPEN_BRACKET:
	  if (pending_mb)
	    continue;

	  if (state == 0)
	    state = 1;
	  continue;

	case CLOSE_BRACKET:
	  if (pending_mb)
	    continue;

	  if (state == 0 || state == 1)
	    return ch;
	  else if (state == 3)
	    state = 0;

	  break;

	default:
	  break;
	}

      /* Getting a character different from .=: whilst in state 1
         goes back to state 0, getting a character different from ]
         whilst in state 3 goes back to state 2.  */
      state &= ~1;
    }
}

static struct buffer *match_slash P_((int slash, int regex));
static struct buffer *
match_slash(slash, regex)
  int slash;
  int regex;
{
  struct buffer *b;
  int ch;
  mbstate_t cur_stat;

  memset (&cur_stat, 0, sizeof (mbstate_t));

  /* We allow only 1 byte characters for a slash.  */
  if (BRLEN (slash, &cur_stat) == -2)
    bad_prog (BAD_DELIM);

  memset (&cur_stat, 0, sizeof (mbstate_t));

  b = init_buffer();
  while ((ch = inchar()) != EOF && ch != '\n')
    {
      bool pending_mb = !MBSINIT (&cur_stat);
      if (BRLEN (ch, &cur_stat) == 1 && !pending_mb)
	{
	  if (ch == slash)
	    return b;
	  else if (ch == '\\')
	    {
	      ch = inchar();
	      if (ch == EOF)
	        break;
#ifndef REG_PERL
	      else if (ch == 'n' && regex)
	        ch = '\n';
#endif
	      else if (ch != '\n' && (ch != slash || (!regex && ch == '&')))
	        add1_buffer(b, '\\');
	    }
          else if (ch == OPEN_BRACKET && regex)
	    {
	      add1_buffer(b, ch);
	      ch = snarf_char_class(b, &cur_stat);
	      if (ch != CLOSE_BRACKET)
	        break;
	    }
	}

      add1_buffer(b, ch);
    }

  if (ch == '\n')
    savchar(ch);	/* for proper line number in error report */
  free_buffer(b);
  return NULL;
}

static int mark_subst_opts P_((struct subst *cmd));
static int
mark_subst_opts(cmd)
  struct subst *cmd;
{
  int flags = 0;
  int ch;

  cmd->global = false;
  cmd->print = false;
  cmd->eval = false;
  cmd->numb = 0;
  cmd->outf = NULL;

  for (;;)
    switch ( (ch = in_nonblank()) )
      {
      case 'i':	/* GNU extension */
      case 'I':	/* GNU extension */
	if (posixicity == POSIXLY_BASIC)
	  bad_prog(_(UNKNOWN_S_OPT));
	flags |= REG_ICASE;
	break;

#ifdef REG_PERL
      case 's':	/* GNU extension */
      case 'S':	/* GNU extension */
	if (posixicity == POSIXLY_BASIC)
	  bad_prog(_(UNKNOWN_S_OPT));
	if (extended_regexp_flags & REG_PERL)
	  flags |= REG_DOTALL;
	break;

      case 'x':	/* GNU extension */
      case 'X':	/* GNU extension */
	if (posixicity == POSIXLY_BASIC)
	  bad_prog(_(UNKNOWN_S_OPT));
	if (extended_regexp_flags & REG_PERL)
	  flags |= REG_EXTENDED;
	break;
#endif

      case 'm':	/* GNU extension */
      case 'M':	/* GNU extension */
	if (posixicity == POSIXLY_BASIC)
	  bad_prog(_(UNKNOWN_S_OPT));
	flags |= REG_NEWLINE;
	break;

      case 'e':
	cmd->eval = true;
	break;

      case 'p':
	if (cmd->print)
	  bad_prog(_(EXCESS_P_OPT));
	cmd->print |= (1 << cmd->eval); /* 1=before eval, 2=after */
	break;

      case 'g':
	if (cmd->global)
	  bad_prog(_(EXCESS_G_OPT));
	cmd->global = true;
	break;

      case 'w':
	cmd->outf = get_openfile(&file_write, "w", true);
	return flags;

      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
	if (cmd->numb)
	  bad_prog(_(EXCESS_N_OPT));
	cmd->numb = in_integer(ch);
	if (!cmd->numb)
	  bad_prog(_(ZERO_N_OPT));
	break;

      case CLOSE_BRACE:
      case '#':
	savchar(ch);
	/* Fall Through */
      case EOF:
      case '\n':
      case ';':
	return flags;

      case '\r':
	if (inchar() == '\n')
	  return flags;
	/* FALLTHROUGH */

      default:
	bad_prog(_(UNKNOWN_S_OPT));
	/*NOTREACHED*/
      }
}


/* read in a label for a `:', `b', or `t' command */
static char *read_label P_((void));
static char *
read_label()
{
  struct buffer *b;
  int ch;
  char *ret;

  b = init_buffer();
  ch = in_nonblank();

  while (ch != EOF && ch != '\n'
	 && !ISBLANK(ch) && ch != ';' && ch != CLOSE_BRACE && ch != '#')
    ch = add_then_next (b, ch);

  savchar(ch);
  add1_buffer(b, '\0');
  ret = ck_strdup(get_buffer(b));
  free_buffer(b);
  return ret;
}

/* Store a label (or label reference) created by a `:', `b', or `t'
   command so that the jump to/from the label can be backpatched after
   compilation is complete, or a reference created by a `{' to be
   backpatched when the corresponding `}' is found.  */
static struct sed_label *setup_label
  P_((struct sed_label *, countT, char *, const struct error_info *));
static struct sed_label *
setup_label(list, idx, name, err_info)
  struct sed_label *list;
  countT idx;
  char *name;
  const struct error_info *err_info;
{
  struct sed_label *ret = OB_MALLOC(&obs, 1, struct sed_label);
  ret->v_index = idx;
  ret->name = name;
  if (err_info)
    MEMCPY(&ret->err_info, err_info, sizeof (ret->err_info));
  ret->next = list;
  return ret;
}

static struct sed_label *release_label P_((struct sed_label *list_head));
static struct sed_label *
release_label(list_head)
  struct sed_label *list_head;
{
  struct sed_label *ret;

  if (!list_head)
    return NULL;
  ret = list_head->next;

  FREE(list_head->name);

#if 0
  /* We use obstacks */
  FREE(list_head);
#endif
  return ret;
}

static struct replacement *new_replacement P_((char *, size_t,
					       enum replacement_types));
static struct replacement *
new_replacement(text, length, type)
  char *text;
  size_t length;
  enum replacement_types type;
{
  struct replacement *r = OB_MALLOC(&obs, 1, struct replacement);

  r->prefix = text;
  r->prefix_length = length;
  r->subst_id = -1;
  r->repl_type = type;

  /* r-> next = NULL; */
  return r;
}

static void setup_replacement P_((struct subst *, const char *, size_t));
static void
setup_replacement(sub, text, length)
     struct subst *sub;
     const char *text;
     size_t length;
{
  char *base;
  char *p;
  char *text_end;
  enum replacement_types repl_type = REPL_ASIS, save_type = REPL_ASIS;
  struct replacement root;
  struct replacement *tail;

  sub->max_id = 0;
  base = MEMDUP(text, length, char);
  length = normalize_text(base, length, TEXT_REPLACEMENT);

  text_end = base + length;
  tail = &root;

  for (p=base; p<text_end; ++p)
    {
      if (*p == '\\')
	{
	  /* Preceding the backslash may be some literal text: */
	  tail = tail->next =
	    new_replacement(base, CAST(size_t)(p - base), repl_type);

	  repl_type = save_type;

	  /* Skip the backslash and look for a numeric back-reference,
	     or a case-munging escape if not in POSIX mode: */
	  ++p;
	  if (p == text_end)
	    ++tail->prefix_length;

	  else if (posixicity == POSIXLY_BASIC && !ISDIGIT (*p))
	    {
	      p[-1] = *p;
	      ++tail->prefix_length;
	    }

	  else
	    switch (*p)
	      {
	      case '0': case '1': case '2': case '3': case '4': 
	      case '5': case '6': case '7': case '8': case '9': 
		tail->subst_id = *p - '0';
		if (sub->max_id < tail->subst_id)
		  sub->max_id = tail->subst_id;
		break;

	      case 'L':
		repl_type = REPL_LOWERCASE;
		save_type = REPL_LOWERCASE;
		break;

	      case 'U':
		repl_type = REPL_UPPERCASE;
		save_type = REPL_UPPERCASE;
		break;
		
	      case 'E':
		repl_type = REPL_ASIS;
		save_type = REPL_ASIS;
		break;

	      case 'l':
		save_type = repl_type;
		repl_type |= REPL_LOWERCASE_FIRST;
		break;

	      case 'u':
		save_type = repl_type;
		repl_type |= REPL_UPPERCASE_FIRST;
		break;
		
	      default:
		p[-1] = *p;
		++tail->prefix_length;
	      }

	  base = p + 1;
	}
      else if (*p == '&')
	{
	  /* Preceding the ampersand may be some literal text: */
	  tail = tail->next =
	    new_replacement(base, CAST(size_t)(p - base), repl_type);

	  repl_type = save_type;
	  tail->subst_id = 0;
	  base = p + 1;
	}
  }
  /* There may be some trailing literal text: */
  if (base < text_end)
    tail = tail->next =
      new_replacement(base, CAST(size_t)(text_end - base), repl_type);

  tail->next = NULL;
  sub->replacement = root.next;
}

static void read_text P_((struct text_buf *buf, int leadin_ch));
static void
read_text(buf, leadin_ch)
  struct text_buf *buf;
  int leadin_ch;
{
  int ch;

  /* Should we start afresh (as opposed to continue a partial text)? */
  if (buf)
    {
      if (pending_text)
	free_buffer(pending_text);
      pending_text = init_buffer();
      buf->text = NULL;
      buf->text_length = 0;
      old_text_buf = buf;
    }
  /* assert(old_text_buf != NULL); */

  if (leadin_ch == EOF)
    return;

  if (leadin_ch != '\n')
    add1_buffer(pending_text, leadin_ch);

  ch = inchar();
  while (ch != EOF && ch != '\n')
    {
      if (ch == '\\')
	{
	  ch = inchar();
	  if (ch != EOF)
	    add1_buffer (pending_text, '\\');
	}

      if (ch == EOF)
	{
	  add1_buffer (pending_text, '\n');
	  return;
	}

      ch = add_then_next (pending_text, ch);
    }

  add1_buffer(pending_text, '\n');
  if (!buf)
    buf = old_text_buf;
  buf->text_length = normalize_text (get_buffer (pending_text),
				     size_buffer (pending_text), TEXT_BUFFER);
  buf->text = MEMDUP(get_buffer(pending_text), buf->text_length, char);
  free_buffer(pending_text);
  pending_text = NULL;
}


/* Try to read an address for a sed command.  If it succeeds,
   return non-zero and store the resulting address in `*addr'.
   If the input doesn't look like an address read nothing
   and return zero.  */
static bool compile_address P_((struct addr *addr, int ch));
static bool
compile_address(addr, ch)
  struct addr *addr;
  int ch;
{
  addr->addr_type = ADDR_IS_NULL;
  addr->addr_step = 0;
  addr->addr_number = ~(countT)0;  /* extremely unlikely to ever match */
  addr->addr_regex = NULL;

  if (ch == '/' || ch == '\\')
    {
      int flags = 0;
      struct buffer *b;
      addr->addr_type = ADDR_IS_REGEX;
      if (ch == '\\')
	ch = inchar();
      if ( !(b = match_slash(ch, true)) )
	bad_prog(_(UNTERM_ADDR_RE));

      for(;;)
	{
	  ch = in_nonblank();
	  if (posixicity == POSIXLY_BASIC)
	    goto posix_address_modifier;
          switch(ch)
	    {
	    case 'I':	/* GNU extension */
	      flags |= REG_ICASE;
	      break;

#ifdef REG_PERL
	    case 'S':	/* GNU extension */
	      if (extended_regexp_flags & REG_PERL)
		flags |= REG_DOTALL;
	      break;

	    case 'X':	/* GNU extension */
	      if (extended_regexp_flags & REG_PERL)
		flags |= REG_EXTENDED;
	      break;
#endif

	    case 'M':	/* GNU extension */
	      flags |= REG_NEWLINE;
	      break;

	    default:
	    posix_address_modifier:
	      savchar (ch);
	      addr->addr_regex = compile_regex (b, flags, 0);
	      free_buffer(b);
	      return true;
	    }
	}
    }
  else if (ISDIGIT(ch))
    {
      addr->addr_number = in_integer(ch);
      addr->addr_type = ADDR_IS_NUM;
      ch = in_nonblank();
      if (ch != '~' || posixicity == POSIXLY_BASIC)
	{
	  savchar(ch);
	}
      else
	{
	  countT step = in_integer(in_nonblank());
	  if (step > 0)
	    {
	      addr->addr_step = step;
	      addr->addr_type = ADDR_IS_NUM_MOD;
	    }
	}
    }
  else if ((ch == '+' || ch == '~') && posixicity != POSIXLY_BASIC)
    {
      addr->addr_step = in_integer(in_nonblank());
      if (addr->addr_step==0)
	; /* default to ADDR_IS_NULL; forces matching to stop on next line */
      else if (ch == '+')
	addr->addr_type = ADDR_IS_STEP;
      else
	addr->addr_type = ADDR_IS_STEP_MOD;
    }
  else if (ch == '$')
    {
      addr->addr_type = ADDR_IS_LAST;
    }
  else
    return false;

  return true;
}

/* Read a program (or a subprogram within `{' `}' pairs) in and store
   the compiled form in `*vector'.  Return a pointer to the new vector.  */
static struct vector *compile_program P_((struct vector *));
static struct vector *
compile_program(vector)
  struct vector *vector;
{
  struct sed_cmd *cur_cmd;
  struct buffer *b;
  int ch;

  if (!vector)
    {
      vector = MALLOC(1, struct vector);
      vector->v = NULL;
      vector->v_allocated = 0;
      vector->v_length = 0;

      obstack_init (&obs);
    }
  if (pending_text)
    read_text(NULL, '\n');

  for (;;)
    {
      struct addr a;

      while ((ch=inchar()) == ';' || ISSPACE(ch))
	;
      if (ch == EOF)
	break;

      cur_cmd = next_cmd_entry(&vector);
      if (compile_address(&a, ch))
	{
	  if (a.addr_type == ADDR_IS_STEP
	      || a.addr_type == ADDR_IS_STEP_MOD)
	    bad_prog(_(BAD_STEP));

	  cur_cmd->a1 = MEMDUP(&a, 1, struct addr);
	  ch = in_nonblank();
	  if (ch == ',')
	    {
	      if (!compile_address(&a, in_nonblank()))
		bad_prog(_(BAD_COMMA));

	      cur_cmd->a2 = MEMDUP(&a, 1, struct addr);
	      ch = in_nonblank();
	    }

	  if ((cur_cmd->a1->addr_type == ADDR_IS_NUM
	       && cur_cmd->a1->addr_number == 0)
	      && ((!cur_cmd->a2 || cur_cmd->a2->addr_type != ADDR_IS_REGEX)
		  || posixicity == POSIXLY_BASIC))
	    bad_prog(_(INVALID_LINE_0));
	}
      if (ch == '!')
	{
	  cur_cmd->addr_bang = true;
	  ch = in_nonblank();
	  if (ch == '!')
	    bad_prog(_(BAD_BANG));
	}

      /* Do not accept extended commands in --posix mode.  Also,
	 a few commands only accept one address in that mode.  */
      if (posixicity == POSIXLY_BASIC)
	switch (ch)
	  {
	    case 'e': case 'v': case 'z': case 'L':
	    case 'Q': case 'T': case 'R': case 'W':
	      bad_command(ch);

	    case 'a': case 'i': case 'l':
	    case '=': case 'r':
	      if (cur_cmd->a2)
	        bad_prog(_(ONE_ADDR));
	  }

      cur_cmd->cmd = ch;
      switch (ch)
	{
	case '#':
	  if (cur_cmd->a1)
	    bad_prog(_(NO_SHARP_ADDR));
	  ch = inchar();
	  if (ch=='n' && first_script && cur_input.line < 2)
	    if (   (prog.base && prog.cur==2+prog.base)
		|| (prog.file && !prog.base && 2==ftell(prog.file)))
	      no_default_output = true;
	  while (ch != EOF && ch != '\n')
	    ch = inchar();
	  continue;	/* restart the for (;;) loop */

	case 'v':
	  /* This is an extension.  Programs needing GNU sed might start
	   * with a `v' command so that other seds will stop.
	   * We compare the version and ignore POSIXLY_CORRECT.
	   */
	  {
	    char *version = read_label ();
	    char *compared_version;
	    compared_version = (*version == '\0') ? "4.0" : version;
	    if (strverscmp (compared_version, SED_FEATURE_VERSION) > 0)
	      bad_prog(_(ANCIENT_VERSION));

	    free (version);
	    posixicity = POSIXLY_EXTENDED;
	  }
	  continue;

	case '{':
	  blocks = setup_label(blocks, vector->v_length, NULL, &cur_input);
	  cur_cmd->addr_bang = !cur_cmd->addr_bang;
	  break;

	case '}':
	  if (!blocks)
	    bad_prog(_(EXCESS_CLOSE_BRACE));
	  if (cur_cmd->a1)
	    bad_prog(_(NO_CLOSE_BRACE_ADDR));
	  ch = in_nonblank();
	  if (ch == CLOSE_BRACE || ch == '#')
	    savchar(ch);
	  else if (ch != EOF && ch != '\n' && ch != ';')
	    bad_prog(_(EXCESS_JUNK));

	  vector->v[blocks->v_index].x.jump_index = vector->v_length;
	  blocks = release_label(blocks);	/* done with this entry */
	  break;

	case 'e':
	  ch = in_nonblank();
	  if (ch == EOF || ch == '\n')
	    {
	      cur_cmd->x.cmd_txt.text_length = 0;
	      break;
	    }
	  else
	    goto read_text_to_slash;

	case 'a':
	case 'i':
	case 'c':
	  ch = in_nonblank();

	read_text_to_slash:
	  if (ch == EOF)
	    bad_prog(_(EXPECTED_SLASH));
	      
	  if (ch == '\\')
	    ch = inchar();
	  else
	    {
	      if (posixicity == POSIXLY_BASIC)
		bad_prog(_(EXPECTED_SLASH));
	      savchar(ch);
	      ch = '\n';
	    }

	  read_text(&cur_cmd->x.cmd_txt, ch);
	  break;

	case ':':
	  if (cur_cmd->a1)
	    bad_prog(_(NO_COLON_ADDR));
	  labels = setup_label(labels, vector->v_length, read_label(), NULL);
	  break;
	
	case 'T':
	case 'b':
	case 't':
	  jumps = setup_label(jumps, vector->v_length, read_label(), NULL);
	  break;

	case 'Q':
	case 'q':
	  if (cur_cmd->a2)
	    bad_prog(_(ONE_ADDR));
	  /* Fall through */

	case 'L':
	case 'l':
	  ch = in_nonblank();
	  if (ISDIGIT(ch) && posixicity != POSIXLY_BASIC)
	    {
	      cur_cmd->x.int_arg = in_integer(ch);
	      ch = in_nonblank();
	    }
	  else
	    cur_cmd->x.int_arg = -1;

	  if (ch == CLOSE_BRACE || ch == '#')
	    savchar(ch);
	  else if (ch != EOF && ch != '\n' && ch != ';')
	    bad_prog(_(EXCESS_JUNK));

	  break;

	case '=':
	case 'd':
	case 'D':
	case 'g':
	case 'G':
	case 'h':
	case 'H':
	case 'n':
	case 'N':
	case 'p':
	case 'P':
	case 'z':
	case 'x':
	  ch = in_nonblank();
	  if (ch == CLOSE_BRACE || ch == '#')
	    savchar(ch);
	  else if (ch != EOF && ch != '\n' && ch != ';')
	    bad_prog(_(EXCESS_JUNK));
	  break;

	case 'r':
	  b = read_filename();
	  cur_cmd->x.fname = ck_strdup(get_buffer(b));
	  free_buffer(b);
	  break;

        case 'R':
	  cur_cmd->x.fp = get_openfile(&file_read, read_mode, false)->fp;
	  break;

        case 'W':
	case 'w':
	  cur_cmd->x.outf = get_openfile(&file_write, "w", true);
	  break;

	case 's':
	  {
	    struct buffer *b2;
	    int flags;
	    int slash;

	    slash = inchar();
	    if ( !(b  = match_slash(slash, true)) )
	      bad_prog(_(UNTERM_S_CMD));
	    if ( !(b2 = match_slash(slash, false)) )
	      bad_prog(_(UNTERM_S_CMD));

	    cur_cmd->x.cmd_subst = OB_MALLOC(&obs, 1, struct subst);
	    setup_replacement(cur_cmd->x.cmd_subst,
			      get_buffer(b2), size_buffer(b2));
	    free_buffer(b2);

	    flags = mark_subst_opts(cur_cmd->x.cmd_subst);
	    cur_cmd->x.cmd_subst->regx =
	      compile_regex(b, flags, cur_cmd->x.cmd_subst->max_id + 1);
	    free_buffer(b);
	  }
	  break;

	case 'y':
	  {
	    size_t len, dest_len;
	    int slash;
	    struct buffer *b2;
            char *src_buf, *dest_buf;
  
	    slash = inchar();
	    if ( !(b = match_slash(slash, false)) )
	      bad_prog(_(UNTERM_Y_CMD));
            src_buf = get_buffer(b);
	    len = normalize_text(src_buf, size_buffer (b), TEXT_BUFFER);

            if ( !(b2 = match_slash(slash, false)) )
 	      bad_prog(_(UNTERM_Y_CMD));
            dest_buf = get_buffer(b2);
	    dest_len = normalize_text(dest_buf, size_buffer (b2), TEXT_BUFFER);

            if (mb_cur_max > 1)
	      {
                int i, j, idx, src_char_num;
                size_t *src_lens = MALLOC(len, size_t);
                char **trans_pairs;
                size_t mbclen;
                mbstate_t cur_stat;

                /* Enumerate how many character the source buffer has.  */
                memset(&cur_stat, 0, sizeof(mbstate_t));
                for (i = 0, j = 0; i < len;)
                  {
                    mbclen = MBRLEN (src_buf + i, len - i, &cur_stat);
                    /* An invalid sequence, or a truncated multibyte character.
                       We treat it as a singlebyte character.  */
                    if (mbclen == (size_t) -1 || mbclen == (size_t) -2
                        || mbclen == 0)
                      mbclen = 1;
                    src_lens[j++] = mbclen;
                    i += mbclen;
                  }
                src_char_num = j;

                memset(&cur_stat, 0, sizeof(mbstate_t));
                idx = 0;

                /* trans_pairs = {src(0), dest(0), src(1), dest(1), ..., NULL}
                     src(i) : pointer to i-th source character.
                     dest(i) : pointer to i-th destination character.
                     NULL : terminator */
                trans_pairs = MALLOC(2 * src_char_num + 1, char*);
                cur_cmd->x.translatemb = trans_pairs;
                for (i = 0; i < src_char_num; i++)
                  {
                    if (idx >= dest_len)
                      bad_prog(_(Y_CMD_LEN));

                    /* Set the i-th source character.  */
                    trans_pairs[2 * i] = MALLOC(src_lens[i] + 1, char);
                    strncpy(trans_pairs[2 * i], src_buf, src_lens[i]);
                    trans_pairs[2 * i][src_lens[i]] = '\0';
                    src_buf += src_lens[i]; /* Forward to next character.  */

                    /* Fetch the i-th destination character.  */
                    mbclen = MBRLEN (dest_buf + idx, dest_len - idx, &cur_stat);
                    /* An invalid sequence, or a truncated multibyte character.
                       We treat it as a singlebyte character.  */
                    if (mbclen == (size_t) -1 || mbclen == (size_t) -2
                        || mbclen == 0)
                      mbclen = 1;

                    /* Set the i-th destination character.  */
                    trans_pairs[2 * i + 1] = MALLOC(mbclen + 1, char);
                    strncpy(trans_pairs[2 * i + 1], dest_buf + idx, mbclen);
                    trans_pairs[2 * i + 1][mbclen] = '\0';
                    idx += mbclen; /* Forward to next character.  */
                  }
                trans_pairs[2 * i] = NULL;
                if (idx != dest_len)
                  bad_prog(_(Y_CMD_LEN));
              }
            else
              {
	        unsigned char *translate =
		  OB_MALLOC(&obs, YMAP_LENGTH, unsigned char);
                unsigned char *ustring = CAST(unsigned char *)src_buf;

		if (len != dest_len)
                  bad_prog(_(Y_CMD_LEN));

	        for (len = 0; len < YMAP_LENGTH; len++)
	          translate[len] = len;

                while (dest_len--)
                  translate[*ustring++] = (unsigned char)*dest_buf++;

	        cur_cmd->x.translate = translate;
	      }

            if ((ch = in_nonblank()) != EOF && ch != '\n' && ch != ';')
              bad_prog(_(EXCESS_JUNK));

            free_buffer(b);
            free_buffer(b2);
	  }
	break;

	case EOF:
	  bad_prog(_(NO_COMMAND));
	  /*NOTREACHED*/

	default:
	  bad_command (ch);
	  /*NOTREACHED*/
	}

      /* this is buried down here so that "continue" statements will miss it */
      ++vector->v_length;
    }
  return vector;
}


/* deal with \X escapes */
size_t
normalize_text(buf, len, buftype)
  char *buf;
  size_t len;
  enum text_types buftype;
{
  const char *bufend = buf + len;
  char *p = buf;
  char *q = buf;

  /* This variable prevents normalizing text within bracket
     subexpressions when conforming to POSIX.  If 0, we
     are not within a bracket expression.  If -1, we are within a
     bracket expression but are not within [.FOO.], [=FOO=],
     or [:FOO:].  Otherwise, this is the '.', '=', or ':'
     respectively within these three types of subexpressions.  */
  int bracket_state = 0;

  int mbclen;
  mbstate_t cur_stat;
  memset(&cur_stat, 0, sizeof(mbstate_t));

  while (p < bufend)
    {
      int c;
      mbclen = MBRLEN (p, bufend - p, &cur_stat);
      if (mbclen != 1)
	{
          /* An invalid sequence, or a truncated multibyte character.
             We treat it as a singlebyte character.  */
          if (mbclen == (size_t) -1 || mbclen == (size_t) -2 || mbclen == 0)
            mbclen = 1;

          memmove (q, p, mbclen);
          q += mbclen;
          p += mbclen;
	  continue;
	}

      if (*p == '\\' && p+1 < bufend && bracket_state == 0)
	switch ( (c = *++p) )
	  {
#if defined __STDC__ && __STDC__-0
	  case 'a': *q++ = '\a'; p++; continue;
#else /* Not STDC; we'll just assume ASCII */
	  case 'a': *q++ = '\007'; p++; continue;
#endif
	  /* case 'b': *q++ = '\b'; p++; continue; --- conflicts with \b RE */
	  case 'f': *q++ = '\f'; p++; continue;
	  case '\n': /*fall through */
	  case 'n': *q++ = '\n'; p++; continue;
	  case 'r': *q++ = '\r'; p++; continue;
	  case 't': *q++ = '\t'; p++; continue;
	  case 'v': *q++ = '\v'; p++; continue;

	  case 'd': /* decimal byte */
	    p = convert_number(q, p+1, bufend, 10, 3, 'd');
	    q++;
	    continue;

	  case 'x': /* hexadecimal byte */
	    p = convert_number(q, p+1, bufend, 16, 2, 'x');
	    q++;
	    continue;

#ifdef REG_PERL
	  case '0': case '1': case '2': case '3':
	  case '4': case '5': case '6': case '7':
	    if ((extended_regexp_flags & REG_PERL)
		&& p+1 < bufend
		&& p[1] >= '0' && p[1] <= '9')
	      {
		p = convert_number(q, p, bufend, 8, 3, *p);
		q++;
	      }
	    else
	      {
		/* we just pass the \ up one level for interpretation */
	        if (buftype != TEXT_BUFFER)
		  *q++ = '\\';
	      }

	    continue;

	  case 'o': /* octal byte */
	    if (!(extended_regexp_flags & REG_PERL))
	      {
	        p = convert_number(q, p+1, bufend,  8, 3, 'o');
		q++;
	      }
	    else
	      {
	        /* we just pass the \ up one level for interpretation */
	        if (buftype != TEXT_BUFFER)
		  *q++ = '\\';
	      }
	    
	    continue;
#else
	  case 'o': /* octal byte */
	    p = convert_number(q, p+1, bufend,  8, 3, 'o');
	    q++;
	    continue;
#endif

	  case 'c':
	    if (++p < bufend)
	      {
		*q++ = toupper(*p) ^ 0x40;
		p++;
		continue;
	      }
	    else
	      {
	        /* we just pass the \ up one level for interpretation */
	        if (buftype != TEXT_BUFFER)
		  *q++ = '\\';
	        continue;
	      }

	  default:
	    /* we just pass the \ up one level for interpretation */
	    if (buftype != TEXT_BUFFER)
	      *q++ = '\\';
	    break;
	  }
      else if (buftype == TEXT_REGEX && posixicity != POSIXLY_EXTENDED)
        switch (*p)
          {
          case '[':
            if (!bracket_state)
              bracket_state = -1;
            break;
 
	  case ':':
	  case '.':
	  case '=':
            if (bracket_state == -1 && p[-1] == '[')
              bracket_state = *p;
            break;

          case ']':
            if (bracket_state == 0)
	      ;
            else if (bracket_state == -1)
              bracket_state = 0;
            else if (p[-2] != bracket_state && p[-1] == bracket_state)
              bracket_state = -1;
            break;
          }

      *q++ = *p++;
    }
    return (size_t)(q - buf);
}


/* `str' is a string (from the command line) that contains a sed command.
   Compile the command, and add it to the end of `cur_program'. */
struct vector *
compile_string(cur_program, str, len)
  struct vector *cur_program;
  char *str;
  size_t len;
{
  static countT string_expr_count = 0;
  struct vector *ret;

  prog.file = NULL;
  prog.base = CAST(unsigned char *)str;
  prog.cur = prog.base;
  prog.end = prog.cur + len;

  cur_input.line = 0;
  cur_input.name = NULL;
  cur_input.string_expr_count = ++string_expr_count;

  ret = compile_program(cur_program);
  prog.base = NULL;
  prog.cur = NULL;
  prog.end = NULL;

  first_script = false;
  return ret;
}

/* `cmdfile' is the name of a file containing sed commands.
   Read them in and add them to the end of `cur_program'.
 */
struct vector *
compile_file(cur_program, cmdfile)
  struct vector *cur_program;
  const char *cmdfile;
{
  struct vector *ret;

  prog.file = stdin;
  if (cmdfile[0] != '-' || cmdfile[1] != '\0')
    prog.file = ck_fopen(cmdfile, "rt", true);

  cur_input.line = 1;
  cur_input.name = cmdfile;
  cur_input.string_expr_count = 0;

  ret = compile_program(cur_program);
  if (prog.file != stdin)
    ck_fclose(prog.file);
  prog.file = NULL;

  first_script = false;
  return ret;
}

/* Make any checks which require the whole program to have been read.
   In particular: this backpatches the jump targets.
   Any cleanup which can be done after these checks is done here also.  */
void
check_final_program(program)
  struct vector *program;
{
  struct sed_label *go;
  struct sed_label *lbl;

  /* do all "{"s have a corresponding "}"? */
  if (blocks)
    {
      /* update info for error reporting: */
      MEMCPY(&cur_input, &blocks->err_info, sizeof (cur_input));
      bad_prog(_(EXCESS_OPEN_BRACE));
    }

  /* was the final command an unterminated a/c/i command? */
  if (pending_text)
    {
      old_text_buf->text_length = size_buffer(pending_text);
      if (old_text_buf->text_length)
        old_text_buf->text = MEMDUP(get_buffer(pending_text),
				    old_text_buf->text_length, char);
      free_buffer(pending_text);
      pending_text = NULL;
    }

  for (go = jumps; go; go = release_label(go))
    {
      for (lbl = labels; lbl; lbl = lbl->next)
	if (strcmp(lbl->name, go->name) == 0)
	  break;
      if (lbl)
	{
	  program->v[go->v_index].x.jump_index = lbl->v_index;
	}
      else
	{
	  if (*go->name)
	    panic(_("can't find label for jump to `%s'"), go->name);
	  program->v[go->v_index].x.jump_index = program->v_length;
	}
    }
  jumps = NULL;

  for (lbl = labels; lbl; lbl = release_label(lbl))
    ;
  labels = NULL;

  /* There is no longer a need to track file names: */
  {
    struct output *p;

    for (p=file_read; p; p=p->link)
      if (p->name)
	{
	  FREE(p->name);
	  p->name = NULL;
	}

    for (p=file_write; p; p=p->link)
      if (p->name)
	{
	  FREE(p->name);
	  p->name = NULL;
	}
  }
}

/* Rewind all resources which were allocated in this module. */
void
rewind_read_files()
{
  struct output *p;

  for (p=file_read; p; p=p->link)
    if (p->fp)
      rewind(p->fp);
}

/* Release all resources which were allocated in this module. */
void
finish_program(program)
  struct vector *program;
{
  /* close all files... */
  {
    struct output *p, *q;

    for (p=file_read; p; p=q)
      {
	if (p->fp)
	  ck_fclose(p->fp);
	q = p->link;
#if 0
	/* We use obstacks. */
	FREE(p);
#endif
      }

    for (p=file_write; p; p=q)
      {
	if (p->fp)
	  ck_fclose(p->fp);
	q = p->link;
#if 0
	/* We use obstacks. */
	FREE(p);
#endif
      }
    file_read = file_write = NULL;
  }

#ifdef DEBUG_LEAKS
  obstack_free (&obs, NULL);
#endif /*DEBUG_LEAKS*/
}
