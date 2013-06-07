/*
  Copyright (C) 2005-2012 Rich Felker

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  Modified in 2013 for the Android Open Source Project.
 */

//#include "stdio_impl.h"
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <wchar.h>
#include <inttypes.h>

// A wrapper around either a FILE* or a string buffer, used to output
// the result of formatted expansion.
typedef struct {
  FILE*    file;
  wchar_t* buffer;
  size_t   buffer_pos;
  size_t   buffer_size;
} Out;

void out_init_file(Out* out, FILE* f) {
  memset(out, 0, sizeof(*out));
  out->file = f;
}

void out_init_buffer(Out* out, wchar_t* buffer, size_t buffer_size) {
  memset(out, 0, sizeof(*out));
  out->buffer = buffer;
  out->buffer_pos = 0;
  out->buffer_size = buffer_size;
}

void out_write(Out* out, const wchar_t* text, size_t length) {
  if (out->file != NULL) {
    // Write into a file the UTF-8 encoded version.
    // TODO(digit): Support locale-specific encoding.
    size_t mb_len = wcstombs(NULL, text, length);
    char* mb_buffer = malloc(mb_len);
    wcstombs(mb_buffer, text, length);
    fwrite(mb_buffer, 1, mb_len, out->file);
    free(mb_buffer);
  } else {
    // Write into a bounded buffer.
    size_t avail = out->buffer_size - out->buffer_pos;
    if (length > avail)
      length = avail;
    memcpy((char*)(out->buffer + out->buffer_pos),
           (const char*)text,
           length * sizeof(wchar_t));
    out->buffer_pos += length;
  }
}

void out_putwc(Out* out, wchar_t wc) {
  if (out->file)
    fputwc(wc, out->file);
  else {
    if (out->buffer_pos < out->buffer_size)
      out->buffer[out->buffer_pos++] = wc;
  }
}

int out_printf(Out* out, const char* format, ...) {
  va_list args;
  va_start(args, format);
  if (out->file)
    return vfprintf(out->file, format, args);
  else {
    // TODO(digit): Make this faster.
    // First, generate formatted byte output.
    int mb_len = vsnprintf(NULL, 0, format, args);
    char* mb_buffer = malloc(mb_len + 1);
    vsnprintf(mb_buffer, mb_len + 1, format, args);
    // Then convert to wchar_t buffer.
    size_t wide_len = mbstowcs(NULL, mb_buffer, mb_len);
    wchar_t* wide_buffer = malloc((wide_len + 1) * sizeof(wchar_t));
    mbstowcs(wide_buffer, mb_buffer, mb_len);
    // Add to buffer.
    out_write(out, wide_buffer, wide_len);
    // finished
    free(wide_buffer);
    free(mb_buffer);

    return wide_len;
  }
  va_end(args);
}
int out_error(Out* out) {
  if (out->file != NULL)
    return ferror(out->file);

  return 0;
}

int out_overflow(Out* out) {
  if (out->file != NULL)
    return feof(out->file);
  else
    return (out->buffer_pos >= out->buffer_size);
}

/* Convenient bit representation for modifier flags, which all fall
 * within 31 codepoints of the space character. */

#define ALT_FORM   (1U<<'#'-' ')
#define ZERO_PAD   (1U<<'0'-' ')
#define LEFT_ADJ   (1U<<'-'-' ')
#define PAD_POS    (1U<<' '-' ')
#define MARK_POS   (1U<<'+'-' ')
#define GROUPED    (1U<<'\''-' ')

#define FLAGMASK (ALT_FORM|ZERO_PAD|LEFT_ADJ|PAD_POS|MARK_POS|GROUPED)

#if UINT_MAX == ULONG_MAX
#define LONG_IS_INT
#endif

#if SIZE_MAX != ULONG_MAX || UINTMAX_MAX != ULLONG_MAX
#define ODD_TYPES
#endif

/* State machine to accept length modifiers + conversion specifiers.
 * Result is 0 on failure, or an argument type to pop on success. */

enum {
	BARE, LPRE, LLPRE, HPRE, HHPRE, BIGLPRE,
	ZTPRE, JPRE,
	STOP,
	PTR, INT, UINT, ULLONG,
#ifndef LONG_IS_INT
	LONG, ULONG,
#else
#define LONG INT
#define ULONG UINT
#endif
	SHORT, USHORT, CHAR, UCHAR,
#ifdef ODD_TYPES
	LLONG, SIZET, IMAX, UMAX, PDIFF, UIPTR,
#else
#define LLONG ULLONG
#define SIZET ULONG
#define IMAX LLONG
#define UMAX ULLONG
#define PDIFF LONG
#define UIPTR ULONG
#endif
	DBL, LDBL,
	NOARG,
	MAXSTATE
};

#define S(x) [(x)-'A']

static const unsigned char states[]['z'-'A'+1] = {
	{ /* 0: bare types */
		S('d') = INT, S('i') = INT,
		S('o') = UINT, S('u') = UINT, S('x') = UINT, S('X') = UINT,
		S('e') = DBL, S('f') = DBL, S('g') = DBL, S('a') = DBL,
		S('E') = DBL, S('F') = DBL, S('G') = DBL, S('A') = DBL,
		S('c') = CHAR, S('C') = INT,
		S('s') = PTR, S('S') = PTR, S('p') = UIPTR, S('n') = PTR,
		S('m') = NOARG,
		S('l') = LPRE, S('h') = HPRE, S('L') = BIGLPRE,
		S('z') = ZTPRE, S('j') = JPRE, S('t') = ZTPRE,
	}, { /* 1: l-prefixed */
		S('d') = LONG, S('i') = LONG,
		S('o') = ULONG, S('u') = ULONG, S('x') = ULONG, S('X') = ULONG,
		S('c') = INT, S('s') = PTR, S('n') = PTR,
		S('l') = LLPRE,
	}, { /* 2: ll-prefixed */
		S('d') = LLONG, S('i') = LLONG,
		S('o') = ULLONG, S('u') = ULLONG,
		S('x') = ULLONG, S('X') = ULLONG,
		S('n') = PTR,
	}, { /* 3: h-prefixed */
		S('d') = SHORT, S('i') = SHORT,
		S('o') = USHORT, S('u') = USHORT,
		S('x') = USHORT, S('X') = USHORT,
		S('n') = PTR,
		S('h') = HHPRE,
	}, { /* 4: hh-prefixed */
		S('d') = CHAR, S('i') = CHAR,
		S('o') = UCHAR, S('u') = UCHAR,
		S('x') = UCHAR, S('X') = UCHAR,
		S('n') = PTR,
	}, { /* 5: L-prefixed */
		S('e') = LDBL, S('f') = LDBL, S('g') = LDBL, S('a') = LDBL,
		S('E') = LDBL, S('F') = LDBL, S('G') = LDBL, S('A') = LDBL,
		S('n') = PTR,
	}, { /* 6: z- or t-prefixed (assumed to be same size) */
		S('d') = PDIFF, S('i') = PDIFF,
		S('o') = SIZET, S('u') = SIZET,
		S('x') = SIZET, S('X') = SIZET,
		S('n') = PTR,
	}, { /* 7: j-prefixed */
		S('d') = IMAX, S('i') = IMAX,
		S('o') = UMAX, S('u') = UMAX,
		S('x') = UMAX, S('X') = UMAX,
		S('n') = PTR,
	}
};

#define OOB(x) ((unsigned)(x)-'A' > 'z'-'A')

union arg
{
	uintmax_t i;
	long double f;
	void *p;
};

static void pop_arg(union arg *arg, int type, va_list *ap)
{
	/* Give the compiler a hint for optimizing the switch. */
	if ((unsigned)type > MAXSTATE) return;
	switch (type) {
	       case PTR:	arg->p = va_arg(*ap, void *);
	break; case INT:	arg->i = va_arg(*ap, int);
	break; case UINT:	arg->i = va_arg(*ap, unsigned int);
#ifndef LONG_IS_INT
	break; case LONG:	arg->i = va_arg(*ap, long);
	break; case ULONG:	arg->i = va_arg(*ap, unsigned long);
#endif
	break; case ULLONG:	arg->i = va_arg(*ap, unsigned long long);
	break; case SHORT:	arg->i = (short)va_arg(*ap, int);
	break; case USHORT:	arg->i = (unsigned short)va_arg(*ap, int);
	break; case CHAR:	arg->i = (signed char)va_arg(*ap, int);
	break; case UCHAR:	arg->i = (unsigned char)va_arg(*ap, int);
#ifdef ODD_TYPES
	break; case LLONG:	arg->i = va_arg(*ap, long long);
	break; case SIZET:	arg->i = va_arg(*ap, size_t);
	break; case IMAX:	arg->i = va_arg(*ap, intmax_t);
	break; case UMAX:	arg->i = va_arg(*ap, uintmax_t);
	break; case PDIFF:	arg->i = va_arg(*ap, ptrdiff_t);
	break; case UIPTR:	arg->i = (uintptr_t)va_arg(*ap, void *);
#endif
	break; case DBL:	arg->f = va_arg(*ap, double);
	break; case LDBL:	arg->f = va_arg(*ap, long double);
	}
}

static int getint(wchar_t **s) {
	int i;
	for (i=0; iswdigit(**s); (*s)++)
		i = 10*i + (**s-'0');
	return i;
}

static const char sizeprefix['y'-'a'] = {
['a'-'a']='L', ['e'-'a']='L', ['f'-'a']='L', ['g'-'a']='L',
['d'-'a']='j', ['i'-'a']='j', ['o'-'a']='j', ['u'-'a']='j', ['x'-'a']='j',
['p'-'a']='j'
};

static int wprintf_core(Out *out, const wchar_t *fmt, va_list *ap, union arg *nl_arg, int *nl_type)
{
	wchar_t *a, *z, *s=(wchar_t *)fmt, *s0;
	unsigned l10n=0, litpct, fl;
	int w, p;
	union arg arg;
	int argpos;
	unsigned st, ps;
	int cnt=0, l=0;
	int i;
	int t;
	char *bs;
	char charfmt[16];
	wchar_t wc;

	for (;;) {
		/* Update output count, end loop when fmt is exhausted */
		if (cnt >= 0) {
			if (l > INT_MAX - cnt) {
				if (!out_error(out)) errno = EOVERFLOW;
				cnt = -1;
			} else cnt += l;
		}
		if (!*s) break;

		/* Handle literal text and %% format specifiers */
		for (a=s; *s && *s!='%'; s++);
		litpct = wcsspn(s, L"%")/2; /* Optimize %%%% runs */
		z = s+litpct;
		s += 2*litpct;
		l = z-a;
		if (out) out_write(out, a, l);
		if (l) continue;

		if (iswdigit(s[1]) && s[2]=='$') {
			l10n=1;
			argpos = s[1]-'0';
			s+=3;
		} else {
			argpos = -1;
			s++;
		}

		/* Read modifier flags */
		for (fl=0; (unsigned)*s-' '<32 && (FLAGMASK&(1U<<*s-' ')); s++)
			fl |= 1U<<*s-' ';

		/* Read field width */
		if (*s=='*') {
			if (iswdigit(s[1]) && s[2]=='$') {
				l10n=1;
				nl_type[s[1]-'0'] = INT;
				w = nl_arg[s[1]-'0'].i;
				s+=3;
			} else if (!l10n) {
				w = out ? va_arg(*ap, int) : 0;
				s++;
			} else return -1;
			if (w<0) fl|=LEFT_ADJ, w=-w;
		} else if ((w=getint(&s))<0) return -1;

		/* Read precision */
		if (*s=='.' && s[1]=='*') {
			if (isdigit(s[2]) && s[3]=='$') {
				nl_type[s[2]-'0'] = INT;
				p = nl_arg[s[2]-'0'].i;
				s+=4;
			} else if (!l10n) {
				p = out ? va_arg(*ap, int) : 0;
				s+=2;
			} else return -1;
		} else if (*s=='.') {
			s++;
			p = getint(&s);
		} else p = -1;

		/* Format specifier state machine */
		s0=s;
		st=0;
		do {
			if (OOB(*s)) return -1;
			ps=st;
			st=states[st]S(*s++);
		} while (st-1<STOP);
		if (!st) return -1;

		/* Check validity of argument type (nl/normal) */
		if (st==NOARG) {
			if (argpos>=0) return -1;
			else if (!out) continue;
		} else {
			if (argpos>=0) nl_type[argpos]=st, arg=nl_arg[argpos];
			else if (out) pop_arg(&arg, st, ap);
			else return 0;
		}

		if (!out) continue;
		t = s[-1];
		if (ps && (t&15)==3) t&=~32;

		switch (t) {
		case 'n':
			switch(ps) {
			case BARE: *(int *)arg.p = cnt; break;
			case LPRE: *(long *)arg.p = cnt; break;
			case LLPRE: *(long long *)arg.p = cnt; break;
			case HPRE: *(unsigned short *)arg.p = cnt; break;
			case HHPRE: *(unsigned char *)arg.p = cnt; break;
			case ZTPRE: *(size_t *)arg.p = cnt; break;
			case JPRE: *(uintmax_t *)arg.p = cnt; break;
			}
			continue;
		case 'c':
			out_putwc(out, btowc(arg.i));
			l = 1;
			continue;
		case 'C':
			out_putwc(out, arg.i);
			l = 1;
			continue;
		case 'S':
			a = arg.p;
			z = wmemchr(a, 0, p);
			if (!z) z=a+p;
			else p=z-a;
			if (w<p) w=p;
			if (!(fl&LEFT_ADJ)) out_printf(out, "%.*s", w-p, "");
			out_write(out, a, p);
			if ((fl&LEFT_ADJ)) out_printf(out, "%.*s", w-p, "");
			l=w;
			continue;
		case 's':
			bs = arg.p;
			if (p<0) p = INT_MAX;
			for (i=l=0; l<p && (i=mbtowc(&wc, bs, MB_LEN_MAX))>0; bs+=i, l++);
			if (i<0) return -1;
			p=l;
			if (w<p) w=p;
			if (!(fl&LEFT_ADJ)) out_printf(out, "%.*s", w-p, "");
			bs = arg.p;
			while (l--) {
				i=mbtowc(&wc, bs, MB_LEN_MAX);
				bs+=i;
				out_putwc(out, wc);
			}
			if ((fl&LEFT_ADJ)) out_printf(out, "%.*s", w-p, "");
			l=w;
			continue;
		}

		snprintf(charfmt, sizeof charfmt, "%%%s%s%s%s%s*.*%c%c",
			"#"+!(fl & ALT_FORM),
			"+"+!(fl & MARK_POS),
			"-"+!(fl & LEFT_ADJ),
			" "+!(fl & PAD_POS),
			"0"+!(fl & ZERO_PAD),
			sizeprefix[(t|32)-'a'], t);

		switch (t|32) {
		case 'a': case 'e': case 'f': case 'g':
			l = out_printf(out, charfmt, w, p, arg.f);
			break;
		case 'd': case 'i': case 'o': case 'u': case 'x': case 'p':
			l = out_printf(out, charfmt, w, p, arg.i);
			break;
		}
	}

	if (out) return cnt;
	if (!l10n) return 0;

	for (i=1; i<=NL_ARGMAX && nl_type[i]; i++)
		pop_arg(nl_arg+i, nl_type[i], ap);
	for (; i<=NL_ARGMAX && !nl_type[i]; i++);
	if (i<=NL_ARGMAX) return -1;
	return 1;
}

int vfwprintf(FILE *restrict f, const wchar_t *restrict fmt, va_list ap)
{
	va_list ap2;
	int nl_type[NL_ARGMAX] = {0};
	union arg nl_arg[NL_ARGMAX];
	int ret;
        Out out[1];
        out_init_file(out, f);
	va_copy(ap2, ap);
        // Check for error in format string before writing anything to file.
	if (wprintf_core(0, fmt, &ap2, nl_arg, nl_type) < 0) {
          va_end(ap2);
          return -1;
        }
	ret = wprintf_core(out, fmt, &ap2, nl_arg, nl_type);
	va_end(ap2);
	return ret;
}

int vswprintf(wchar_t *restrict s, size_t l, const wchar_t *restrict fmt, va_list ap)
{
  va_list ap2;
  int nl_type[NL_ARGMAX] = {0};
  union arg nl_arg[NL_ARGMAX];
  int ret;
  Out out[1];
  out_init_buffer(out, s, l);
  va_copy(ap2, ap);
  ret = wprintf_core(out, fmt, &ap2, nl_arg, nl_type);
  va_end(ap2);
  if (out_overflow(out)) return -1;
  return ret;
}
