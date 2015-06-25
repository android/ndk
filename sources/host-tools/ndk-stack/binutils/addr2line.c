/* addr2line.c -- convert addresses to line number and function name
   Copyright 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006,
   2007, 2009  Free Software Foundation, Inc.
   Contributed by Ulrich Lauther <Ulrich.Lauther@mchp.siemens.de>

   This file is part of GNU Binutils.

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
   Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */


/* Derived from objdump.c and nm.c by Ulrich.Lauther@mchp.siemens.de

   Usage:
   addr2line [options] addr addr ...
   or
   addr2line [options]

   both forms write results to stdout, the second form reads addresses
   to be converted from stdin.  */

#include "sysdep.h"
#include "bfd.h"
#include "getopt.h"
#include "libiberty.h"
#include "demangle.h"
#include "bucomm.h"
#include "elf-bfd.h"

static bfd_boolean unwind_inlines;	/* -i, unwind inlined functions. */
static bfd_boolean with_addresses;	/* -a, show addresses.  */
static bfd_boolean with_functions;	/* -f, show function names.  */
static bfd_boolean do_demangle;		/* -C, demangle names.  */
static bfd_boolean pretty_print;	/* -p, print on one line.  */
static bfd_boolean base_names;		/* -s, strip directory names.  */

static int naddr;		/* Number of addresses to process.  */
static char **addr;		/* Hex addresses to process.  */

static asymbol **syms;		/* Symbol table.  */

static struct option long_options[] =
{
  {"addresses", no_argument, NULL, 'a'},
  {"basenames", no_argument, NULL, 's'},
  {"demangle", optional_argument, NULL, 'C'},
  {"exe", required_argument, NULL, 'e'},
  {"functions", no_argument, NULL, 'f'},
  {"inlines", no_argument, NULL, 'i'},
  {"pretty-print", no_argument, NULL, 'p'},
  {"section", required_argument, NULL, 'j'},
  {"target", required_argument, NULL, 'b'},
  {"help", no_argument, NULL, 'H'},
  {"version", no_argument, NULL, 'V'},
  {0, no_argument, 0, 0}
};

static void usage (FILE *, int);
static void slurp_symtab (bfd *);
static void find_address_in_section (bfd *, asection *, void *);
static void find_offset_in_section (bfd *, asection *);
static void translate_addresses (bfd *, asection *);

/* Print a usage message to STREAM and exit with STATUS.  */

static void
usage (FILE *stream, int status)
{
  fprintf (stream, _("Usage: %s [option(s)] [addr(s)]\n"), program_name);
  fprintf (stream, _(" Convert addresses into line number/file name pairs.\n"));
  fprintf (stream, _(" If no addresses are specified on the command line, they will be read from stdin\n"));
  fprintf (stream, _(" The options are:\n\
  @<file>                Read options from <file>\n\
  -a --addresses         Show addresses\n\
  -b --target=<bfdname>  Set the binary file format\n\
  -e --exe=<executable>  Set the input file name (default is a.out)\n\
  -i --inlines           Unwind inlined functions\n\
  -j --section=<name>    Read section-relative offsets instead of addresses\n\
  -p --pretty-print      Make the output easier to read for humans\n\
  -s --basenames         Strip directory names\n\
  -f --functions         Show function names\n\
  -C --demangle[=style]  Demangle function names\n\
  -h --help              Display this information\n\
  -v --version           Display the program's version\n\
\n"));

  list_supported_targets (program_name, stream);
  if (REPORT_BUGS_TO[0] && status == 0)
    fprintf (stream, _("Report bugs to %s\n"), REPORT_BUGS_TO);
  exit (status);
}

/* Read in the symbol table.  */

static void
slurp_symtab (bfd *abfd)
{
  long storage;
  long symcount;
  bfd_boolean dynamic = FALSE;

  if ((bfd_get_file_flags (abfd) & HAS_SYMS) == 0)
    return;

  storage = bfd_get_symtab_upper_bound (abfd);
  if (storage == 0)
    {
      storage = bfd_get_dynamic_symtab_upper_bound (abfd);
      dynamic = TRUE;
    }
  if (storage < 0)
    bfd_fatal (bfd_get_filename (abfd));

  syms = (asymbol **) xmalloc (storage);
  if (dynamic)
    symcount = bfd_canonicalize_dynamic_symtab (abfd, syms);
  else
    symcount = bfd_canonicalize_symtab (abfd, syms);
  if (symcount < 0)
    bfd_fatal (bfd_get_filename (abfd));
}

/* These global variables are used to pass information between
   translate_addresses and find_address_in_section.  */

static bfd_vma pc;
static const char *filename;
static const char *functionname;
static unsigned int line;
static unsigned int discriminator;
static bfd_boolean found;

/* Look for an address in a section.  This is called via
   bfd_map_over_sections.  */

static void
find_address_in_section (bfd *abfd, asection *section,
			 void *data ATTRIBUTE_UNUSED)
{
  bfd_vma vma;
  bfd_size_type size;

  if (found)
    return;

  if ((bfd_get_section_flags (abfd, section) & SEC_ALLOC) == 0)
    return;

  vma = bfd_get_section_vma (abfd, section);
  if (pc < vma)
    return;

  size = bfd_get_section_size (section);
  if (pc >= vma + size)
    return;

  found = bfd_find_nearest_line_discriminator (abfd, section, syms, pc - vma,
                                               &filename, &functionname,
                                               &line, &discriminator);
}

/* Look for an offset in a section.  This is directly called.  */

static void
find_offset_in_section (bfd *abfd, asection *section)
{
  bfd_size_type size;

  if (found)
    return;

  if ((bfd_get_section_flags (abfd, section) & SEC_ALLOC) == 0)
    return;

  size = bfd_get_section_size (section);
  if (pc >= size)
    return;

  found = bfd_find_nearest_line_discriminator (abfd, section, syms, pc,
                                               &filename, &functionname,
                                               &line, &discriminator);
}

/* Read hexadecimal addresses from stdin, translate into
   file_name:line_number and optionally function name.  */

static void
translate_addresses (bfd *abfd, asection *section)
{
  int read_stdin = (naddr == 0);

  for (;;)
    {
      if (read_stdin)
	{
	  char addr_hex[100];

	  if (fgets (addr_hex, sizeof addr_hex, stdin) == NULL)
	    break;
	  pc = bfd_scan_vma (addr_hex, NULL, 16);
	}
      else
	{
	  if (naddr <= 0)
	    break;
	  --naddr;
	  pc = bfd_scan_vma (*addr++, NULL, 16);
	}

      if (bfd_get_flavour (abfd) == bfd_target_elf_flavour)
	{
	  const struct elf_backend_data *bed = get_elf_backend_data (abfd);
	  bfd_vma sign = (bfd_vma) 1 << (bed->s->arch_size - 1);

	  pc &= (sign << 1) - 1;
	  if (bed->sign_extend_vma)
	    pc = (pc ^ sign) - sign;
	}

      if (with_addresses)
        {
          printf ("0x");
          bfd_printf_vma (abfd, pc);

          if (pretty_print)
            printf (": ");
          else
            printf ("\n");
        }

      found = FALSE;
      if (section)
	find_offset_in_section (abfd, section);
      else
	bfd_map_over_sections (abfd, find_address_in_section, NULL);

      if (! found)
	{
	  if (with_functions)
	    printf ("??\n");
	  printf ("??:0\n");
	}
      else
	{
	  while (1)
            {
              if (with_functions)
                {
                  const char *name;
                  char *alloc = NULL;

                  name = functionname;
                  if (name == NULL || *name == '\0')
                    name = "??";
                  else if (do_demangle)
                    {
                      alloc = bfd_demangle (abfd, name, DMGL_ANSI | DMGL_PARAMS);
                      if (alloc != NULL)
                        name = alloc;
                    }

                  printf ("%s", name);
                  if (pretty_print)
		    /* Note for translators:  This printf is used to join the
		       function name just printed above to the line number/
		       file name pair that is about to be printed below.  Eg:

		         foo at 123:bar.c  */
                    printf (_(" at "));
                  else
                    printf ("\n");

                  if (alloc != NULL)
                    free (alloc);
                }

              if (base_names && filename != NULL)
                {
                  char *h;

                  h = strrchr (filename, '/');
                  if (h != NULL)
                    filename = h + 1;
                }

              printf ("%s:", filename ? filename : "??");
	      if (line != 0)
                {
                  if (discriminator != 0)
                    printf ("%u (discriminator %u)\n", line, discriminator);
                  else
                    printf ("%u\n", line);
                }
	      else
		printf ("?\n");
              if (!unwind_inlines)
                found = FALSE;
              else
                found = bfd_find_inliner_info (abfd, &filename, &functionname,
					       &line);
              if (! found)
                break;
              if (pretty_print)
		/* Note for translators: This printf is used to join the
		   line number/file name pair that has just been printed with
		   the line number/file name pair that is going to be printed
		   by the next iteration of the while loop.  Eg:

		     123:bar.c (inlined by) 456:main.c  */
                printf (_(" (inlined by) "));
            }
	}

      /* fflush() is essential for using this command as a server
         child process that reads addresses from a pipe and responds
         with line number information, processing one address at a
         time.  */
      fflush (stdout);
    }
}

/* Process a file.  Returns an exit value for main().  */

static int
process_file (const char *file_name, const char *section_name,
	      const char *target)
{
  bfd *abfd;
  asection *section;
  char **matching;

  if (get_file_size (file_name) < 1)
    return 1;

  abfd = bfd_openr (file_name, target);
  if (abfd == NULL)
    bfd_fatal (file_name);

  /* Decompress sections.  */
  abfd->flags |= BFD_DECOMPRESS;

  if (bfd_check_format (abfd, bfd_archive))
    fatal (_("%s: cannot get addresses from archive"), file_name);

  if (! bfd_check_format_matches (abfd, bfd_object, &matching))
    {
      bfd_nonfatal (bfd_get_filename (abfd));
      if (bfd_get_error () == bfd_error_file_ambiguously_recognized)
	{
	  list_matching_formats (matching);
	  free (matching);
	}
      xexit (1);
    }

  if (section_name != NULL)
    {
      section = bfd_get_section_by_name (abfd, section_name);
      if (section == NULL)
	fatal (_("%s: cannot find section %s"), file_name, section_name);
    }
  else
    section = NULL;

  slurp_symtab (abfd);

  translate_addresses (abfd, section);

  if (syms != NULL)
    {
      free (syms);
      syms = NULL;
    }

  bfd_close (abfd);

  return 0;
}

int
addr2line_main (int argc, char **argv) // Android changed.
{
  const char *file_name;
  const char *section_name;
  char *target;
  int c;

#if defined (HAVE_SETLOCALE) && defined (HAVE_LC_MESSAGES)
  setlocale (LC_MESSAGES, "");
#endif
#if defined (HAVE_SETLOCALE)
  setlocale (LC_CTYPE, "");
#endif
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  program_name = *argv;
  xmalloc_set_program_name (program_name);

  expandargv (&argc, &argv);

  bfd_init ();
  set_default_bfd_target ();

  file_name = NULL;
  section_name = NULL;
  target = NULL;
  optind = 1; /* make getopt_* reentrant */
  while ((c = getopt_long (argc, argv, "ab:Ce:sfHhij:pVv", long_options, (int *) 0))
	 != EOF)
    {
      switch (c)
	{
	case 0:
	  break;		/* We've been given a long option.  */
	case 'a':
	  with_addresses = TRUE;
	  break;
	case 'b':
	  target = optarg;
	  break;
	case 'C':
	  do_demangle = TRUE;
	  if (optarg != NULL)
	    {
	      enum demangling_styles style;

	      style = cplus_demangle_name_to_style (optarg);
	      if (style == unknown_demangling)
		fatal (_("unknown demangling style `%s'"),
		       optarg);

	      cplus_demangle_set_style (style);
	    }
	  break;
	case 'e':
	  file_name = optarg;
	  break;
	case 's':
	  base_names = TRUE;
	  break;
	case 'f':
	  with_functions = TRUE;
	  break;
        case 'p':
          pretty_print = TRUE;
          break;
	case 'v':
	case 'V':
	  print_version ("addr2line");
	  break;
	case 'h':
	case 'H':
	  usage (stdout, 0);
	  break;
	case 'i':
	  unwind_inlines = TRUE;
	  break;
	case 'j':
	  section_name = optarg;
	  break;
	default:
	  usage (stderr, 1);
	  break;
	}
    }

  if (file_name == NULL)
    file_name = "a.out";

  addr = argv + optind;
  naddr = argc - optind;

  return process_file (file_name, section_name, target);
}
