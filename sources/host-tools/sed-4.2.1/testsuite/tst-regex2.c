#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <fcntl.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int
main (int argc, char *argv[])
{
  struct stat st;
  static const char *pat[] = {
    ".?.?.?.?.?.?.?argc",
    "(.?)(.?)(.?)(.?)(.?)(.?)(.?)argc",
    "((((((((((.?))))))))))((((((((((.?))))))))))((((((((((.?))))))))))"
    "((((((((((.?))))))))))((((((((((.?))))))))))((((((((((.?))))))))))"
    "((((((((((.?))))))))))argc" };

  size_t len;
  int fd;
  int testno, i, j, k, l;
  char *string;
  char *buf;

  if (argc < 2)
    abort ();

  fd = open (argv[1], O_RDONLY);
  if (fd < 0)
    {
      printf ("Couldn't open %s: %s\n", argv[1], strerror (errno));
      abort ();
    }

  if (fstat (fd, &st) < 0)
    {
      printf ("Couldn't fstat %s: %s\n", argv[1], strerror (errno));
      abort ();
    }

  buf = malloc (st.st_size + 1);
  if (buf == NULL)
    {
      printf ("Couldn't allocate buffer: %s\n", strerror (errno));
      abort ();
    }

  if (read (fd, buf, st.st_size) != (ssize_t) st.st_size)
    {
      printf ("Couldn't read %s", argv[1]);
      abort ();
    }

  close (fd);
  buf[st.st_size] = '\0';

  string = buf;
  len = st.st_size;

  for (testno = 0; testno < 4; ++testno)
    for (i = 0; i < sizeof (pat) / sizeof (pat[0]); ++i)
      {
	regex_t rbuf;
	struct re_pattern_buffer rpbuf;
	int err;

	printf ("test %d pattern %d", testno, i);
	if (testno < 2)
	  {
	    err = regcomp (&rbuf, pat[i],
			   REG_EXTENDED | (testno ? REG_NOSUB : 0));
	    if (err != 0)
	      {
		char errstr[300];
		putchar ('\n');
		regerror (err, &rbuf, errstr, sizeof (errstr));
		puts (errstr);
		return err;
	      }
	  }
	else
	  {
	    const char *s;
	    re_set_syntax (RE_SYNTAX_POSIX_EGREP
			   | (testno == 3 ? RE_NO_SUB : 0));

	    memset (&rpbuf, 0, sizeof (rpbuf));
	    s = re_compile_pattern (pat[i], strlen (pat[i]), &rpbuf);
	    if (s != NULL)
	      {
		printf ("\n%s\n", s);
		abort ();
	      }

	    /* Just so that this can be tested with earlier glibc as well.  */
	    if (testno == 3)
	      rpbuf.no_sub = 1;
	  }

      if (testno < 2)
	{
	  regmatch_t pmatch[71];
	  err = regexec (&rbuf, string, 71, pmatch, 0);
	  if (err == REG_NOMATCH)
	    {
	      puts ("\nregexec failed");
	      abort ();
	    }

	  if (testno == 0)
	    {
	      if (pmatch[0].rm_eo != pmatch[0].rm_so + 11
		  || pmatch[0].rm_eo > len
		  || string + pmatch[0].rm_so >= strchr (string, 'R')
		  || strncmp (string + pmatch[0].rm_so,
			      "n (int argc",
			      sizeof "n (int argc" - 1)
		     != 0)
		{
		  puts ("\nregexec without REG_NOSUB did not find the correct match");
		  abort ();
		}

	      if (i > 0)
		for (j = 0, l = 1; j < 7; ++j)
		  for (k = 0; k < (i == 1 ? 1 : 10); ++k, ++l)
		    if (pmatch[l].rm_so != pmatch[0].rm_so + j
			|| pmatch[l].rm_eo != pmatch[l].rm_so + 1)
		      {
			printf ("\npmatch[%d] incorrect\n", l);
			abort ();
		      }
	    }
	}
      else
	{
	  struct re_registers regs;
	  int match;

	  memset (&regs, 0, sizeof (regs));
	  match = re_search (&rpbuf, string, len, 0, len,
				 &regs);
	  if (match < 0)
	    {
	      puts ("\nre_search failed");
	      abort ();
	    }

	  if (match + 11 > len
	      || string + match >= strchr (string, 'R')
	      || strncmp (string + match,
			  "n (int argc",
			  sizeof "n (int argc" - 1)
		  != 0)
	    {
	      puts ("\nre_search did not find the correct match");
	      abort ();
	    }

	  if (testno == 2)
	    {
	      if (regs.num_regs != 2 + (i == 0 ? 0 : i == 1 ? 7 : 70))
		{
		  printf ("\nincorrect num_regs %d\n", regs.num_regs);
		  abort ();
		}

	      if (regs.start[0] != match || regs.end[0] != match + 11)
		{
		  printf ("\nincorrect regs.{start,end}[0] = { %d, %d}\n",
			  regs.start[0], regs.end[0]);
		  abort ();
		}

	      if (regs.start[regs.num_regs - 1] != -1
		  || regs.end[regs.num_regs - 1] != -1)
		{
		  puts ("\nincorrect regs.{start,end}[num_regs - 1]");
		  abort ();
		}

	      if (i > 0)
		for (j = 0, l = 1; j < 7; ++j)
		  for (k = 0; k < (i == 1 ? 1 : 10); ++k, ++l)
		    if (regs.start[l] != match + j
			|| regs.end[l] != regs.start[l] + 1)
		      {
			printf ("\nregs.{start,end}[%d] incorrect\n", l);
			abort ();
		      }
	    }
	}

      putchar ('\n');

      if (testno < 2)
	regfree (&rbuf);
      else
	regfree (&rpbuf);
    }

  exit (0);
}
