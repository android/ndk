## Process this file with automake to produce Makefile.in
info_TEXINFOS = sed.texi
sed_TEXINFOS = config.texi version.texi
dist_man_MANS = sed.1
dist_noinst_DATA = sed.x sed-in.texi s-texi
dist_noinst_SCRIPTS = groupify.sed
TEXI2DVI = $(top_srcdir)/build-aux/texi2dvi --expand
HELP2MAN = $(top_srcdir)/build-aux/help2man
SED = $(top_builddir)/sed/sed

AM_MAKEINFOHTMLFLAGS = --no-split

# To produce better quality output, in the example sed
# scripts we group comments with lines following them;
# since mantaining the "@group...@end group" manually
# is a burden, we do this automatically
$(srcdir)/sed.texi: $(srcdir)/s-texi
$(srcdir)/s-texi: sed-in.texi $(srcdir)/groupify.sed
	sed -nf $(srcdir)/groupify.sed \
	  < $(srcdir)/sed-in.texi > $(srcdir)/sed-tmp.texi
	if cmp $(srcdir)/sed.texi $(srcdir)/sed-tmp.texi; then \
	  rm -f $(srcdir)/sed-tmp.texi; \
	else \
	  mv -f $(srcdir)/sed-tmp.texi $(srcdir)/sed.texi; \
	fi
	echo stamp > $(srcdir)/s-texi

sed.1: $(top_srcdir)/sed/sed.c $(top_srcdir)/configure.ac $(srcdir)/sed.x
	$(HELP2MAN) --name "stream editor for filtering and transforming text" \
	  -p sed --include $(srcdir)/sed.x -o $(srcdir)/sed.1 $(SED) 

dist-hook:
	touch $(distdir)/sed.1
